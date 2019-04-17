<?php
namespace Tests;

use StatonLab\TripalTestSuite\DBTransaction;
use StatonLab\TripalTestSuite\TripalTestCase;
use Faker\Factory;

class dataIntegrityTest extends TripalTestCase {
  // Uncomment to auto start and rollback db transactions per test method.
  use DBTransaction;

  /**
   * Tests loading of a VCF file.
   */
  public function testLoadVCF() {
    $faker = Factory::create();
    module_load_include('inc','genotypes_loader','genotypes_loader.drush');

    $organism = factory('chado.organism');

    $cv = chado_get_cv(['name' => 'sequence']);
    $marker_type = factory('chado.cvterm')->create(['cv_id' => $cv->cv_id]);
    $variant_type = factory('chado.cvterm')->create(['cv_id' => $cv->cv_id]);

    // Ensure germplasm type exists.
    $cv = chado_get_cv(['name' => 'stock_type']);
    print_r($cv);
    //$germplasm_type = factory('chado.cvterm')->create(['name' => 'Individual', 'cv_id' => $cv->cv_id]);
    //print_r($germplasm_type);

    $germplasm_type = tripal_insert_cvterm(array( 'id' => 'stock_type' . ':' . 'Individual', 'name' => 'Individual', 'cv_name' => 'stock_type', ));
    print_r($germplasm_type);

    $project = factory('chado.project');

    $module_path = drupal_get_path('module','genotypes_loader');
    $samples_file = $module_path . '/sample_files/cats.list';
    $vcf_file = $module_path . '/sample_files/cats.vcf';
    $options = array(
      'organism_id' => $organism->organism_id,
      'feature_type_of_variant' => $variant_type->name, // @when-fixed
      'variant_type' => $variant_type->name, // @when-fixed $faker->words(2, TRUE),
      'feature_type_of_marker' => $marker_type->name,
      'marker_type' => $faker->words(3,TRUE),
      'project_name' => $project->name,
      'storage_method' => 'genotype_call',
      // 1 = Insert Only; 0 = Select Only, 2 = Insert or Select.
      'insert_samples' => 2,
      'insert_germplasm' => 2,
      'insert_markers' => 2,
      'insert_variants' => 2,
      'silent' => TRUE,
      'no_drush' => TRUE,
    );
    $success = genotypes_loader_load_genotypes($input_file, $sample_file, $options);

    $this->assertTrue($success,
      "Loading genotypes failed.");
  }
}
