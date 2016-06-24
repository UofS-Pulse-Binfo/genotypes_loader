<?php
/**
 * @file
 * Provides functionaltiy for loading a simple marker by sample genotype matrix.
 */

/**
 * Provides an interface for loading data.
 */
function nd_genotypes_submit_matrix_loading_job_form($form, $form_state) {

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
function nd_genotypes_submit_matrix_loading_job_form_submit($form, $form_state) {
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
function nd_genotypes_load_genotype_matrix($file, $options, $job_id = 0) {

  // Start our transaction.
  $transaction = db_transaction();

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

  // Display the options to the user.
  $storage_method = array(
    'nd_exp' => 'Natural Diversity Experiment',
    'genotype_call' => 'Genotype Call Custom Table',
    'stock_genotype' => 'Genotype Stock table',
    'custom' => 'Custom',
  );
  drush_print("\n" . 'Loading your data according to the "'.$storage_method[ $options['storage_method'] ].'" Method.');

  drush_print("File: ".$file);
  drush_print("Options:");
  drush_print("\tProject=".$options['project_name'].' ('.$project_id.').');
  drush_print("\tOrganism ID=".$organism_name.' ('.$options['organism_id'].').');
  drush_print("\tVariant Type=".$options['variant_type'].'.');
  drush_print("\tMarker Type=".$options['marker_type'].'.');

  if (isset($options['nd_geolocation'])) {
    drush_print("\tGeolocation=".$options['nd_geolocation'].'.');
  }

  $db_options = array(
    1 => 'Insert Only',
    0 => 'Select Only',
    2 => 'Insert And Select',
  );
  drush_print("\tSamples=".$db_options[ $options['insert_samples'] ].'.');
  drush_print("\tGermplasm=".$db_options[ $options['insert_germplasm'] ].'.');
  drush_print("\tVariants=".$db_options[ $options['insert_variants'] ].'.');
  drush_print("\tMarkers=".$db_options[ $options['insert_markers'] ].'.');

  drush_print(" ");

  // Check the custom table is there if using Method #2: Genotype Call
  if ($options['storage_method'] == 'genotype_call') {
    chado_create_custom_table(
      'genotype_call',
      nd_genotypes_genotype_call_schema_template()
    );
  }

  // Validate the file
  $file = trim($file);
  if (!file_exists($file)) {
    return drush_set_error(t("Cannot find the file on the system. Check that the file exists or that the web server has permissions to read the file."));
  }

  // Open the file.
  $IN = fopen($DRUPAL_ROOT . '/' . $file, "r") or die("Unable to open file!");

  // Save the header.
  $header = fgetcsv($IN, 0, "\t");

  // First look up all the Samples/Germplasm.
  // If they don't already exist and insert samples/germplasm is TRUE
  // (ie: 1= INSERT ONLY AND 2=INSERT & SELECT) then create them first.
  drush_print("Ensuring all samples exist...");
  $stock_ids = array();
  foreach ($header as $k => $name) {
    if ($k <= 2) continue;

    // Sample.
    $stock_id = nd_genotypes_helper_add_record_with_mode('Sample', 'stock', $options['insert_samples'], array(
      'name' => $name,
      'uniquename' => $name,
      'organism_id' => $options['organism_id'],
      'type_id' => array('name' => 'DNA')
    ));
    if ($stock_id) { $stock_ids[$k]['sample'] = $stock_id; }
    else { return FALSE; }

    // Germplasm.
    $germplasm_id = nd_genotypes_helper_add_record_with_mode('Germplasm', 'stock', $options['insert_germplasm'], array(
      'name' => $name,
      'uniquename' => $name,
      'organism_id' => $options['organism_id'],
      'type_id' => array('name' => 'Accession')
    ));
    if ($germplasm_id) { $stock_ids[$k]['germplasm'] = $stock_id; }
    else { return FALSE; }

    // Now Link them.
    $status = nd_genotypes_helper_add_record_with_mode('Germplasm to Sample Link', 'stock_relationship', 2, array(
      'subject_id' => $stock_id,
      'type_id' => array('name' => 'is_extracted_from'),
      'object_id' => $germplasm_id,
    ));
    if (!$status) { return FALSE; }
  }
  drush_log("All stocks now exist.","ok");

  // For each line in the file (except the header) save the genotypes to the database
  // according to the specified method.
  drush_print("\nNow processing each line of the file for variants, markers and genotypes...");
  while(!feof($IN)) {
    $line = fgetcsv($IN, 0, "\t");

    // If the line is empty then skip it.
    if (sizeof($line) < 2) { continue; }

    // Set up some readable variables for the info stored in the first 3 columns.
    $variant_name = $line[0];
    $marker_name = $line[0] . ' ' . ucwords(str_replace('_',' ',$options['marker_type']));
    $backbone_name = $line[1];
    $fmin = $line[2]-1;
    $fmax = $line[2];

    // Set some variables to abstract mode for nd_genotypes_helper_add_record_with_mode().
    $select_only = 0;
    $insert_only = 1;
    $both = 2;

    //-----------------------
    // First create the marker/variant/chromosome combination.
    // This is the same for all storage methods.

    // Variant.
    $variant_id = nd_genotypes_helper_add_record_with_mode('Variant', 'feature', $options['insert_variants'], array(
      'name' => $variant_name,
      'uniquename' => $variant_name,
      'organism_id' => $options['organism_id'],
      'type_id' => array('name' => $options['variant_type'])
    ));
    if (!$variant_id) { $transaction->rollback(); return FALSE; }

    // Marker.
    $marker_id = nd_genotypes_helper_add_record_with_mode('Marker', 'feature', $options['insert_markers'], array(
      'name' => $marker_name,
      'uniquename' => $marker_name,
      'organism_id' => $options['organism_id'],
      'type_id' => array('name' => $options['marker_type'])
    ));
    if (!$marker_id) { $transaction->rollback(); return FALSE; }

    // Link Variant to marker.
    $status = nd_genotypes_helper_add_record_with_mode('Marker Variant Link', 'feature_relationship', $both, array(
      'subject_id' => $marker_id,
      'type_id' => array('name' => 'is_marker_of'),
      'object_id' => $variant_id,
    ));
    if (!$status) { $transaction->rollback(); return FALSE; }

    // Locate the variant on chromosome.
    $status = nd_genotypes_helper_add_record_with_mode('Variant location on Chromsome', 'featureloc', $both, array(
      'feature_id' => $variant_id,
      'srcfeature_id' => array('name' => $backbone_name),
      'fmin' => $fmin,
      'fmax' => $fmax
    ));
    if (!$status) { $transaction->rollback(); return FALSE; }

    // Locate the marker on chromosome.
    $status = nd_genotypes_helper_add_record_with_mode('Marker location on Chromsome', 'featureloc', $both, array(
      'feature_id' => $marker_id,
      'srcfeature_id' => array('name' => $backbone_name),
      'fmin' => $fmin,
      'fmax' => $fmax
    ));
    if (!$status) { $transaction->rollback(); return FALSE; }

    //-----------------------
    // Now process the genotypes.
    foreach ($line as $k => $allele) {

      // Skip the first 3 columns since they have to do with marker/location which
      // have already been handled above.
      if ($k <= 2) continue;

      // We are going to use a helper function for this portion since it depends heavily
      // on the storage method chosen. As such, compile an array of fields to pass in
      // and then call the appropriate helper function.
      $fields = array(
        'project_id' => $project_id,
        'project_name' => $options['project_name'],
        'marker_id' => $marker_id,
        'marker_name' => $marker_name,
        'marker_type' => $options['marker_type'],
        'variant_id' => $variant_id,
        'variant_name' => $variant_name,
        'variant_type' => $options['variant_type'],
        'stock_id' => $stock_ids[$k]['sample'],
        'stock_name' => $header[$k],
        'allele' => $allele,
      );
      if (isset($options['nd_geolocation'])) $fields['nd_geolocation'] = $options['nd_geolocation'];
      $status = call_user_func('nd_genotypes_helper_add_genotypes_'.$options['storage_method'], $fields);
      if (!$status) { $transaction->rollback(); return FALSE; }
    }
  }

  // If we got here without returning an error then we completed successfully!
  drush_log("Loading Complete.", "success");

  // Close the file.
  fclose($IN);
}

/**
 * Adds/Selects a record based on the mode. Helper function for loading genotype matrix
 * that does the select/insert based on mode logic.
 *
 * @param $record_type
 *   A human-readable term for your record for error messages.
 * @param $table
 *   The chado table to select/insert from/into.
 * @param $mode
 *   One of 0: Select Only, 1: Insert Only, 2: Insert & Select.
 * @param $values
 *   An array of [table column] => [value] mappings. These will be used for both the
 *   select and the insert.
 * @return
 *   The value of the primary key for the inserted/selected record or FALSE if failed.
 */
function nd_genotypes_helper_add_record_with_mode($record_type, $table, $mode, $values) {

  // the name of the primary key.
  $pkey = $table . '_id';

  // First we select the record to see fi it already exists.
  $record = chado_select_record($table, array($pkey), $values);

  // If it exists and the mode is 1 (Insert Only) then return an error to drush.
  if ($record) {
    if ($mode == 1) {
      return drush_set_error(dt('Record "@record_type" already exists but you chose to only insert (mode=@mode). Values: @values', array('@record_type' => $record_type, '@mode' => $mode, '@values' => print_r($values,TRUE))));
    }
    // Otherwise the mode allows select so return the value of the primary key.
    else {
      return $record[0]->{$pkey};
    }
  }
  // If there is no pre-existing sample but we've been given permission to create it,
  // then we will insert it :-).
  elseif ($mode) {
    $record = chado_insert_record($table, $values);
    return $record[$pkey];
  }
  // If there is no pre-existing record and we are not allowed to create one
  // then return an error.
  else {
    return drush_set_error(dt('Record "@record_type" doesn\'t already exist but you chose to only select (mode=@mode). Values: @values', array('@record_type' => $record_type, '@mode' => $mode, '@values' => print_r($values,TRUE))));
  }

}

/**
 * Adds genotype records when using the ND Experiment Tables storage method. This is a
 * helper function for the genotype matrix loader.
 *
 * This function handles all the tables beyond the feature block and up to but not
 * including stock and project.
 * See https://github.com/UofS-Pulse-Binfo/nd_genotypes/wiki/How-to-Store-your-Data#method-1-the-chado-natural-diversity-experiment-tables
 *
 * @param $v
 *   An array of values to use when inserting/selecting. Keys include:
 *    - project_id: the project_id of the project.
 *    - project_name: the name of the project.
 *    - marker_id: the feature_id of the marker.
 *    - marker_name: the name of the marker.
 *    - marker_type: the cvterm.name of the feature.type_id for the variant.
 *    - variant_id: the feature_id of the variant.
 *    - variant_name: the name of the variant
 *    - variant_type: the cvterm.name of the feature.type_id for the variant.
 *    - stock_id: the stock_id of the sample.
 *    - stock_name: the name of the sample.
 *    - allele: the allele call.
 * @return
 *   TRUE if successful and FALSE otherwise.
 */
function nd_genotypes_helper_add_genotypes_nd_exp($v) {

  // Set some variables to abstract mode for nd_genotypes_helper_add_record_with_mode().
  $select_only = 0;
  $insert_only = 1;
  $both = 2;

  // Genotype.
  $genotype_id = nd_genotypes_helper_add_record_with_mode('Genotype', 'genotype', $both, array(
    'name' => $v['marker_name'] . ': ' . $v['allele'],
    'uniquename' => str_replace(' ','_',$v['marker_name']) . '_' . $v['allele'],
    'description' => $v['allele'],
    'type_id' => array('name' => $v['variant_type']),
  ));
  if (!$genotype_id) return FALSE;

  // Feature <=> Genotype Link.
  $feature_genotype_id = nd_genotypes_helper_add_record_with_mode('Marker Genotype Link', 'feature_genotype', $both, array(
    'feature_id' => $v['marker_id'],
    'genotype_id' => $genotype_id,
    'cvterm_id' => array('name' => $v['variant_type']),
    'rank' => 0,
    'cgroup' => 0,
  ));
  if (!$feature_genotype_id) return FALSE;

  // ND Experiment.
  // Has to be done without nd_genotypes_helper_add_record_with_mode() since we need to
  // avoid selecting first. There is no unique key on this table, we just want to create
  // a record in all cases.
  $ndexp = chado_insert_record('nd_experiment', array(
    'nd_geolocation_id' => array('description' => $v['nd_geolocation']),
    'type_id' => array('name' => $v['variant_type']),
  ));
  if ($ndexp) $ndexp_id = $ndexp['nd_experiment_id'];
  if (!$ndexp) {
    return drush_set_error(dt('Unable to create ND Experiment record.'));
  }

  // ND Experiment <=> Genotype.
  $status = nd_genotypes_helper_add_record_with_mode('ND Experiment Genotype Link', 'nd_experiment_genotype', $insert_only, array(
    'nd_experiment_id' => $ndexp_id,
    'genotype_id' => $genotype_id,
  ));
  if (!$status) return FALSE;

  // ND Experiment <=> Stock.
  $status = nd_genotypes_helper_add_record_with_mode('ND Experiment Sample Link', 'nd_experiment_stock', $insert_only, array(
    'nd_experiment_id' => $ndexp_id,
    'stock_id' => $v['stock_id'],
    'type_id' => array('name' => $v['variant_type']),
  ));
  if (!$status) return FALSE;

  // ND Experiment <=> Project.
  $status = nd_genotypes_helper_add_record_with_mode('ND Experiment Project Link', 'nd_experiment_project', $insert_only, array(
    'nd_experiment_id' => $ndexp_id,
    'project_id' => $v['project_id'],
  ));
  if (!$status) return FALSE;

  return TRUE;
}

/**
 * Adds genotype records when using the Genotype Call storage method. This is a
 * helper function for the genotype matrix loader.
 *
 * This function handles all the tables beyond the feature block and up to but not
 * including stock and project.
 * See https://github.com/UofS-Pulse-Binfo/nd_genotypes/wiki/How-to-Store-your-Data#method-2-custom-genotype-call-table
 *
 * @param $v
 *   An array of values to use when inserting/selecting. Keys include:
 *    - project_id: the project_id of the project.
 *    - project_name: the name of the project.
 *    - marker_id: the feature_id of the marker.
 *    - marker_name: the name of the marker.
 *    - marker_type: the cvterm.name of the feature.type_id for the variant.
 *    - variant_id: the feature_id of the variant.
 *    - variant_name: the name of the variant
 *    - variant_type: the cvterm.name of the feature.type_id for the variant.
 *    - stock_id: the stock_id of the sample.
 *    - stock_name: the name of the sample.
 *    - allele: the allele call.
 * @return
 *   TRUE if successful and FALSE otherwise.
 */
function nd_genotypes_helper_add_genotypes_genotype_call($v) {

  // Set some variables to abstract mode for nd_genotypes_helper_add_record_with_mode().
  $select_only = 0;
  $insert_only = 1;
  $both = 2;

   // Genotype.
  $genotype_id = nd_genotypes_helper_add_record_with_mode('Genotype', 'genotype', $both, array(
    'name' => $v['allele'],
    'uniquename' => $v['allele'],
    'description' => $v['allele'],
    'type_id' => array('name' => $v['variant_type']),
  ));
  if (!$genotype_id) return FALSE;

  // Genotype Call
  $genotype_call_id = nd_genotypes_helper_add_record_with_mode('Genotype Call', 'genotype_call', $both, array(
    'variant_id' => $v['variant_id'],
    'marker_id' => $v['marker_id'],
    'genotype_id' => $genotype_id,
    'project_id' => $v['project_id'],
    'stock_id' => $v['stock_id'],
  ));
  if (!$genotype_call_id) return FALSE;

  return TRUE;
}

/**
 * Adds genotype records when using the Genotype Stock storage method. This is a
 * helper function for the genotype matrix loader.
 *
 * This function handles all the tables beyond the feature block and up to but not
 * including stock and project.
 * See https://github.com/UofS-Pulse-Binfo/nd_genotypes/wiki/How-to-Store-your-Data#method-3-via-stock-genotype-table
 *
 * @param $v
 *   An array of values to use when inserting/selecting. Keys include:
 *    - project_id: the project_id of the project.
 *    - project_name: the name of the project.
 *    - marker_id: the feature_id of the marker.
 *    - marker_name: the name of the marker.
 *    - marker_type: the cvterm.name of the feature.type_id for the variant.
 *    - variant_id: the feature_id of the variant.
 *    - variant_name: the name of the variant
 *    - variant_type: the cvterm.name of the feature.type_id for the variant.
 *    - stock_id: the stock_id of the sample.
 *    - stock_name: the name of the sample.
 *    - allele: the allele call.
 * @return
 *   TRUE if successful and FALSE otherwise.
 */
function nd_genotypes_helper_add_genotypes_stock_genotype($v) {

  // Set some variables to abstract mode for nd_genotypes_helper_add_record_with_mode().
  $select_only = 0;
  $insert_only = 1;
  $both = 2;

  // Genotype.
  $genotype_id = nd_genotypes_helper_add_record_with_mode('Genotype', 'genotype', $both, array(
    'name' => $v['marker_name'] . ': ' . $v['allele'],
    'uniquename' => str_replace(' ','_',$v['marker_name']) . '_' . $v['allele'],
    'description' => $v['allele'],
    'type_id' => array('name' => $v['variant_type']),
  ));
  if (!$genotype_id) return FALSE;

  // Feature <=> Genotype Link.
  $feature_genotype_id = nd_genotypes_helper_add_record_with_mode('Marker Genotype Link', 'feature_genotype', $both, array(
    'feature_id' => $v['marker_id'],
    'genotype_id' => $genotype_id,
    'cvterm_id' => array('name' => $v['variant_type']),
    'rank' => 0,
    'cgroup' => 0,
  ));
  if (!$feature_genotype_id) return FALSE;

  // Allele <=> Stock.
  $status = nd_genotypes_helper_add_record_with_mode('Genotype Sample Link', 'stock_genotype', $both, array(
    'genotype_id' => $genotype_id,
    'stock_id' => $v['stock_id'],
  ));
  if (!$status) return FALSE;

  return TRUE;
}
