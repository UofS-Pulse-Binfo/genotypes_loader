<?php
/**
 * @file
 * Administration of the genotype loader.
 */

/**
 * Settings Form.
 */
function genotypes_loader_admin_settings_form($form, &$form_state) {

  $form['#attached']['js'][] = drupal_get_path('module', 'genotypes_loader') . '/theme/js/adminCheckCVTerms.js';

  $form['#attached']['css'][] = array(
    'data' =>
      'img.cvterm-status-ico {
        height: 20px;
        width: 20px;
        top: 5px;
        position: relative;
        padding-left: 5px; }',
    'type' => 'inline',
  );

  $form['cvterms'] = array(
    '#type' => 'fieldset',
    '#title' => 'Controlled Vocabulary Terms',
    '#description' => 'The genotypes loader uses a number of controlled vocabulary terms (cvterms) when loading genotypes into Chado. This form will allow you to not only check that the required cvterms are present but also to change them to suit your needs.',
  );

  $stock_type_cv = tripal_get_cv(array('name' => 'stock_type'));
  $form['cvterms']['sample_type'] = array(
    '#type' => 'textfield',
    '#title' => 'Stock Type: DNA',
    '#description' => 'This cvterm is used as the stock.type_id for the DNA samples that the genotypes are directly attached to.',
    '#default_value' => variable_get('genotypes_sample_type', 'DNA'),
    '#autocomplete_path' => 'ajax/genotypes_loader/cvterm/' . $stock_type_cv->cv_id,
    '#attributes' => array('data-cv' => 'stock_type', 'data-cv_id' => $stock_type_cv->cv_id ),
  );

  $stock_rel_cv = tripal_get_cv(array('name' => 'stock_relationship'));
  $form['cvterms']['stock_rel'] = array(
    '#type' => 'textfield',
    '#title' => 'Stock Relationship: is_extracted_from',
    '#description' => 'This cvterm is used as the stock_relationship.type_id relating the DNA sample to the germplasm is was extracted from.',
    '#default_value' => variable_get('genotypes_stock_ref', 'is_extracted_from'),
    '#autocomplete_path' => 'ajax/genotypes_loader/cvterm/' . $stock_rel_cv->cv_id,
    '#attributes' => array('data-cv' => 'stock_relationship', 'data-cv_id' => $stock_rel_cv->cv_id ),
  );

  $feature_rel_cv = tripal_get_cv(array('name' => 'feature_relationship'));
  $form['cvterms']['feature_rel'] = array(
    '#type' => 'textfield',
    '#title' => 'Feature Relationship: is_marker_of',
    '#description' => 'This cvterm is used as the feature_relationship.type_id relating the marker to the variant.',
    '#default_value' => variable_get('genotypes_feature_rel', 'is_marker_of'),
    '#autocomplete_path' => 'ajax/genotypes_loader/cvterm/' . $feature_rel_cv->cv_id,
    '#attributes' => array('data-cv' => 'feature_relationship', 'data-cv_id' => $feature_rel_cv->cv_id ),
  );

  $feature_prop_cv = tripal_get_cv(array('name' => 'feature_property'));
  $form['cvterms']['featureprop_type'] = array(
    '#type' => 'textfield',
    '#title' => 'Feature Property: marker_type',
    '#description' => 'This cvterm is used as the featureprop.type_id for the property indicating the type of marker.',
    '#default_value' => variable_get('genotypes_featureprop_type', 'marker_type'),
    '#autocomplete_path' => 'ajax/genotypes_loader/cvterm/' . $feature_prop_cv->cv_id,
    '#attributes' => array('data-cv' => 'feature_property', 'data-cv_id' => $feature_prop_cv->cv_id ),
  );

  $form['cvterms']['add_cvterms'] = array(
    '#type' => 'checkbox',
    '#title' => "Add Terms if they don't exist?",
    '#description' => 'If checked, clicking save below will add terms to the appropriate controlled vocabularies.',
  );

  $form['cvterms']['save_cvterms'] = array(
    '#type' => 'submit',
    '#value' => 'Save Terms',
  );

  return $form;
}

/**
 * Settings Form: Validation.
 */
function genotypes_loader_admin_settings_form_validate($form, &$form_state) {

}

/**
 * Settings Form: Submission.
 */
function genotypes_loader_admin_settings_form_submit($form, &$form_state) {

  if ($form_state['values']['op'] == 'Save Terms') {

    // First, save the values.
    variable_set('genotypes_sample_type', $form_state['values']['sample_type']);
    variable_set('genotypes_stock_ref', $form_state['values']['stock_rel']);
    variable_set('genotypes_feature_rel', $form_state['values']['feature_rel']);
    variable_set('genotypes_featureprop_type', $form_state['values']['featureprop_type']);

    // Next, if requested, save the new cvterms.
    if ($form_state['values']['add_cvterms']) {

      foreach(array('sample_type','stock_rel','feature_rel','featureprop_type') as $element) {
        if (!empty($form_state['values'][$element])) {

          $cv_name = $form['cvterms'][$element]['#attributes']['data-cv'];
          $term = array(
            'id' => $cv_name . ':' . $form_state['values'][$element],
            'name' => $form_state['values'][$element],
            'cv_name' => $cv_name,
          );
          tripal_insert_cvterm($term);
        }
      }
    }
  }
}