<?php
/**
 * @file
 * Provides functionality for loading a variant call format (VCF) file.
 */

/**
 * Parses a VCF file and inserts markers, variants, and genotype calls into the database.
 *
 * @param $input_file
 *   The absolute path of the VCF file to be parsed.
 * @param $options
 *   An associative array of additional information. Keys include:
 *     - organism_id: the organism_id of both the marker/variants and samples/germplasm.
 *     - variant_type: the cvterm.name of the feature.type_id for the variant.
 *     - feature_type_of_marker: the cvterm_name of the feature.type_id for the marker.
 *     - marker_type: the cvterm.name of the feature.type_id for the marker.
 *     - project_name: the name of the project.
 *     - storage_method: the method to use when storing the genotypes;
 *        one of nd_exp, genotype_call, stock_genotype.
 *     - insert_samples: whether to insert the sample record if it doesn't already exist;
 *        one of 0: Select Only, 1: Insert Only, 2: Insert & Select.
 *     - insert_germplasm: whether to insert the germplasm record if it doesn't already exist;
 *        one of 0: Select Only, 1: Insert Only, 2: Insert & Select.
 *     - insert_markers: whether to insert the marker record if it doesn't already exist;
 *        one of 0: Select Only, 1: Insert Only, 2: Insert & Select.
 *     - insert_variants: whether to insert the variant record if it doesn't already exist;
 *        one of 0: Select Only, 1: Insert Only, 2: Insert & Select.
 *     - nd_geolocation: nd_geolocation.description; only if the ND Experiment storage
 *        method is chosen.
 * @param $terms
 *  An associative array of cvterms and their IDs in Chado which are required by the loader,
 *  such as types of features and stocks, and relationships.
 */

