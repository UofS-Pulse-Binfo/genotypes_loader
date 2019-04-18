<?php
namespace Tests;

use StatonLab\TripalTestSuite\DBTransaction;
use StatonLab\TripalTestSuite\TripalTestCase;
use Faker\Factory;

class dataIntegrityTest extends TripalTestCase {
  // Uncomment to auto start and rollback db transactions per test method.
  // use DBTransaction;

  /**
   * Tests loading of a VCF file.
   */
  public function testLoadVCF() {
    $faker = Factory::create();
    module_load_include('inc','genotypes_loader','genotypes_loader.drush');

    // Create fake organism for markers/variants.
    $organism = factory('chado.organism')->create();
    // Create cats for germplasm: must match samples file (Felis catus and Felis silvestris).
    factory('chado.organism')->create(['genus' => 'Felis', 'species' => 'catus']);
    factory('chado.organism')->create(['genus' => 'Felis', 'species' => 'silvestris']);


    // Create fake marker/variant types.
    $cv = chado_get_cv(['name' => 'sequence']);
    $marker_type = factory('chado.cvterm')->create(['cv_id' => $cv->cv_id]);
    $variant_type = factory('chado.cvterm')->create(['cv_id' => $cv->cv_id]);
    $genotype_type = factory('chado.cvterm')->create(['cv_id' => $cv->cv_id]);

    // Ensure germplasm type exists (from file).
    $cv = chado_get_cv(['name' => 'stock_type']);
    $germplasm_type = tripal_insert_cvterm([
      'id' => 'tripal:Individual',
      'name' => 'Individual',
      'cv_name' => 'stock_type'
    ]);

    // Ensure cvterms at admin/tripal/extension/genotypes_loader
    // have been inserted and configured.
    $settings = [
      'genotypes_sample_type' => 'sequence',
      'genotypes_stock_ref' => 'stock_relationship',
      'genotypes_feature_rel' => 'feature_relationship',
      'genotypes_featureprop_type_marker' => 'schema',
      'genotypes_featureprop_type_variant' => 'schema',
    ];
    foreach ($settings as $var_name => $cv_name) {
      $cv = chado_get_cv(['name' => $cv_name]);
      $term = factory('chado.cvterm')->create(['cv_id' => $cv->cv_id]);
      variable_set($var_name, $term->name);
    }

    // Create fake project.
    $project = factory('chado.project')->create();

    // Add chromosomes needed for the VCF file.
    factory('chado.feature')->create([
      'name'=>'1A', 
      'uniquename'=>'1A',
      'organism_id' => $organism->organism_id
    ]);

    $module_path = drupal_get_path('module','genotypes_loader');
    $samples_file = DRUPAL_ROOT . '/' . $module_path . '/sample_files/cats.list';
    $vcf_file = DRUPAL_ROOT . '/' . $module_path . '/sample_files/cats.vcf';
    $options = array(
      'organism_id' => $organism->organism_id,
      'feature_type_of_variant' => $variant_type->name,
      'variant_type' => $faker->words(2, TRUE),
      'feature_type_of_marker' => $marker_type->name,
      'marker_type' => $faker->words(3,TRUE),
      'project_name' => $project->name,
      'genotype_type' => $genotype_type->name,
      'storage_method' => 'genotype_call',
      // 1 = Insert Only; 0 = Select Only, 2 = Insert or Select.
      'insert_samples' => 2,
      'insert_germplasm' => 2,
      'insert_markers' => 2,
      'insert_variants' => 2,
      'silent' => TRUE,
      'no_drush' => TRUE,
    );
    $success = genotypes_loader_load_genotypes($vcf_file, $samples_file, $options);

    $this->assertTrue($success,
      "Loading genotypes failed.");
  }
}
