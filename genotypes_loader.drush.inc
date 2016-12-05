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
    'description' => 'A loader for genotypic data in either a flat file, matrix or VCF format.',
    'aliases' => array('load-geno'),
    'arguments' => array(
      'input-file' => 'The filename of the genotypic file for upload',
      'sample-file' => 'The filename of a tab-delimited file specifying for each sample name in the genotypes file: the name of the stock in the database, the stock accession ID, the name of the germplasm, and the germplasm Accession ID. See "samples.list" in the sample_files folder for an example.',
    ),
    'options' => array(
      'organism' => 'The organism to which the genotypes are associated with.',
      'variant-type' => 'The Sequence Ontology (SO) term name that describes the type of variants in the file (eg. SNP, MNP, indel).',
      'marker-type' => 'A free-text title that describes the marker technology used to generate the genotypes in the file (e.g. "Exome Capture", "GBS", "KASPar", etc.).',
      'project-name' => 'All genotypes will be grouped via a project to allow users to specify a particular dataset.',
      'ndgeolocation' => 'A meaningful location associated with this natural diversity experiment. For example, this could be the location the assay was completed in, the location the germplasm collection was from, or the location the markers were developed at. This should be the description field of your ndgeolocation.',
    ),
  );

  return $items;
}

/**
 * Our drush callback function
 */
function drush_genotypes_loader_load_genotypes($input_file = NULL, $sample_file = NULL) {

  // ------------------------
  // FILE VALIDATION
  // ------------------------
  // Check that both files exist
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
        return drush_set_error(dt("ERROR: Could not locate '@file'\nPlease ensure you spelled your file correctly or that the web server has permissions to read the file.",
        array('@file' => $file)));
      }
    }
  }

  // ---------------------------
  // COLLECT USER OPTIONS
  // ---------------------------

  // ----- Organism -----
  // @TODO: Allow multiple organisms
  // Set up our array of all possible organisms in the database.
  $sql = "SELECT * FROM {organism} ORDER BY genus, species";
  $org_rset = chado_query($sql);
  $organisms = array();
  while ($organism = $org_rset->fetchObject()) {
    // Start building our array of organisms
    $organisms[$organism->organism_id] = "$organism->genus $organism->species ($organism->common_name)";
  }
  // See if the user has provided organism name as an option, and split into genus and species.
  $organism_name = drush_get_option('organism', NULL);
  if ($organism_name) {
    foreach ($organisms as $o_id => $organism) {
      if (strpos($organism, $organism_name) !== false) {
        $organism_id = $o_id;
        break;
      }
    }
    // Check if there was no match
    if (!$organism_id) {
      drush_log("Could not find a record in the database for '" . $organism_name . "'", 'warning');
      $organism_id = drush_choice($organisms, dt('Please select an organism from the following options:'));
    }
  } else {
    // Either we didn't find the organism that the user provided as an option, or no option was given. So
    // give them a listing to choose from using the array we just built.
    $organism_id = drush_choice($organisms, dt('Please select which organism the genotypes are associated with.'));
  }

  // ----- Variant type -----
  $variant_type = drush_get_option('variant-type', NULL);
  if (!$variant_type) {
    //$values = array('name' => 'sequence');
    //$cv = chado_select_record('cv', array('cv_id'), $values);
    //$cv_id = $cv[0]->cv_id;
    $variant_type = drush_prompt(dt('Please enter the Sequence Ontology (SO) term name that describes the type of variants in the file (eg. SNP, MNP, indel).'));
  }

  // ----- Marker type -----
  $marker_type = drush_get_option('marker-type', NULL);
  if (!$marker_type) {
    $marker_type = drush_prompt(dt('Please enter a free-text title that describes the marker technology used to generate the genotypes in the file (e.g. "Exome Capture", "GBS", "KASPar", etc.).'));
  }

  // ----- Project -----
  // @TODO: Should this be retrieved using drush_choice instead?
  $project_name = drush_get_option('project-name', NULL);
  if (!$project_name) {
    $project_name = drush_prompt(dt('Please provide the name of a project in which all genotypes will be grouped to allow users to specify a particular dataset.'));
  }

  // If the storage method is ND experiment, then grab this option.
  // ----- ND Geolocation -----
  $nd_geolocation = drush_get_option('ndgeolocation', NULL);
  if (!$nd_geolocation) {
    $nd_geolocation = drush_prompt(dt('Please provide a meaningful location associated with this natural diversity experiment. For example, this could be the location the assay was completed in, the location the germplasm collection was from, or the location the markers were developed at. This should be the description field of your nd_geolocation.'));
  }

  // @TODO: Collect the following variables through a settings form
  //$storage_method = variable_get('nd_genotype_storage_method', 'nd_exp');
  $storage_method = 'genotype_call';
  $feature_type_of_marker = 'genetic_marker';
  $insert_samples = '2';
  $insert_germplasm = '2';
  $insert_markers = '2';
  $insert_variants = '2';

  $options = array(
    'organism_id' => $organism_id,
    'variant_type' => $variant_type,
    'feature_type_of_marker' => $feature_type_of_marker,
    'marker_type' => $marker_type,
    'project_name' => $project_name,
    'storage_method' => $storage_method,
    'insert_samples' => $insert_samples,
    'insert_germplasm' => $insert_germplasm,
    'insert_markers' => $insert_markers,
    'insert_variants' => $insert_variants,
  );
  if (isset($options['nd_geolocation'])) $options['nd_geolocation'] = $nd_geolocation;

  genotypes_loader_load_genotypes($input_file, $sample_file, $options);

}