function genotypes_loader_load_VCF($input_file = NULL, $options, $terms) {

  // Open our VCF file and iterate through meta lines until we reach our header
  $VCFfile = fopen($input_file, 'r') or die ("ERROR: Unable to open $input_file!");
  while(!feof($VCFfile)) {
    # Get each meta line to skip processing them
    $current_line = fgets($VCFfile);
    if (preg_match('/^#CHROM/', $current_line)) {
      $current_line = trim($current_line);
      $header = explode("\t",$current_line);
      break;
    }
  }

  // Select variables for the helper functions.
  $select_only = 0;
  $insert_only = 1;
  $both = 2;

  // Set the file name to be used for remote copy
  if ($options['storage_method'] == 'genotype_call') {
    // Pull out the name of the database to ensure the file is not overwritten by a concurrent upload
    // to another database (yes, I speak from experience)
    global $databases;
    $db_name = $databases['default']['default']['database'];
    $table_name = 'genotype_call';
    $remote_copy_filename = '/tmp/genotypes_loader.' . $db_name . '-' . $table_name . '.remotecopy.csv';
  }

  // -------------------------
  //    PROCESSING VCF FILE
  // -------------------------

  // Get all the sample names in the header in order to confirm that they match what was provided
  // in the samples file
  $geno_file_sample_names = array_slice($header, 10);
  foreach ($geno_file_sample_names as $sample) {
    if (!$options['sample_list'][$sample]) {
      return tripal_report_error('genotypes_loader', TRIPAL_ERROR,'ERROR: Sample @sample_name in your genotypes file does not match any samples provided in your samples file.', array('@sample_name' => $sample));
    }
  }

  $total_lines = intval(exec("grep -vc '^#' " . escapeshellarg($input_file)));
  if ($total_lines) { print("Number of lines to process: $total_lines"); }
  else {
    return tripal_report_error('genotypes_loader', TRIPAL_ERROR,'ERROR: Unable to count the number of lines in file @input_file', array('@input_file' => $input_file));
  }
  // Setting this to 0 since we're counting number of lines with SNPs, not header lines.
  $num_lines = 0;
  // Start the progress bar
  $progress = genotypes_loader_print_progress($num_lines, $total_lines);
  print($progress);

  // Start the counter for number of (non-empty) genotypic calls
  $num_calls = 0;

  // Now going line by line to pull out the marker, then genotypes.
  while(!feof($VCFfile)) {

    $current_line = fgetcsv($VCFfile, 0, "\t");
    if (empty($current_line)) continue;

    // Use the header to create the keys for each marker array
    $marker = array_combine($header, $current_line);

    // ------------------------
    // INSERT MARKER/VARIANT
    // ------------------------
    // Pull out the backbone (chromosome, scaffold, etc...) and check that it exists.
    // Since a VCF file is the result of alignments to a genome that should already be in the database,
    // we assume that the "chromosomes" already exist, so let's select and store the feature id ahead of time.
    // ----- Chromosome -----
    $backbone_name = $marker['#CHROM'];
    $chromosome_id = genotypes_loader_helper_add_record_with_mode('Chromosome', 'feature', $select_only, array(
        'name' => $backbone_name,
        'uniquename' => $backbone_name,
        'organism_id' => $options['organism_id'],
    ));
    if (!$chromosome_id) { return tripal_report_error('genotypes_loader',TRIPAL_ERROR,'ERROR: Could not find a chromosome ID for @backbone', array('@backbone' => $backbone_name)); }

    // If there is a value for ID, use it as the variant name.
    $variant_name = $marker['ID'];
    if ($variant_name == '.') {
      // @TODO: This works for KP but do we want to make it generic?
      $variant_name = $marker['#CHROM'] . 'p' . $marker['POS'];
    }

    $marker_name = $variant_name . ' ' . ucwords(str_replace('_',' ',$options['marker_type']));

    // Now create a variant and marker in Chado, and link it to a chromosome.
    // ----- Variant -----
    $variant_id = genotypes_loader_helper_add_record_with_mode('Variant', 'feature', $options['insert_variants'], array(
      'name' => $variant_name,
      'uniquename' => $variant_name,
      'organism_id' => $options['organism_id'],
      'type_id' => $terms['variant_type']['cvterm_id'],
    ));
    if (!$variant_id) { return tripal_report_error('genotypes_loader', TRIPAL_ERROR,'ERROR: Could not find a variant ID for @variant', array('@variant' => $variant_name)); }

    // ----- Variant Type -----
    $variant_type_prop_id = genotypes_loader_helper_add_record_with_mode('Variant Type', 'featureprop', $both, array(
      'feature_id' => $variant_id,
      'type_id' => $terms['free_text_variant_type']['cvterm_id'],
      'value' => $options['variant_type'],
    ));
    if (!$variant_type_prop_id) { return tripal_report_error('genotypes_loader', TRIPAL_ERROR,'ERROR: Could not find a variant type property ID for @variant_type', array('@variant_type' => $variant_name)); }

    // ----- Marker -----
    $marker_id = genotypes_loader_helper_add_record_with_mode('Marker', 'feature', $options['insert_markers'], array(
      'name' => $marker_name,
      'uniquename' => $marker_name,
      'organism_id' => $options['organism_id'],
      'type_id' => $terms['marker_type']['cvterm_id'],
    ));
    if (!$marker_id) { return tripal_report_error('genotypes_loader', TRIPAL_ERROR,'ERROR: Could not find a marker ID for @marker', array('@marker' => $marker_name)); }

    // ----- Marker Type -----
    $marker_type_prop_id = genotypes_loader_helper_add_record_with_mode('Marker Type', 'featureprop', $both, array(
      'feature_id' => $marker_id,
      'type_id' => $terms['free_text_marker_type']['cvterm_id'],
      'value' => $options['marker_type'],
    ));
    if (!$marker_type_prop_id) { return tripal_report_error('genotypes_loader', TRIPAL_ERROR,'ERROR: Could not find a marker type property ID for @marker_type', array('@marker_type' => $marker_name)); }

    // ----- Link Marker to Project -----
    $status = genotypes_loader_helper_add_record_with_mode('Marker Project Link', 'project_feature', $both, array(
      'feature_id' => $marker_id,
      'project_id' => $options['project_id'],
    ));
    if (!$status) { return tripal_report_error('genotypes_loader', TRIPAL_ERROR,'ERROR: Could not link marker @marker to project @project', array(
      '@project' => $options['project_name'],
      '@marker' => $marker_name));
    }

    // ----- Link Variant to Marker -----
    $status = genotypes_loader_helper_add_record_with_mode('Marker Variant Link', 'feature_relationship', $both, array(
      'subject_id' => $marker_id,
      'type_id' => $terms['marker_variant_relationship']['cvterm_id'],
      'object_id' => $variant_id,
    ));
    if (!$status) { return tripal_report_error('genotypes_loader', TRIPAL_ERROR,'ERROR: Could not link variant @variant to marker @marker', array(
      '@variant' => $variant_name,
      '@marker' => $marker_name));
    }

    // ----- Locate Variant on Chromosome -----
    $status = genotypes_loader_helper_add_record_with_mode('Variant location on Chromsome', 'featureloc', $both, array(
      'feature_id' => $variant_id,
      'srcfeature_id' => $chromosome_id,
      'fmin' => $marker['POS'] - 1,
      'fmax' => $marker['POS']
    ));
    if (!$status) { return tripal_report_error('genotypes_loader', TRIPAL_ERROR,
      'ERROR: Could not locate variant @variant on backbone @backbone', array(
      '@variant' => $variant_name,
      '@backbone' => $backbone_name));
    }

    // ----- Locate Marker on Chromosome -----
    $status = genotypes_loader_helper_add_record_with_mode('Marker location on Chromsome', 'featureloc', $both, array(
      'feature_id' => $marker_id,
      'srcfeature_id' => $chromosome_id,
      'fmin' => $marker['POS'] - 1,
      'fmax' => $marker['POS']
    ));
    if (!$status) { return tripal_report_error('genotypes_loader', TRIPAL_ERROR,
      'ERROR: Could not locate marker @marker on backbone @backbone', array(
      '@marker' => $marker_name,
      '@backbone' => $backbone_name));
    }

    // ------------------------
    //     INSERT GENOTYPES
    // ------------------------

    // The alternate allele can potentially be multiple alleles, so this calls for an associative array
    $ref_allele = $marker['REF'];
    $alt_allele = explode(',',$marker['ALT']);
    $RefAlt_alleles = $alt_allele;
    array_unshift($RefAlt_alleles, $ref_allele);

    // Similarly, let's explode the FORMAT cell
    $format_keys = explode(':',$marker['FORMAT']);

    // For each genotype source, explode the format fields so that we can pull out the genotype directly
    for ($i=9;$i<count($header);$i++) {
      $source_name = $header[$i];
      if (!$options['sample_list'][$source_name]) {
        return tripal_report_error('genotypes_loader', TRIPAL_ERROR,'ERROR: Sample @source_name in your genotypes file does not match any samples provided in your samples file.', array('@source_name' => $source_name));
      }

      if ($format_keys) {
        $num_fk = count($format_keys);
        $format_fields = explode(":",$marker[$source_name]);
        $num_ff = count($format_fields);
        // NOTE: Why the following comparison between format key and field counts?
        // Occasionally we may see that the format field will specify additional sub-fields than what is
        // actually present for that source. This is because of questionably loose requirements for VCF:
        // missing sub-fields are shown as ".", except for trailing fields which can be dropped. Therefore,
        // if there are fewer format fields for a source than expected, we will add onto the end of the
        // array the appropriate number of missing values.
        while ($num_fk > $num_ff)
        {
          array_push($format_fields,".");
          $num_ff = count($format_fields);
        }
        $geno_field_arr = array_combine($format_keys,$format_fields);
        // Now we nest the marker array with this new genotype fields array for each source
        // Example: Marker -> Source1 -> GT -> 0/1
        //                            -> DP -> 25
        $marker[$source_name] = $geno_field_arr;
      } else {
        $genotype_field = [];
        $genotype_field[$marker['FORMAT']] = $marker[$source_name];
        $marker[$source_name] = $genotype_field;
      }

      // Create an associative array of "metadata" that will be converted to a JSON blob if
      // the storage method chosen is the custom genotype call table.
      $metadata = array();
      foreach ($format_keys as $subfield) {
        // We are already saving the genotype, so ignore GT. Also, ignore any subfields with missing data.
        if (($subfield != 'GT') && (preg_match('/\b\.*\b/', $marker[$source_name][$subfield]))) {
          $metadata[$subfield] = $marker[$source_name][$subfield];
        }
      }

      // Explode any multi-ploidy calls
      // Also saving phased vs unphased info in metadata blob
      $genotype_calls = $marker[$source_name]['GT'];
      if (preg_match('/\//', $genotype_calls)) {
        $marker[$source_name]['GT'] = explode('/', $genotype_calls);
        $metadata['Phased'] = 'False';
      }
      else if (preg_match('/|/', $genotype_calls)) {
        $marker[$source_name]['GT'] = explode('|', $genotype_calls);
        $metadata['Phased'] = 'True';
      }
      $genotype_calls = $marker[$source_name]['GT'];

      // Translate the genotypic calls to be nucleotides (ie. AA, CC, etc...)
      $allele = "";
      foreach ($genotype_calls as $call) {
        $allele .= $RefAlt_alleles[$call];
        // If we end up with an allele that is longer than 2 nucleotides, we likely have an indel...
        // @TODO: Handle indels!
        if ( strlen($RefAlt_alleles[$call]) > 2 ) {
          drush_log("Found an indel for marker: " . $marker_name, 'warning');
          continue;
        }
      }

      // If $allele is empty, it means we have missing data for this genotype (ie. ./.)
      // Thus, skip this particular genotype and move onto the next.
      if (!$allele) { continue; }

      // Since it is non-empty, increment the genotype call count
      $num_calls++;

      $fields = array(
        'project_id' => $options['project_id'],
        'project_name' => $options['project_name'],
        'marker_id' => $marker_id,
        'marker_name' => $marker_name,
        'marker_type' => $options['marker_type'],
        'feature_type_of_marker' => $options['feature_type_of_marker'],
        'variant_id' => $variant_id,
        'variant_name' => $variant_name,
        'variant_type_id' => $terms['variant_type']['cvterm_id'],
	'variant_type' => $options['variant_type'],
	'genotype_type_id' => $terms['genotype_type']['cvterm_id'],
        'stock_id' => $options['sample_list'][$source_name]['sample_stock_id'],
        'stock_name' => $options['sample_list'][$source_name]['sample_name'],
        'allele' => $allele,
      );

      // Insert genotypes based on the preferred storage method
      if ($options['storage_method'] == 'genotype_call') {
        $fields['meta_data'] = json_encode($metadata);
        $status = genotypes_loader_helper_add_genotypes_genotype_call($fields, $num_calls, $remote_copy_filename);
      }
      // @TODO: ND genotypes option - deprecating this in the future?
      else if ($options['storage_method'] == 'nd_geolocation') {
        $fields['nd_geolocation'] = $options['nd_geolocation'];
        $status = genotypes_loader_helper_add_genotypes_nd_exp($fields);
      }
      else if ($options['storage_method'] == 'stock_genotype') {
        $status = genotypes_loader_helper_add_genotypes_stock_genotype($fields);
      }
      // @TODO: Return a useful error message
      if (!$status) { return FALSE; }
    }

    // Update progress
    $num_lines++;
    $progress = genotypes_loader_print_progress($num_lines, $total_lines);
    print($progress);
  }

  // Since we finished inserting the calls, we may want to clean up if we were using the COPY method
  if ($options['storage_method'] == 'genotype_call') {
    print("\nCopying in the final batch of calls...\n");
    $status = genotypes_loader_helper_copy_records($num_calls, NULL, NULL, array(), $remote_copy_filename, TRUE);
  }

  // If we got here without returning an error then we completed successfully!
  //drush_log("Loading Complete.", "success");
  print("Loading Complete.\n");

  // Close the file.
  fclose($VCFfile);

  return TRUE;
}
