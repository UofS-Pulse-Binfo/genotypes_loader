<?php
/**
 * @file
 * Provides functionality for loading a simple marker by sample genotype matrix.
 */

/**
 * The Main function for loading a genotype matrix.
 * SHOULD BE EXECUTED VIA A TRIPAL JOB, NOT DIRECTLY!
 *
 * @param $file
 *   The absolute path of the file to be loaded.
 * @param $options
 *   An associative array of additional information. Keys include:
 *     - organism_id: the organism_id of both the marker/variants and samples/germplasm.
 *     - variant_type: the cvterm.name of the feature.type_id for the variant.
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
 * @param $types
 *  An associative array of types and their type IDs in Chado which are required by the loader,
 *  such as types of features and stocks.
 * @param $job_id
 *   The job_id of the Tripal job calling this function.
 */
function genotypes_loader_load_matrix($input_file, $options, $types, $job_id = 0) {

  // Set some variables to abstract mode for genotypes_loader_helper_add_record_with_mode().
  $select_only = 0;
  $insert_only = 1;
  $both = 2;

  // Open our matrix file
  $MATRIX_FILE = fopen($input_file, "r") or die("ERROR: Unable to open $input_file!");

  // --------------------------
  //   PROCESSING MATRIX FILE
  // --------------------------

  // Save the header.
  $header = fgetcsv($MATRIX_FILE, 0, "\t");

  // Get all the sample names in the header in order to confirm that they match what was provided
  // in the samples file
  $geno_file_sample_names = array_slice($header, 3);
  foreach ($geno_file_sample_names as $sample) {
    if (!$options['sample_list'][$sample]) {
      return drush_set_error(dt('ERROR: Sample @sample_name in your genotypes file does not match any samples provided in your samples file.', array('@sample_name' => $sample)));
    }
  }

  // Determine the number of lines in the input file.
  $total_lines = intval(exec("grep -vc '^#' " . escapeshellarg($input_file)));
  if ($total_lines) { drush_print("Number of lines to process: $total_lines"); }
  else {
    return drush_set_error(dt('ERROR: Unable to count the number of lines in file @input_file', array('@input_file' => $input_file)));
  }
  // Setting this to 1 since we're counting number of lines with SNPs + 1 header line
  $num_lines = 1;
  // Start the progress bar
  $progress = genotypes_loader_print_progress($num_lines, $total_lines);
  print($progress);

  // For each line in the file (except the header) save the genotypes to the database
  // according to the specified method.
  while(!feof($MATRIX_FILE)) {

    $current_line = fgetcsv($MATRIX_FILE, 0, "\t");

    // If the line doesn't have at least 2 columns (such as a comment or empty line) then skip it.
    if (sizeof($current_line) < 2) { continue; }
    $line_num = $num_lines+1;

    // Set up some readable variables for the info stored in the first 3 columns.
    // ----- Column 1 => Variant Name, Marker Name -----
    if ($current_line[0]) {
      $variant_name = $current_line[0];
      $marker_name = $current_line[0] . ' ' . ucwords(str_replace('_',' ',$options['marker_type']));
    } else {
      return drush_set_error(dt('ERROR: Unexpected empty value for variant name at line #@line_number', array('@line_number' => $num_lines)));
    }

    // ----- Column 2 => Backbone -----
    if ($current_line[1]) {
      $backbone_name = $current_line[1];
    } else {
      return drush_set_error(dt('ERROR: Unexpected empty value for backbone at line #@line_number', array('@line_number' => $num_lines)));
    }

    // ----- Column 3 => Position on backbone -----
    // @ASSUMPTION: SNPs only, no indels
    if ($current_line[2]) {
      $fmin = $current_line[2] - 1;
      $fmax = $current_line[2];
    } else {
      return drush_set_error(dt('ERROR: Unexpected empty value for position at line #@line_number', array('@line_number' => $num_lines)));
    }

    //print "Marker name: " . $marker_name . "\nVariant name: " . $variant_name . "\nBackbone: " . $backbone_name . "\nFmin: " . $fmin . "\nFmax: " . $fmax . "\n";

    // -------------------------
    //   INSERT MARKER/VARIANT
    // -------------------------
    // Pull out the backbone (chromosome, scaffold, etc...) and check that it exists.
    // ----- Chromosome -----
    $chromosome_id = genotypes_loader_helper_add_record_with_mode('Chromosome', 'feature', $select_only, array(
        'name' => $backbone_name,
        'uniquename' => $backbone_name,
        'organism_id' => $options['organism_id'],
    ));
    if (!$chromosome_id) { return drush_set_error(dt('ERROR: Could not find a chromosome ID for @backbone', array('@backbone' => $backbone_name))); }

    // Now create a variant and marker in Chado, and link it to a chromosome.
    // ----- Variant -----
    $variant_id = genotypes_loader_helper_add_record_with_mode('Variant', 'feature', $options['insert_variants'], array(
      'name' => $variant_name,
      'uniquename' => $variant_name,
      'organism_id' => $options['organism_id'],
      'type_id' => $types[ $options['variant_type'] ]
    ));
    if (!$variant_id) { return drush_set_error(dt('ERROR: Could not find a variant ID for @variant', array('@variant' => $variant_name))); }

    // ----- Marker -----
    $marker_id = genotypes_loader_helper_add_record_with_mode('Marker', 'feature', $options['insert_markers'], array(
      'name' => $marker_name,
      'uniquename' => $marker_name,
      'organism_id' => $options['organism_id'],
      'type_id' => $types[ $options['feature_type_of_marker'] ],
    ));
    if (!$marker_id) { return drush_set_error(dt('ERROR: Could not find a marker ID for @marker', array('@marker' => $marker_name))); }

    // ----- Marker Type -----
    $marker_type_prop_id = genotypes_loader_helper_add_record_with_mode('Marker Type', 'featureprop', $both, array(
      'feature_id' => $marker_id,
      'type_id' => $types['marker_type'],
      'value' => $options['marker_type'],
    ));
    if (!$marker_type_prop_id) { return drush_set_error(dt('ERROR: Could not find a marker type property ID for @marker_type', array('@marker_type' => $marker_name))); }

    // ----- Link Variant to Marker -----
    $status = genotypes_loader_helper_add_record_with_mode('Marker Variant Link', 'feature_relationship', $both, array(
      'subject_id' => $marker_id,
      'type_id' => $types['is_marker_of'],
      'object_id' => $variant_id,
    ));
    if (!$status) { return drush_set_error(dt('ERROR: Could not link variant @variant to marker @marker', array(
      '@variant' => $variant_name,
      '@marker' => $marker_name)));
    }

    // ----- Locate the variant on chromosome -----
    $status = genotypes_loader_helper_add_record_with_mode('Variant location on Chromsome', 'featureloc', $both, array(
      'feature_id' => $variant_id,
      'srcfeature_id' => $chromosome_id,
      'fmin' => $fmin,
      'fmax' => $fmax
    ));
    if (!$status) { return drush_set_error(dt('ERROR: Could not locate variant @variant on backbone @backbone', array(
      '@variant' => $variant_name,
      '@backbone' => $backbone_name)));
    }

    // ----- Locate the marker on chromosome -----
    $status = genotypes_loader_helper_add_record_with_mode('Marker location on Chromsome', 'featureloc', $both, array(
      'feature_id' => $marker_id,
      'srcfeature_id' => $chromosome_id,
      'fmin' => $fmin,
      'fmax' => $fmax
    ));
    if (!$status) { return drush_set_error(dt('ERROR: Could not locate marker @marker on backbone @backbone', array(
      '@marker' => $marker_name,
      '@backbone' => $backbone_name)));
    }

    // ------------------------
    //     INSERT GENOTYPES
    // ------------------------

    // Now process the genotypes.
    foreach ($current_line as $k => $allele) {

      // Skip the first 3 columns since they have to do with marker/location which
      // have already been handled above.
      if ($k <= 2) continue;

      // If $allele is empty, it means we have missing data for this genotype.
      // Thus, skip this particular genotype and move onto the next.
      // @TODO: Should also check for non-nucleotides? Something like N/A?
      if (!$allele) continue;

      // Get the source name so that we can find the unique stock name and ID stored in options
      $source_name = $header[$k];
      if (!$options['sample_list'][$source_name]) {
        return drush_set_error(dt('ERROR: Sample @source_name in your genotypes file does not match any samples provided in your samples file.', array('@source_name' => $source_name)));
      }

      // We are going to use a helper function for this portion since it depends heavily
      // on the storage method chosen. As such, compile an array of fields to pass in
      // and then call the appropriate helper function.
      $fields = array(
        'project_id' => $options['project_id'],
        'project_name' => $options['project_name'],
        'marker_id' => $marker_id,
        'marker_name' => $marker_name,
        'marker_type' => $options['marker_type'],
        'feature_type_of_marker' => $options['feature_type_of_marker'],
        'variant_id' => $variant_id,
        'variant_name' => $variant_name,
        'variant_type_id' => $types[ $options['variant_type'] ],
        'variant_type' => $options['variant_type'],
        'stock_id' => $options['sample_list'][$source_name]['sample_stock_id'],
        'stock_name' => $options['sample_list'][$source_name]['sample_name'],
        'allele' => $allele,
      );
      if (isset($options['nd_geolocation'])) $fields['nd_geolocation'] = $options['nd_geolocation'];

      $status = call_user_func('genotypes_loader_helper_add_genotypes_'.$options['storage_method'], $fields);
      // @TODO: return a useful error message
      if (!$status) { return FALSE; }
    }
    // Update progress
    $num_lines++;
    $progress = genotypes_loader_print_progress($num_lines, $total_lines);
    print($progress);
  }
  // If we got here without returning an error then we completed successfully!
  print("\n");
  drush_log("Loading Complete. " . $num_lines . " lines processed.", "success");

  // Close the file.
  fclose($MATRIX_FILE);
}
