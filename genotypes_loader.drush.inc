<?php

/**
 * @file
 * Implements a drush command to load a VCF file
 */

/**
 * Implements hook_drush_command().
 */
function genotypes_loader_drush_command() {

  $items['load-VCF'] = array(
    'description' => 'Takes a VCF file and associates it with a project in KnowPulse, then imports marker, variant and genotype data into KnowPulse.',
    'aliases' => array('load-vcf'),
    'arguments' => array(
      'input_file' => 'The filename of the VCF file for upload',
    ),
  );

  // Make this one drush command for all 3 file formats?
  $items['load-genotype-matrix'] = array(
    'description' => 'Takes a genotype matrix and associates it with a project in Knowpulse, and stores marker, variant and genotype data.',
    'aliases' => array('load-matrix'),
    'arguments' => array(
      'input_file' => 'The filename of the matrix file for upload',
    ),
    'options' => array(
      'organism' => 'The organism to which the genotypes are associated with.',
      'variant_type' => 'The Sequence Ontology (SO) term name that describes the type of variants in the file (eg. SNP, MNP, indel).',
      'marker_type' => 'The Sequence Ontology (SO) term name that describes the marker technology used to generate the genotypes in the file (e.g. "Exome Capture", "GBS", "KASPar", etc.).',
      'project_name' => 'All genotypes will be grouped via a project to allow users to specify a particular dataset.',
      'nd_geolocation' => 'A meaningful location associated with this natural diversity experiment. For example, this could be the location the assay was completed in, the location the germplasm collection was from, or the location the markers were developed at. This should be the description field of your nd_geolocation.',
    ),
  );

  return $items;
}

function drush_genotypes_loader_load_genotype_matrix($input_file = NULL) {
  // Collect options from the user.
  // Organism
  $organism = drush_get_option('organism');
  if (!$organism) {
    $sql = "SELECT * FROM {organism} ORDER BY genus, species";
    $org_rset = chado_query($sql);
    $organisms = array();
    while ($organism = $org_rset->fetchObject()) {
      $organisms[$organism->organism_id] = "$organism->genus $organism->species ($organism->common_name)";
    }
    $organism = drush_choice($organisms, dt('Please select which organism the genotypes are associated with.'));
  } else {
    // @TODO: Look up the organism ID if it was provided as an option
  }

  // Variant type
  $variant_type = drush_get_option('variant_type');
  if (!$variant_type) {
    //$values = array('name' => 'sequence');
    //$cv = chado_select_record('cv', array('cv_id'), $values);
    //$cv_id = $cv[0]->cv_id;
    $variant_type = drush_prompt(dt('Please enter the Sequence Ontology (SO) term name that describes the type of variants in the file (eg. SNP, MNP, indel).'));
    // @TODO: Validate that this is a SO term?
  }

  // Marker type
  $marker_type = drush_get_option('marker_type');
  if (!$marker_type) {
    $marker_type = drush_prompt(dt('Please enter the Sequence Ontology (SO) term name that describes the marker technology used to generate the genotypes in the file (e.g. "Exome Capture", "GBS", "KASPar", etc.).'));
    // @TODO: Validate that this is a SO term?
  }

  // Project
  // @TODO: Should this be retrieved using drush_choice instead?
  $project_name = drush_get_option('project_name');
  if (!$project_name) {
    $project_name = drush_prompt(dt('Please provide the name of a project in which all genotypes will be grouped to allow users to specify a particular dataset.'));
  }

  // If the storage method is ND experiment, then grab this option.
  // ND Geolocation
  $nd_geolocation = drush_get_option('nd_geolocation');
  if (!$nd_geolocation) {
    $nd_geolocation = drush_prompt(dt('Please provide a meaningful location associated with this natural diversity experiment. For example, this could be the location the assay was completed in, the location the germplasm collection was from, or the location the markers were developed at. This should be the description field of your nd_geolocation.'));
    // @TODO: Validate?
  }

  //$storage_method = variable_get('nd_genotype_storage_method', 'nd_exp');


  // Check the file exists
//   if (!$input_file) {
//     return drush_set_error(dt('ERROR: No matrix file specified. See: \'drush help load-genotype-matrix\''));
//   }
//   // Otherwise, let's assume we need the full path for the file by using the cwd
//   else {
//     $current_directory = drush_cwd();
//     $input_file = $current_directory . "/" . $input_file;
//     // Test the file again, return error if we still can't find it.
//     if (!file_exists($input_file)) {
//       return drush_set_error(dt("ERROR: Could not locate $input_file\nPlease ensure you spelled your file correctly or try using the full path."));
//     }
//   }

  print_r("Organism : $organisms[$organism]\n");
  print_r("Variant type: $variant_type\n");
  print_r("Marker type: $marker_type\n");
  print_r("Project: $project_name\n");
  print_r("ND_geolocation: $nd_geolocation\n");

  // @TODO: Create an array containing the options (see @param $options in:
  // genotypes_loader.genotype_matrix.inc) Will need to acquire the DB-specific
  // options somehow...
  // Then:
  //include('genotypes_loader.genotype_matrix.inc');
  //genotypes_loader_load_genotype_matrix($input_file, $options, $user->uid);
}

