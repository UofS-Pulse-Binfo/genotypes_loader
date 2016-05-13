<?php

/**
 * @file
 * Implements a drush command to load a VCF file
 */

/**
 * Implements hook_drush_command().
 */
function vcf_genotypes_drush_command() {

  $items['load-VCF'] = array(
    'description' => 'Takes a VCF file and associates it with a LIMS record to link to germplasm in KnowPulse, then imports marker, variant and genotype data into KnowPulse.',
    'aliases' => array('load-vcf'),
    /*'arguments' => array(
      'input_file' => 'The filename of the VCF file for upload',
    ),*/
  );

  return $items;
}

/**
 * Callback for load-VCF command
 */
function drush_vcf_genotypes_load_VCF() {
  drupal_set_message(t('Hello world!'));
}
