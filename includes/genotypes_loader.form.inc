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