/**
 * Callback for load-VCF command
 */
function drush_genotypes_loader_load_VCF($input_file = NULL) {

  // TEST INPUTS. @TODO: Get these as options
  $options = array(
    'organism_id' => '4',
    'variant_type' => 'SNP',
    'feature_type_of_marker' => 'genetic_marker',
    'marker_type' => 'GBS',
    'project_name' => 'AGILE: Application of Genomic Innovation in the Lentil Economy',
    'storage_method' => 'nd_exp',
    'insert_samples' => '0',
    'insert_germplasm' => '0',
    'insert_markers' => '2',
    'insert_variants' => '2',
    'nd_geolocation' => '2C33',
  );

  // Retrieve all the type_ids to save time loading.
  $terms = array('DNA', 'Accession', 'is_extracted_from', $options['variant_type'], $options['marker_type'],'is_marker_of','Individual','genetic_marker');
  $query = 'SELECT cvterm_id FROM {cvterm} WHERE name=:name';
  $types = array();
  foreach ($terms as $term) {
    $types[$term] = chado_query($query, array(':name' => $term))->fetchField();
    drush_print("Type: $term, ID: $types[$term] \n");
  }

  // Retrieve the name of our organism and the project ID
  // @TODO: Output a friendly error when something is not found?
  $organism_name = chado_query(
     "SELECT organism.genus||' '||organism.species
        FROM {organism}
        WHERE organism_id=:id",
     array(':id' => $options['organism_id'])
  )->fetchField();
  $project_id = chado_query(
     'SELECT project_id
        FROM {project}
        WHERE name=:name',
     array(':name' => $options['project_name'])
  )->fetchField();

  // Check the file exists
  $input_file = trim($input_file);
  if (!$input_file) {
    return drush_set_error(dt('ERROR: No VCF file specified. See: \'drush help load-VCF\''));
  }
  // Otherwise, let's assume we need the full path for the file by using the cwd
  else {
    $current_directory = drush_cwd();
    $input_file = $current_directory . "/" . $input_file;
    // Test the file again, return error if we still can't find it.
    if (!file_exists($input_file)) {
      return drush_set_error(dt("ERROR: Could not locate $input_file\nPlease ensure you spelled your file correctly or that the web server has permissions to read the file."));
    }
  }

  // Open the file and check the first line to ensure it is in VCF format
  // ie. 1st line should be: ##fileformat=VCFv4.x
  $VCFfile = fopen($input_file, 'r');
  $first_line = fgets($VCFfile);
  if (!preg_match('/^##fileformat/', $first_line)) {
    return drush_set_error(dt("ERROR: First line of file $VCFfile does not meet VCF file requirements."));
  }

  // Iterate through remaining meta lines until we reach our header
  while(!feof($VCFfile)) {
    # Get each meta line to skip processing them
    $current_line = fgets($VCFfile);
    if (preg_match('/^#CHROM/', $current_line)) {
      $current_line = trim($current_line);
      $header = explode("\t",$current_line);
      break;
    }
  }

  // Now that we have our header, look up samples/germplasm
  // If a sample doesn't already exist and insert samples/germplasm is TRUE then create it.
  drush_print(dt("Looking up samples..."));

  // @TEST DATA
  // A mapping of KP:GERM#s to germplasm names to test in /dev/clone
  $KP_germs = array(
    '964a-46' => 'KP:GERM6755',
    '1048-8R' => 'KP:GERM7951',
    'Giftgi' => 'KP:GERM9771',
  );

  $stock_ids = array();
  foreach ($header as $k => $name) {
    // Skip over columns not containing a sample name
    if ($k <= 8) continue;

    // Sample
    drush_print("Currently on sample $name");
    $stock_id = genotypes_loader_helper_add_record_with_mode('Sample', 'stock', $options['insert_samples'], array(
      'uniquename' => $KP_germs[$name] . '_LAM_plate1_extraction',
      'organism_id' => $options['organism_id'],
      // Hardcoded the type since DNA is a stock type and cv term- we want the stock type here.
      // @TODO: Don't make assumptions!
      'type_id' => '3630',
    ), array(
      'name' => $name . ' LAM Plate1 Extraction',
    ));
    if ($stock_id) { $stock_ids[$k]['sample'] = $stock_id; }
    else { return FALSE; }


    // Germplasm.
    drush_print("Currently on germplasm $name");
    $germplasm_id = genotypes_loader_helper_add_record_with_mode('Germplasm', 'stock', $options['insert_germplasm'], array(
      'uniquename' => $KP_germs[$name],
      'organism_id' => $options['organism_id'],
      'type_id' => $types['Individual']
    ), array(
      'name' => $name,
    ));
    if ($germplasm_id) { $stock_ids[$k]['germplasm'] = $stock_id; }
    else { return FALSE; }

    // Now Link them.
    $status = genotypes_loader_helper_add_record_with_mode('Germplasm to Sample Link', 'stock_relationship', 2, array(
      'subject_id' => $stock_id,
      'type_id' => $types['is_extracted_from'],
      'object_id' => $germplasm_id,
    ));
    if (!$status) { return FALSE; }
  }

  drush_print("Done processing samples and germplasm!");
  $total_lines = intval(exec("grep -vc '^#' $input_file"));
  drush_print("Total Lines=".$total_lines);
  // Setting this to 0 since we're counting number of lines with SNPs, not header lines.
  $num_lines = 0;
  // Start the progress bar
  $progress = genotypes_loader_print_progress($num_lines, $total_lines);
  drush_print($progress);

  // Now processing the SNP calls
  while(!feof($VCFfile)) {
    $current_line = fgetcsv($VCFfile, 0, "\t");

    if (empty($current_line)) continue;
    // Use the header to create the keys for each marker array
    $marker = array_combine($header, $current_line);

    // Select variables for the helper functions.
    $select_only = 0;
    $insert_only = 1;
    $both = 2;

    // Pull out the backbone (chromosome, scaffold, etc...) and check that it exists.
    // Since a VCF file is the result of alignments to a genome that should already be in the database,
    // we assume that the "chromosomes" already exist, so let's select and store the feature id ahead of time.
    $chromosome_id = genotypes_loader_helper_add_record_with_mode('Chromosome', 'feature', $select_only, array(
        'name' => $marker['#CHROM'],
        'uniquename' => $marker['#CHROM'],
        'organism_id' => $options['organism_id'],
    ));
    if (!$chromosome_id) { return FALSE; }

    // This works for KP but do we want to make it generic?
    $variant_name = $marker['#CHROM'] . 'p' . $marker['POS'];
    $marker_name = $variant_name . '_' . $options['marker_type'];

    // Now create a marker and variant in Chado, and link it to a chromosome.
    // Variant.
    $variant_id = genotypes_loader_helper_add_record_with_mode('Variant', 'feature', $options['insert_variants'], array(
      'name' => $variant_name,
      'uniquename' => $variant_name,
      'organism_id' => $options['organism_id'],
      'type_id' => $types[ $options['variant_type'] ]
    ));
    if (!$variant_id) { return FALSE; }

    // Marker.
    $marker_id = genotypes_loader_helper_add_record_with_mode('Marker', 'feature', $options['insert_markers'], array(
      'name' => $marker_name,
      'uniquename' => $marker_name,
      'organism_id' => $options['organism_id'],
      'type_id' => $types[ $options['feature_type_of_marker'] ]
    ));
    if (!$marker_id) { return FALSE; }

    // Marker Type.
    $marker_type_prop_id = genotypes_loader_helper_add_record_with_mode('Marker Type', 'featureprop', $both, array(
      'feature_id' => $marker_id,
      'type_id' => array('name' => 'marker_type'),
      'value' => 'Genotype-by-Sequencing',
    ));
    if (!$marker_type_prop_id) { return FALSE; }

    // Link Variant to marker.
    $status = genotypes_loader_helper_add_record_with_mode('Marker Variant Link', 'feature_relationship', $both, array(
      'subject_id' => $marker_id,
      'type_id' => $types['is_marker_of'],
      'object_id' => $variant_id,
    ));
    if (!$status) { return FALSE; }

    // Locate the variant on chromosome.
    $status = genotypes_loader_helper_add_record_with_mode('Variant location on Chromsome', 'featureloc', $both, array(
      'feature_id' => $variant_id,
      'srcfeature_id' => $chromosome_id,
      'fmin' => $marker['POS'] - 1,
      'fmax' => $marker['POS']
    ));
    if (!$status) { return FALSE; }

    // Locate the marker on chromosome.
    $status = genotypes_loader_helper_add_record_with_mode('Marker location on Chromsome', 'featureloc', $both, array(
      'feature_id' => $marker_id,
      'srcfeature_id' => $chromosome_id,
      'fmin' => $marker['POS'] - 1,
      'fmax' => $marker['POS']
    ));
    if (!$status) { return FALSE; }

    // The alternate allele can potentially be multiple alleles, so this calls for an associative array
    $ref_allele = $marker['REF'];
    $alt_allele = explode(',',$marker['ALT']);
    $RefAlt_alleles = $alt_allele;
    array_unshift($RefAlt_alleles, $ref_allele);

    // Similarly, let's explode the FORMAT cell
    /* @TODO: As an aside... how are we going to handle the information provided in the INFO field?
           This involves stats that are specific to the variant, not by SNP. Lacey suggests using
           the feature_prop table and storing as an array. The issue is whether or not we want to
           trust this information, since this is dependent on how the file was filtered. If individual
           SNPs are filtered out, was the INFO updated properly or is it even useful for anything other
           than for filtering? */
    $format_keys = explode(':',$marker['FORMAT']);

    // For each sample, explode the format fields so that we can pull out the genotype directly
    for ($i=9;$i<count($header);$i++) {
      $sample_name = $header[$i];
      if ($format_keys) {
        $num_fk = count($format_keys);
        $format_fields = explode(":",$marker[$sample_name]);
        $num_ff = count($format_fields);
        // NOTE: Why the following comparison between format key and field counts?
        // Occasionally we may see that the format field will specify additional sub-fields than what is
        // actually present for that sample. This is because of questionably loose requirements for VCF:
        // missing sub-fields are shown as ".", except for trailing fields which can be dropped. Therefore,
        // if there are fewer format fields for a sample than expected, we will add onto the end of the
        // array the appropriate number of missing values.
        while ($num_fk > $num_ff)
        {
          array_push($format_fields,".");
          $num_ff = count($format_fields);
        }
        $geno_field_arr = array_combine($format_keys,$format_fields);
        // Now we nest the marker array with this new genotype fields array for each sample
        // Example: Marker -> Sample1 -> GT -> 0/1
        //                            -> DP -> 25
        $marker[$sample_name] = $geno_field_arr;
      } else {
        $genotype_field = [];
        $genotype_field[$marker['FORMAT']] = $marker[$sample_name];
        $marker[$sample_name] = $genotype_field;
      }

      // Explode the multi-ploidy calls
      // @TODO: Saved phased vs unphased info in metadata blob
      $genotype_calls = $marker[$sample_name]['GT'];
      if (preg_match('/\//', $genotype_calls)) {
        $marker[$sample_name]['GT'] = explode('/', $genotype_calls);
      }
      else if (preg_match('/|/', $genotype_calls)) {
        $marker[$sample_name]['GT'] = explode('|', $genotype_calls);
      }
      $genotype_calls = $marker[$sample_name]['GT'];

      // Translate the genotypic calls to be nucleotides (ie. AA, CC, etc...)
      $allele = "";
      foreach ($genotype_calls as $call) {
        // Check for a missing call
        if ($call == '.') {

        }
        $allele .= $RefAlt_alleles[$call];
        // If we end up with an allele that is longer than 2 nucleotides, we likely have an indel...
        // @TODO: Handle indels!
        if ( strlen($RefAlt_alleles[$call]) > 2 ) {
          drush_log("Found an indel for marker: " . $marker_name, 'warning');
          if (drush_confirm('The genotypes loader currently doesn\'t handle indels. Continue anyway?')) {
            continue;
          } else {
            drush_user_abort();
          }
        }
      }

      $fields = array(
        'project_id' => $project_id,
        'project_name' => $options['project_name'],
        'marker_id' => $marker_id,
        'marker_name' => $marker_name,
        'marker_type_id' => $types[ $options['marker_type'] ],
        'marker_type' => $options['marker_type'],
        'feature_type_of_marker' => $options['feature_type_of_marker'],
        'variant_id' => $variant_id,
        'variant_name' => $variant_name,
        'variant_type_id' => $types[ $options['variant_type'] ],
        'variant_type' => $options['variant_type'],
        'stock_id' => $stock_ids[$i]['sample'],
        'stock_name' => $sample_name,
        'allele' => $allele,
      );

      if (isset($options['nd_geolocation'])) $fields['nd_geolocation'] = $options['nd_geolocation'];
      $status = call_user_func('genotypes_loader_helper_add_genotypes_'.$options['storage_method'], $fields);
      if (!$status) { return FALSE; }
    }

    // Update progress
    $num_lines++;
    $progress = genotypes_loader_print_progress($num_lines, $total_lines);
    drush_print($progress);
  }
}