/**
 * Callback for load-genotypes command
 */
function genotypes_loader_load_genotypes($input_file = NULL, $sample_file = NULL, $options) {

  // ------------------------
  // DATA RETRIEVAL
  // ------------------------
  drush_print("Begin validating options and samples...");

  // Inform the user which data loading method is being used.
  $storage_method = array(
    'nd_exp' => 'Natural Diversity Experiment',
    'genotype_call' => 'Genotype Call Custom Table',
    'stock_genotype' => 'Genotype Stock table',
    'custom' => 'Custom',
  );
  drush_print('  Loading your data according to the "'.$storage_method[ $options['storage_method'] ].'" Method.');

  // Check the custom table is there if using Method #2: Genotype Call
  // @Lacey: This code was originally from the genotype matrix loader. Is this sufficient? (looks like it
  // just creates the custom table?)
  if ($options['storage_method'] == 'genotype_call') {
    chado_create_custom_table(
      'genotype_call',
      nd_genotypes_genotype_call_schema_template()
    );
  }

  // Set some variables to abstract mode for genotypes_loader_helper_add_record_with_mode().
  $select_only = 0;
  $insert_only = 1;
  $both = 2;

  $db_options = array(
    1 => 'Insert Only',
    0 => 'Select Only',
    2 => 'Insert And Select',
  );

  // ----- TYPE IDs -----
  // @TODO: Set these through a settings form
  $terms = array(
    array('name' => 'DNA', 'cv_name' => 'stock_type'),
    array('name' => 'Accession', 'cv_name' => 'stock_type'),
    array('name' => 'Individual', 'cv_name' => 'stock_type'),
    array('name' => 'is_extracted_from', 'cv_name' => 'stock_relationship'),
    array('name' => 'is_marker_of', 'cv_name' => 'stock_relationship'),
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
  print_r($types);

  // ----- ORGANISM NAME -----
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
    drush_print(dt("  Found organism \"@organism\" in the database.", array('@organism' => $organism_name)));
  }

  // ----- PROJECT ID -----
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
    drush_print(dt("  Found project \"@project_name\" in the database.", array('@project_name' => $options['project_name'])));
    $options['project_id'] = $project_id;
  }

  drush_print('  Checking for samples [' . $db_options[ $options['insert_samples'] ] . '] and for germplasm [' . $db_options[ $options['insert_germplasm'] ] . '] in the database...');

  // ----- SAMPLES MAPPING -----
  // Open the sample mapping file and ensure we have the expected number of columns.
  // Save it as an associative array and close it
  $SAMPLE_MAP = fopen($sample_file, 'r') or die ("ERROR: Unable to open $sample_file!\n");
  $header = fgetcsv($SAMPLE_MAP, 0, "\t");
  $num_columns = count($header);
  if ($num_columns != 5) {
    return drush_set_error(dt("ERROR: Unexpected number of columns of $num_columns in '@file'\nSee: 'drush help load-genotypes''",
        array('@file' => $sample_file)));
  }

  while(!feof($SAMPLE_MAP)) {
    $current_line = fgetcsv($SAMPLE_MAP, 0, "\t");
    if (empty($current_line)) continue;
    // 1st column = DNA source name
    // Additional columns contain information needed by Chado
    $source_name = array_shift($current_line);
    $sample_name = array_shift($current_line);
    $sample_accession = array_shift($current_line);
    $germplasm_name = array_shift($current_line);
    $germplasm_accession = array_shift($current_line);

    // ----------------------------
    // LOOKUP/INSERT SAMPLES
    // ----------------------------
    /* @ASSUMPTION: Samples in the samples file get checked for or inserted regardless if they
     *              appear in the input file containing genotypic calls. This could be useful if
     *              whoever is managing the database wants to use a single master file containing
     *              all the samples in their database. It also means some samples may be inserted
     *              but no additional data is inserted for those samples by this loader.
     */

    // ----- STOCK -----
    $stock_id = genotypes_loader_helper_add_record_with_mode('Sample', 'stock', $options['insert_samples'], array(
      'uniquename' => $sample_accession,
      'organism_id' => $options['organism_id'],
      // @ASSUMPTION: The original stock for which the marker is associated to is of type DNA
      'type_id' => $types['DNA'],
    ), array(
      'name' => $sample_name,
    ));
    if (!$stock_id) { return FALSE; }
//    drush_print("Found stock ID: $stock_id for sample: $sample_name");

    // ----- GERMPLASM -----
    $germplasm_id = genotypes_loader_helper_add_record_with_mode('Germplasm', 'stock', $options['insert_germplasm'], array(
      'uniquename' => $germplasm_accession,
      'organism_id' => $options['organism_id'],
    ), array(
      // @ASSUMPTION: If a germplasm already exists, we don't assume its type, but we do require
      //              a type if we're inserting. So here we are assuming it is of type "Individual".
      'type_id' => $types['Individual'],
      'name' => $germplasm_name,
    ));
    if (!$germplasm_id) { return FALSE; }
//    drush_print("Found stock ID: $germplasm_id for germplasm: $germplasm_name");

    // ----- GERMPLASM TO SAMPLE LINK -----
    $status = genotypes_loader_helper_add_record_with_mode('Germplasm to Sample Link', 'stock_relationship', $both, array(
      'subject_id' => $stock_id,
      'type_id' => $types['is_extracted_from'],
      'object_id' => $germplasm_id,
    ));
    if (!$status) { return FALSE; }
//    drush_print("Linked stock ID: $stock_id to stock ID: $germplasm_id");

    // Save the sample name (in Chado) and its stock id in the options array
    $options['sample_list'][$source_name]['sample_name'] = $sample_name;
    $options['sample_list'][$source_name]['sample_stock_id'] = $stock_id;
  }

  drush_print("  Successfully mapped stocks and germplasm.");

  // ----- DETECT INPUT FILE TYPE -----
  // Open the input file and try to determine which type of file it is.
  $FILE = fopen($input_file, 'r') or die ("ERROR: Unable to open $input_file!\n");;
  $first_line = fgets($FILE);
  if (preg_match('/^##fileformat/', $first_line)) {
    // File format is VCF since first line should be: ##fileformat=VCFv4.x
    $file_format = "vcf";
  } else {
    while(!feof($FILE)) {
      // Get each meta line to skip processing them
      $current_line = fgets($FILE);
      if (preg_match('/^#CHROM/', $current_line)) {
        $file_format = 'vcf';
        break;
      } else if (preg_match('/\t[ATCG]+\t[ATGC]+$/i', $current_line)) {
        // File format is a matrix since there are 2 or more genotype calls on a single line
        // This won't match VCF since it checks for nucleotides at the end of the line
        $file_format = 'matrix';
        break;
      } else {
        $current_line_arr = explode("\t", $current_line);
        $column_count = count($current_line_arr);
        if ((preg_match('/\t[ATGC]+/i', $current_line)) AND (($column_count == 4) || ($column_count == 5))) {
          // File format is a "flat matrix" = 1 line per genotype call
          $file_format = 'legacy';
          break;
        }
      }
    }
  }

  // ----- CALL THE APPROPRIATE LOADER -----
  switch ($file_format) {
    case "vcf":
      drush_print("  Detected VCF file format. \nProceeding to load genotypes...");
      genotypes_loader_load_VCF($input_file, $options, $types);
      break;
    case "matrix":
      drush_print("  Detected genotype matrix file format. \nProceeding to load genotypes...");
      genotypes_loader_load_matrix($input_file, $options, $types);
      break;
    case "legacy":
      drush_print("  Detected flat file format. \nProceeding to load genotypes...");
      genotypes_loader_load_legacy($input_file, $options, $types);
      break;
    default:
      return drush_set_error(dt("ERROR: Could not determine the format for '@file'. Please ensure your file is tab-delimited in the form of a VCF, a genotype matrix or genotype flat file.", array('@file' => $input_file)));
  }
}
