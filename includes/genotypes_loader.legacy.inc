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
  $file = fopen($input_file, 'r') or die ("ERROR: Unable to open $input_file!");

  // Save the header
  $header = fgetcsv($file, 0, "\t");

  // Select variables for the helper functions.
  $select_only = 0;
  $insert_only = 1;
  $both = 2;

  // -----------------------------
  // PROCESSING LEGACY FORMAT FILE
  // -----------------------------
  $total_lines = intval(exec("grep -vc '^#' $input_file"));
  drush_print("Number of lines to process: $total_lines");
  // Setting this to 0 since we're counting number of lines with SNPs, not header lines.
  $num_lines = 0;
  // Start the progress bar
  $progress = genotypes_loader_print_progress($num_lines, $total_lines);
  print($progress);

  // For each line except for the header, process sample, variant, marker and genotype
  while(!feof($file)) {

    $current_line = fgetcsv($file, 0, "\t");
    if (empty($current_line)) continue;

    // Set up some variables to save the values in each column
    // Column 1 => Marker Name, Variant Name
    $marker_name = $current_line[0] . '_' . $options['marker_type'];
    $variant_name = $current_line[0];
    // Column 2 => Backbone
    $backbone_name = $current_line[1];
    // Column 3 => Position on backbone
    // @ASSUMPTION: SNPs only, no indels
    $fmin = $current_line[2] - 1;
    $fmax = $current_line[2];
    // Column 4 => Sample Name
    $sample_name = $current_line[3];
    // Column 5 => Genotype
    $genotype_call = $current_line[4];

    print "Marker name: " . $marker_name . "\nVariant name: " . $variant_name . "\nBackbone: " . $backbone_name . "\nFmin: " . $fmin . "\nFmax: " . $fmax . "\nSample name: " . $sample_name . "\nGenotype Call: " . $genotype_call . "\n";

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
      'type_id' => $types[ $options['feature_type_of_marker'] ]
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
    //

    print("Check for chromosome, adding of variants and markers: DONE!\n");

  }
}
