<?php
/**
 * @file
 * Provides functionality for loading a legacy file.
 */

/**
 * Parses a tab-delimited file and inserts markers, variants, and genotype calls into the database.
 *
 * @param $input_file
 *   The absolute path of the file to be parsed.
 * @param $sample_file
 *   The absolute path of a tab-delimited file specifying the samples in the input file and
 *   their associated stock name, stock accession, germplasm name and germplasm accession.
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
 */

function genotypes_loader_load_legacy($input_file = NULL, $options, $types) {

  // Open our file
  $FILE = fopen($input_file, 'r') or die ("ERROR: Unable to open $input_file!");

  // Save the header
  $header = fgetcsv($FILE, 0, "\t");

  // Select variables for the helper functions.
  $select_only = 0;
  $insert_only = 1;
  $both = 2;

  // -----------------------------
  // PROCESSING LEGACY FORMAT FILE
  // -----------------------------
  $total_lines = intval(exec("grep -vc '^#' $input_file"));
  drush_print("Number of lines to process: $total_lines");
  // Setting this to 1 since we're counting number of lines with SNPs + 1 header line
  $num_lines = 1;
  // Start the progress bar
  $progress = genotypes_loader_print_progress($num_lines, $total_lines);
  print($progress);

  // For each line except for the header, process sample, variant, marker and genotype
  while(!feof($FILE)) {

    $current_line = fgetcsv($FILE, 0, "\t");
    // If the line doesn't have at least 2 columns (such as a comment or empty line) then skip it.
    if (empty($current_line)) continue;

    // Set up some variables to save the values in each column
    // Column 1 => Variant Name, Marker Name
    $variant_name = $current_line[0];
    $marker_name = $current_line[0] . ' ' . ucwords(str_replace('_',' ',$options['marker_type']));
    // Column 2 => Backbone
    $backbone_name = $current_line[1];
    // Column 3 => Position on backbone
    // @ASSUMPTION: SNPs only, no indels
    $fmin = $current_line[2] - 1;
    $fmax = $current_line[2];
    // Column 4 => DNA source
    $source_name = $current_line[3];
    // Column 5 => Genotype
    $allele = $current_line[4];

    //print "Marker name: " . $marker_name . "\nVariant name: " . $variant_name . "\nBackbone: " . $backbone_name . "\nFmin: " . $fmin . "\nFmax: " . $fmax . "\nSample name: " . $source_name . "\nGenotype Call: " . $allele . "\n";

    // ------------------------
    // INSERT MARKER/VARIANT
    // ------------------------
    // Pull out the backbone (chromosome, scaffold, etc...) and check that it exists.
    // ----- Chromosome -----
    $chromosome_id = genotypes_loader_helper_add_record_with_mode('Chromosome', 'feature', $select_only, array(
        'name' => $backbone_name,
        'uniquename' => $backbone_name,
        'organism_id' => $options['organism_id'],
    ));
    if (!$chromosome_id) { return FALSE; }

    // Now create a variant and marker in Chado, and link it to a chromosome.
    // ----- Variant -----
    $variant_id = genotypes_loader_helper_add_record_with_mode('Variant', 'feature', $options['insert_variants'], array(
      'name' => $variant_name,
      'uniquename' => $variant_name,
      'organism_id' => $options['organism_id'],
      'type_id' => $types[ $options['variant_type'] ]
    ));
    if (!$variant_id) { return FALSE; }

    // ----- Marker -----
    $marker_id = genotypes_loader_helper_add_record_with_mode('Marker', 'feature', $options['insert_markers'], array(
      'name' => $marker_name,
      'uniquename' => $marker_name,
      'organism_id' => $options['organism_id'],
      'type_id' => $types[ $options['feature_type_of_marker'] ],
    ));
    if (!$marker_id) { return FALSE; }

    // ----- Marker Type -----
    $marker_type_prop_id = genotypes_loader_helper_add_record_with_mode('Marker Type', 'featureprop', $both, array(
      'feature_id' => $marker_id,
      'type_id' => $types['marker_type'],
      'value' => $options['marker_type'],
    ));
    if (!$marker_type_prop_id) { return FALSE; }

    // ----- Link Variant to Marker -----
    $status = genotypes_loader_helper_add_record_with_mode('Marker Variant Link', 'feature_relationship', $both, array(
      'subject_id' => $marker_id,
      'type_id' => $types['is_marker_of'],
      'object_id' => $variant_id,
    ));
    if (!$status) { return FALSE; }

    // ----- Locate the variant on chromosome -----
    $status = genotypes_loader_helper_add_record_with_mode('Variant location on Chromsome', 'featureloc', $both, array(
      'feature_id' => $variant_id,
      'srcfeature_id' => $chromosome_id,
      'fmin' => $fmin,
      'fmax' => $fmax,
    ));
    if (!$status) { return FALSE; }

    // ----- Locate the marker on chromosome -----
    $status = genotypes_loader_helper_add_record_with_mode('Marker location on Chromsome', 'featureloc', $both, array(
      'feature_id' => $marker_id,
      'srcfeature_id' => $chromosome_id,
      'fmin' => $fmin,
      'fmax' => $fmax,
    ));
    if (!$status) { return FALSE; }

    // ------------------------
    // INSERT GENOTYPES
    // ------------------------

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
    if (!$status) { return FALSE; }

    // Update progress
    $num_lines++;
    $progress = genotypes_loader_print_progress($num_lines, $total_lines);
    print($progress);
  }
  // If we got here without returning an error then we completed successfully!
  drush_log("Loading Complete.", "success");

  // Close the file.
  fclose($FILE);
}
