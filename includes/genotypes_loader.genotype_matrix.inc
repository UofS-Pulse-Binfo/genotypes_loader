<?php
/**
 * @file
 * Provides functionality for loading a simple marker by sample genotype matrix.
 */

/**
 * Provides an interface for loading data.
 */
function genotypes_loader_submit_loading_job_form($form, $form_state) {

  $storage_method = variable_get('nd_genotype_storage_method', 'nd_exp');

  // File to be loaded.
  // @todo: provide more information about format.
  $form['file'] = array(
    '#type' => 'textfield',
    '#title' => t('Genotype Matrix File (including absolute path)'),
    '#description' => 'Please enter the full system path for the genotype matrix file.
      The path must be accessible to the server on which this Drupal instance is running.',
    '#required' => TRUE,
  );

  $form['type_org'] = array(
    '#type' => 'fieldset',
    '#title' => 'Types and Organisms',
  );

  // Organism.
  // @todo: handle different organism for germplasm.
  $sql = "SELECT * FROM {organism} ORDER BY genus, species";
  $org_rset = chado_query($sql);
  $organisms = array();
  $organisms[''] = '';
  while ($organism = $org_rset->fetchObject()) {
    $organisms[$organism->organism_id] = "$organism->genus $organism->species ($organism->common_name)";
  }
  $form['type_org']['organism_id'] = array(
    '#title' => t('Organism'),
    '#type' => t('select'),
    '#description' => t("Choose the organism to which both the samples and markers the
      genotypes are associated with."),
    '#required' => TRUE,
    '#options' => $organisms
  );

  // Variant Type.
  $values = array('name' => 'sequence');
  $cv = chado_select_record('cv', array('cv_id'), $values);
  $cv_id = $cv[0]->cv_id;
  $form['type_org']['variant_type'] = array(
    '#type' => 'textfield',
    '#title' => t('Variant Type'),
    '#required' => TRUE,
    '#description' => t('Please enter the Sequence Ontology (SO) term name that describes
      the type of variants in the file (e.g. SNP, MNP, indel).'),
    '#autocomplete_path' => "admin/tripal/chado/tripal_cv/cvterm/auto_name/$cv_id"
  );

  // Marker Type.
  // @todo: assuming this is a sequence ontology term which it likely isn't...
  $values = array('name' => 'sequence');
  $cv = chado_select_record('cv', array('cv_id'), $values);
  $cv_id = $cv[0]->cv_id;
  $form['type_org']['marker_type'] = array(
    '#type' => 'textfield',
    '#title' => t('Marker Type'),
    '#required' => TRUE,
    '#description' => t('Please enter the Sequence Ontology (SO) term name that describes
      the marker technology used to generate the genotypes in the file
        (e.g. "Exome Capture", "GBS", "KASPar", etc.).'),
    '#autocomplete_path' => "admin/tripal/chado/tripal_cv/cvterm/auto_name/$cv_id"
  );

  $form['meta'] = array(
    '#type' => 'fieldset',
    '#title' => 'Grouping and Meta-data',
  );

  // Project.
  // @todo autocomplete.
  $form['meta']['project_name'] = array(
    '#type' => 'textfield',
    '#title' => 'Project',
    '#required' => TRUE,
    '#description' => 'All genotypes will be grouped via a project to allow users to specify a particular dataset.',
  );

  // If the ND Experiment storage method is chosen then we need additional information.
  if ($storage_method == 'nd_exp') {

    // Geolocation.
    // @todo add autocomplete.
    $form['meta']['nd_geolocation'] = array(
      '#type' => 'textfield',
      '#title' => 'ND Geolocation',
      '#required' => TRUE,
      '#description' => 'A meaningful location associated with this natural diversity
        experiment. For example, this could be the location the assay was completed in,
        the location the germplasm collection was from, or the location the markers
        were developed at. <strong>This should be the description field of your nd_geolocation.</strong>',
    );
  }

  $form['db'] = array(
    '#type' => 'fieldset',
    '#title' => 'Database-Specific',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  // Storage Method.
  // Note: can't be changed.
  // @todo point them to where it can be changed.
  $form['db']['storage_method'] = array(
    '#type' => 'select',
    '#title' => t('Storage Method'),
    '#description' => 'This is the method you are using to store your genotypic data. You can change it on the "Settings" tab.',
    '#options' => array(
      'nd_exp' => 'Natural Diversity Experiment',
      'genotype_call' => 'Genotype Call Custom Table',
      'stock_genotype' => 'Genotype Stock table',
      'custom' => 'Custom',
    ),
    '#default_value' => $storage_method,
    '#disabled' => TRUE,
  );

  // Insert/Select options & help.
  $help = 'Select how %type_singular in the Genotype Matrix file are handled. Select "Insert only"
    to insert new %type_plural. If a %type_singular already exists with the same name or
    unique name, organism and type then it is skipped. Select "Select only" to only select %type_plural
    that already exist in the database. Select "Insert and Select" to insert %type_plural
    that do not exist and select those that do.';
  $options = array(
    1 => 'Insert Only',
    0 => 'Select Only',
    2 => 'Insert And Select',
  );

  // Insert Samples?
  $form['db']['insert_samples'] = array(
    '#type' => 'radios',
    '#title' => 'Samples',
    '#description' => format_string($help, array('%type_singular' => 'sample', '%type_plural' => 'samples')),
    '#options' => $options,
    '#default_value' => 2,
  );

  // Insert Germplasm?
  $form['db']['insert_germplasm'] = array(
    '#type' => 'radios',
    '#title' => 'Germplasm',
    '#description' => format_string($help, array('%type_singular' => 'germplasm', '%type_plural' => 'germplasm')),
    '#options' => $options,
    '#default_value' => 0,
  );

  // Insert Variants?
  $form['db']['insert_variants'] = array(
    '#type' => 'radios',
    '#title' => 'Variants',
    '#description' => format_string($help, array('%type_singular' => 'variant', '%type_plural' => 'variants')),
    '#options' => $options,
    '#default_value' => 2,
  );

  // Insert Markers?
  $form['db']['insert_markers'] = array(
    '#type' => 'radios',
    '#title' => 'Markers',
    '#description' => format_string($help, array('%type_singular' => 'marker', '%type_plural' => 'markers')),
    '#options' => $options,
    '#default_value' => 2,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Import Genotype Matrix',
  );

  return $form;
}

/**
 * SUBMIT: Provides an interface for loading data.
 */
function genotypes_loader_submit_loading_job_form_submit($form, $form_state) {
  global $user;

  // Simply submit a tripal job :-).
  $args = array(
    'file' => $form_state['values']['file'],
    'options' => $form_state['values'],
  );
  tripal_add_job(
    'Import Genotype Matrix: ' . $form_state['values']['file'],
    'nd_genotypes',
    'nd_genotypes_load_genotype_matrix',
    $args, $user->uid
  );

}

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

  // Save the header.
  $header = fgetcsv($MATRIX_FILE, 0, "\t");

  // -------------------------
  // PROCESSING MATRIX FILE
  // -------------------------
  // Determine the number of lines in the input file.
  $total_lines = intval(exec("wc -l '$input_file'"));
  drush_print("Number of lines to process: " . $total_lines);
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

    // Set up some readable variables for the info stored in the first 3 columns.
    // Column 1 => Variant Name, Marker Name
    $variant_name = $current_line[0];
    $marker_name = $current_line[0] . ' ' . ucwords(str_replace('_',' ',$options['marker_type']));
    // Column 2 => Backbone
    $backbone_name = $current_line[1];
    // Column 3 => Position on backbone
    // @ASSUMPTION: SNPs only, no indels
    $fmin = $current_line[2]-1;
    $fmax = $current_line[2];

    //print "Marker name: " . $marker_name . "\nVariant name: " . $variant_name . "\nBackbone: " . $backbone_name . "\nFmin: " . $fmin . "\nFmax: " . $fmax . "\n";

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
      'fmax' => $fmax
    ));
    if (!$status) { return FALSE; }

    // ----- Locate the marker on chromosome -----
    $status = genotypes_loader_helper_add_record_with_mode('Marker location on Chromsome', 'featureloc', $both, array(
      'feature_id' => $marker_id,
      'srcfeature_id' => $chromosome_id,
      'fmin' => $fmin,
      'fmax' => $fmax
    ));
    if (!$status) { return FALSE; }

    // -----------------------
    // INSERT GENOTYPES
    // -----------------------
    // Now process the genotypes.
    foreach ($current_line as $k => $allele) {

      // Skip the first 3 columns since they have to do with marker/location which
      // have already been handled above.
      if ($k <= 2) continue;

      // Get the source name so that we can find the unique stock name and ID
      $source_name = $header[$k];

      //print "Inserting genotype " . $allele . " for sample " . $source_name . "\n";

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
