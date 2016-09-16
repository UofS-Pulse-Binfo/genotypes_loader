<?php

/**
 * @file
 * Implements a drush command to load a VCF file
 */

/**
 * Implements hook_drush_command().
 */
function genotypes_loader_drush_command() {

  $items['load-genotypes'] = array(
    'description' => 'Takes a VCF file and associates it with a project in KnowPulse, then imports marker, variant and genotype data into KnowPulse.',
    'aliases' => array('load-vcf'),
    'arguments' => array(
      'input_file' => 'The filename of the VCF file for upload',
      'germplasm_mapping' => 'The filename of a tab-delimited file specifying the sample name in the VCF, the name of the stock in the database, the stock accession ID, the name of the germplasm, and the germplasm Accession ID',
    ),
  );

  // Make this one drush command for all 3 file formats?
  $items['load-genotype-matrix'] = array(
    'description' => 'Takes a genotype matrix and associates it with a project in Knowpulse, and stores marker, variant and genotype data.',
    'aliases' => array('load-matrix'),
    'arguments' => array(
      'input_file' => 'The filename of the matrix file for upload',
    ),
    'options' => array(
      'organism' => 'The organism to which the genotypes are associated with.',
      'variant_type' => 'The Sequence Ontology (SO) term name that describes the type of variants in the file (eg. SNP, MNP, indel).',
      'marker_type' => 'The Sequence Ontology (SO) term name that describes the marker technology used to generate the genotypes in the file (e.g. "Exome Capture", "GBS", "KASPar", etc.).',
      'project_name' => 'All genotypes will be grouped via a project to allow users to specify a particular dataset.',
      'nd_geolocation' => 'A meaningful location associated with this natural diversity experiment. For example, this could be the location the assay was completed in, the location the germplasm collection was from, or the location the markers were developed at. This should be the description field of your nd_geolocation.',
    ),
  );

  return $items;
}

/**
 * Our drush callback function
 * @TODO: should be prefixed with "drush_"
 */
function genotypes_loader_load_genotypes($input_file = NULL, $germplasm_mapping = NULL) {

  // Collect options from the user.
  // Organism
  $organism = drush_get_option('organism');
  if (!$organism) {
    $sql = "SELECT * FROM {organism} ORDER BY genus, species";
    $org_rset = chado_query($sql);
    $organisms = array();
    while ($organism = $org_rset->fetchObject()) {
      $organisms[$organism->organism_id] = "$organism->genus $organism->species ($organism->common_name)";
    }
    $organism = drush_choice($organisms, dt('Please select which organism the genotypes are associated with.'));
  } else {
    // @TODO: Look up the organism ID if it organism name was provided as an option
  }

  // Variant type
  $variant_type = drush_get_option('variant_type');
  if (!$variant_type) {
    //$values = array('name' => 'sequence');
    //$cv = chado_select_record('cv', array('cv_id'), $values);
    //$cv_id = $cv[0]->cv_id;
    $variant_type = drush_prompt(dt('Please enter the Sequence Ontology (SO) term name that describes the type of variants in the file (eg. SNP, MNP, indel).'));
  }

  // Marker type
  $marker_type = drush_get_option('marker_type');
  if (!$marker_type) {
    $marker_type = drush_prompt(dt('Please enter the Sequence Ontology (SO) term name that describes the marker technology used to generate the genotypes in the file (e.g. "Exome Capture", "GBS", "KASPar", etc.).'));
  }

  // Project
  // @TODO: Should this be retrieved using drush_choice instead?
  $project_name = drush_get_option('project_name');
  if (!$project_name) {
    $project_name = drush_prompt(dt('Please provide the name of a project in which all genotypes will be grouped to allow users to specify a particular dataset.'));
  }

  // If the storage method is ND experiment, then grab this option.
  // ND Geolocation
  $nd_geolocation = drush_get_option('nd_geolocation');
  if (!$nd_geolocation) {
    $nd_geolocation = drush_prompt(dt('Please provide a meaningful location associated with this natural diversity experiment. For example, this could be the location the assay was completed in, the location the germplasm collection was from, or the location the markers were developed at. This should be the description field of your nd_geolocation.'));
  }

  // @TODO: Collect the storage method here or check for it elsewhere??
  $storage_method = variable_get('nd_genotype_storage_method', 'nd_exp');


  print_r("Organism : $organisms[$organism]\n");
  print_r("Variant type: $variant_type\n");
  print_r("Marker type: $marker_type\n");
  print_r("Project: $project_name\n");
  print_r("ND_geolocation: $nd_geolocation\n");

  $options = array(
    'organism_id' => $organisms[$organism],
    'variant_type' => $variant_type,
    'feature_type_of_marker' => 'genetic_marker',
    'marker_type' => $marker_type,
    'project_name' => $project_name,
    'storage_method' => $storage_method,
    'insert_samples' => '0',
    'insert_germplasm' => '0',
    'insert_markers' => '2',
    'insert_variants' => '2',
  );
  if (isset($options['nd_geolocation'])) $options['nd_geolocation'] = $nd_geolocation;

  // @TODO: Will need to acquire the DB-specific options somehow...
}

/**
 * Callback for load-genotypes command
 */
function drush_genotypes_loader_load_genotypes($input_file = NULL, $sample_file = NULL) {

  // TEST INPUTS. @TODO: Get these as options
  $options = array(
    'organism_id' => '4',
    'variant_type' => 'SNP',
    'feature_type_of_marker' => 'genetic_marker',
    'marker_type' => 'GBS',
    'project_name' => 'AGILE: Application of Genomic Innovation in the Lentil Economy',
    'storage_method' => 'genotype_call',
    'insert_samples' => '2',
    'insert_germplasm' => '2',
    'insert_markers' => '2',
    'insert_variants' => '2',
    'nd_geolocation' => 'University of Saskatchewan',
  );

  // DATA RETRIEVAL
  // ------------------------
  // Retrieve all the type_ids to save time loading.
  $terms = array(
    array('name' => 'DNA', 'cv_name' => 'stock_types'),
    array('name' => 'Accession', 'cv_name' => 'stock_types'),
    array('name' => 'Individual', 'cv_name' => 'stock_types'),
    array('name' => 'is_extracted_from', 'cv_name' => 'stock_relationships'),
    array('name' => 'is_marker_of', 'cv_name' => 'relationship'),
    array('name' => $options['variant_type'], 'cv_name' => 'sequence'),
    array('name' => $options['feature_type_of_marker'], 'cv_name' => 'sequence'),
    array('name' => 'marker_type', 'cv_name' => 'feature_property'),
  );
  $query = 'SELECT cvterm_id FROM {cvterm} WHERE name=:name AND cvterm.cv_id IN (SELECT cv_id FROM {cv} WHERE name=:cv_name)';
  $types = array();
  foreach ($terms as $d) {
    $term = $d['name'];
    $types[$term] = chado_query($query, array(':name' => $term, ':cv_name' => $d['cv_name']))->fetchField();
    if (!$types[$term]) {
      return drush_set_error(dt("ERROR: Could not find type '@type' in the database.",
        array('@type' => $term)));
    }
  }

  // Retrieve the name of our organism
  $organism_name = chado_query(
     "SELECT organism.genus||' '||organism.species
        FROM {organism}
        WHERE organism_id=:id",
     array(':id' => $options['organism_id'])
  )->fetchField();
  if (!$organism_name) {
    return drush_set_error(dt("ERROR: Could not find an organism with ID of \"@organism_id\" in the database.",
      array('@organism_id' => $options['organism_id'])));
  } else {
    drush_print(dt("Found organism \"@organism\" in the database.", array('@organism' => $organism_name)));
  }

  // Retrieve the project ID
  $project_id = chado_query(
     'SELECT project_id
        FROM {project}
        WHERE name=:name',
     array(':name' => $options['project_name'])
  )->fetchField();
  if (!$project_id) {
    return drush_set_error(dt("ERROR: Could not find project \"@project_name\" in the database.",
      array('@project_name' => $options['project_name'])));
  } else {
    drush_print(dt("Found project \"@project_name\" in the database.", array('@project_name' => $options['project_name'])));
    $options['project_id'] = $project_id;
  }

  // FILE VALIDATION
  // ------------------------
  // Check the files exist
  $input_file = trim($input_file);
  if (!$input_file) { return drush_set_error(dt('ERROR: No input file specified. See: \'drush help load-genotypes\'')); }
  $sample_file = trim($sample_file);
  if (!$sample_file) { return drush_set_error(dt('ERROR: No sample mapping specified. See: \'drush help load-genotypes\'')); }

  // Ensure we can open the files. If not, first check the current directory and
  // attach the full path and try again.
  if (!file_exists($input_file) || !file_exists($sample_file)) {
    $current_directory = drush_cwd();
    $input_file = $current_directory . "/" . $input_file;
    $sample_file = $current_directory . "/" . $sample_file;

    // Test each file again, return error if we still can't find it.
    foreach (array($input_file, $sample_file) as $file) {
      if (!file_exists($file)) {
        return drush_set_error(dt("ERROR: Could not locate \'@file\'\nPlease ensure you spelled your file correctly or that the web server has permissions to read the file.",
        array('@file' => $file)));
      }
    }
  }

  // Open the sample mapping file and ensure we have the expected number of columns.
  // Save it as an associative array and close it
  $samples = fopen($sample_file, 'r');
  $first_line = fgets($samples);
  $first_line = trim($first_line);
  $samples_keys = preg_split('/[\t]/', $first_line);
  $num_columns = count($samples_keys);
  if ($num_columns != 5) {
    return drush_set_error(dt("ERROR: Unexpected number of columns in \'@file\'\nSee: \'drush help load-genotypes\''",
        array('@file' => $sample_file)));
  }

  /* Remove the first element in the samples_header array to use as a key in a nested associative array for all the samples.
  *  The resulting samples "hash" should look something like:
  *  'ABC' => (
  *    'Sample_name' => 'ABC Extraction',
  *    'Sample_Accession' => '123',
  *    'Germplasm_name' => 'ABC',
  *    'Germplasm_Extraction' => '112233'
  *  );
  */
  array_shift($samples_keys);
  $samples_array = array();
  while(!feof($samples)) {
    $current_line = fgetcsv($samples, 0, "\t");
    if (empty($current_line)) continue;
    // Pull out the sample name as it should appear in the VCF file
    $current_sample = array_shift($current_line);
    $sample_values = array_combine($samples_keys, $current_line);
    $cur_sample_array = array("$current_sample" => $sample_values);
    $samples_array = array_merge($samples_array, $cur_sample_array);
  }

  // Open the input file and try to determine which type of file it is.
  $file = fopen($input_file, 'r');
  $first_line = fgets($file);
  if (preg_match('/^##fileformat/', $first_line)) {
    // File format is VCF since first line should be: ##fileformat=VCFv4.x
    $file_format = "vcf";
  } else {
    while(!feof($file)) {
      # Get each meta line to skip processing them
      $current_line = fgets($file);
      if (preg_match('/^#CHROM/', $current_line)) {
        $file_format = 'vcf';
        break;
      } else if (preg_match('/\t[ATCG]+\t[ATGC]+$/i', $current_line)) {
        // File format is a matrix since there are 2 or more genotype calls on a single line
        // This won't match VCF since it checks for nucleotide calls at the end of the line
        $file_format = 'matrix';
        break;
      } else {
        $current_line_arr = explode("\t", $current_line);
        $column_count = count($current_line_arr);
        if ((preg_match('/\t[ATGC]+/i', $current_line)) AND (($count == 4) || ($count == 5))) {
          // File format is a "flat matrix" = 1 line per genotype call
          $file_format = 'legacy';
          break;
        }
      }
    }
  }
  if ($file_format == "vcf") { genotypes_loader_load_VCF($input_file, $samples_array, $options, $types); }
  else if ($file_format == "matrix") { genotypes_loader_load_matrix($input_file, $samples_array, $options); }
  else if ($file_format == "legacy") { genotypes_loader_load_legacy($input_file, $samples_array, $options); }
  else {
      return drush_set_error(dt("ERROR: Could not determine the format for \'@file\'\n. Please ensure your file is tab-delimited in the form of a VCF, a genotype matrix or genotype flat file.",
        array('@file' => $file)));
  }

  drush_print(dt("Storage method: \"@method\"", array('@method' => $options['storage_method'])));

}
