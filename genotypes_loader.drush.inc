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
      'organism' => 'The organism of the samples in your samples file. If your samples consist of multiple organisms, please fill in the organism in the 7th column of the samples file. The organism name should be formatted as "Genus species". For example, for cultivated lentil: Lens culinaris',
      'variant-type' => 'A free-text title that describes the type of variants in the file (e.g. SNP, MNP, Indel).',
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
  //      FILE VALIDATION
  // ------------------------
  // Check that both files exist
  $input_file = trim($input_file);
  if (!$input_file) {
    if ($options['no_drush']) { print 'Ln43'; return FALSE; }
    else {
      return drush_set_error(dt('ERROR: No input file specified. See: \'drush help load-genotypes\''));
    }
  }
  $sample_file = trim($sample_file);
  if (!$sample_file) {
    if ($options['no_drush']) { print 'Ln50'; return FALSE; }
    else {
      return drush_set_error(dt('ERROR: No sample mapping specified. See: \'drush help load-genotypes\''));
    }
  }

  // Ensure we can open the files. If not, first check the current directory and
  // attach the full path and try again.
  if (!file_exists($input_file) || !file_exists($sample_file)) {
    $current_directory = drush_cwd();
    $input_file = $current_directory . "/" . $input_file;
    $sample_file = $current_directory . "/" . $sample_file;
    // Test each file again, return error if we still can't find it.
    foreach (array($input_file, $sample_file) as $file) {
      if (!file_exists($file)) {
        if ($options['no_drush']) { print 'Ln65'; return FALSE; }
        else {
          return drush_set_error(dt("ERROR: Could not locate '@file'\nPlease ensure you spelled your file correctly or that the web server has permissions to read the file.", array('@file' => $file)));
        }
      }
    }
  }

  // ---------------------------
  //    COLLECT USER OPTIONS
  // ---------------------------

  // ----- Organism -----
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
      $organism_id = drush_choice($organisms, dt(
'Please select an organism for your samples from the following options.
If your samples are from multiple organisms, it is recommended that you
add organisms to each sample in the samples file. You can choose the most
prevalent organism here, and anything in the file will take precedence over it.:'));
    }
  } else {
    // Either we didn't find the organism that the user provided as an option, or no option was given. So
    // give them a listing to choose from using the array we just built.
    $organism_id = drush_choice($organisms, dt(
'Please select an organism for your samples from the following options.
If your samples are from multiple organisms, it is recommended that you
add organisms to each sample in the samples file. You can choose the most
prevalent organism here, and anything in the file will take precedence over it.:'));
  }

  // ----- Variant type -----
  $variant_type = drush_get_option('variant-type', NULL);
  if (!$variant_type) {
    //$values = array('name' => 'sequence');
    //$cv = chado_select_record('cv', array('cv_id'), $values);
    //$cv_id = $cv[0]->cv_id;
    $variant_type = drush_prompt(dt('Please enter a free-text title that describes the type of variants in the file (eg. SNP, MNP, indel).'));
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

  /*
  // If the storage method is ND experiment, then grab this option.
  // ----- ND Geolocation -----
  $nd_geolocation = drush_get_option('ndgeolocation', NULL);
  if (!$nd_geolocation) {
    $nd_geolocation = drush_prompt(dt('Please provide a meaningful location associated with this natural diversity experiment. For example, this could be the location the assay was completed in, the location the germplasm collection was from, or the location the markers were developed at. This should be the description field of your nd_geolocation.'));
  }
  */

  // @TODO: Collect the following variables through a settings form
  //$storage_method = variable_get('nd_genotype_storage_method', 'nd_exp');
  $storage_method = 'genotype_call';
  $feature_type_of_marker = 'genetic_marker';
  $feature_type_of_variant = 'sequence_variant';
  $genotype_type = 'SNP';
  $insert_samples = variable_get('genotypes_samples_mode', 2);
  $insert_germplasm = variable_get('genotypes_germplasm_mode', 2);
  $insert_variants = variable_get('genotypes_variants_mode', 2);
  $insert_markers = variable_get('genotypes_markers_mode', 2);

  $options = array(
    'organism_id' => $organism_id,
    'feature_type_of_variant' => $feature_type_of_variant,
    'variant_type' => $variant_type,
    'feature_type_of_marker' => $feature_type_of_marker,
    'marker_type' => $marker_type,
    'genotype_type' => $genotype_type,
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
  //      DATA RETRIEVAL
  // ------------------------
  if (!$options['silent']) { drush_print("Begin validating options and samples..."); }

  // Inform the user which data loading method is being used.
  $storage_method = array(
    'nd_exp' => 'Natural Diversity Experiment',
    'genotype_call' => 'Genotype Call Custom Table',
    'stock_genotype' => 'Genotype Stock table',
    'custom' => 'Custom',
  );
  if (!$options['silent']) { drush_print('  Loading your data according to the "'.$storage_method[ $options['storage_method'] ].'" Method.'); }

  // Check the custom table is there if using Method #2: Genotype Call
  if ($options['storage_method'] == 'genotype_call') {
    // Before attempting to create the table, check it it's already there.
    if (!chado_get_custom_table_id('genotype_call')) {
      chado_create_custom_table(
        'genotype_call',
        genotypes_loader_genotype_call_schema_template()
      );
    }
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
  // @TODO: Make CVs configurable. See comment below!
  $terms = array();
  $terms['sample_type'] = array('name' => variable_get('genotypes_sample_type', 'genomic_DNA'), 'cv_name' => 'sequence');
  $terms['sample_germplasm_relationship'] = array('name' => variable_get('genotypes_stock_ref', 'is_extracted_from'), 'cv_name' => 'stock_relationship');
  $terms['marker_variant_relationship'] = array('name' => variable_get('genotypes_feature_rel', 'is_marker_of'), 'cv_name' => 'feature_relationship');
  $terms['variant_type'] = array('name' => $options['feature_type_of_variant'], 'cv_name' => 'sequence');
  $terms['marker_type'] = array('name' => $options['feature_type_of_marker'], 'cv_name' => 'sequence');
  // For our purposes, additionalType is the cv term used for both markers and variants. This desperately calls for the ability for users to change
  // the CV which they are pulling from since additionalType is the only useful term in its CV.
  $terms['free_text_marker_type'] = array('name' => variable_get('genotypes_marker_featureprop_type', 'additionalType'), 'cv_name' => 'schema');
  $terms['free_text_variant_type'] = array('name' => variable_get('genotypes_variant_featureprop_type', 'additionalType'), 'cv_name' => 'schema');
  $terms['genotype_type'] = array('name' => $options['genotype_type'], 'cv_name' => 'sequence');

  $type_query = 'SELECT cvterm_id FROM {cvterm} WHERE name=:name AND cvterm.cv_id IN (SELECT cv_id FROM {cv} WHERE name=:cv_name)';
  foreach ($terms as $key => $values) {
    $term = $values['name'];
    $cv_name = $values['cv_name'];
    $terms[$key]['cvterm_id'] = chado_query($type_query, array(':name' => $term, ':cv_name' => $cv_name))->fetchField();
    if (!$terms[$key]['cvterm_id']) {
      if ($options['no_drush']) { print 'Ln232:'.$term; return FALSE; }
      else {
        return drush_set_error(dt("ERROR: Could not find type '@term' with vocabulary '@cv_name'.",
          array('@term' => $term, '@cv_name' => $cv_name,)));
      }
    }
  }

  // ----- ORGANISM NAME -----
  // This is the organism for which the chromosome/variants/markers are located on its genome, since multiple organisms
  // may be present in the input file but only one organism's genome would have had the sequence aligned to it.
  $organism_name = chado_query(
     "SELECT organism.genus||' '||organism.species
        FROM {organism}
        WHERE organism_id=:id",
     array(':id' => $options['organism_id'])
  )->fetchField();
  if (!$organism_name) {
    if ($options['no_drush']) { print 'Ln250:'.$options['organism_id']; return FALSE; }
    else {
      return drush_set_error(dt("ERROR: Could not find an organism with ID of \"@organism_id\" in the database.",
        array('@organism_id' => $options['organism_id'])));
    }
  } else {
    if (!$options['silent']) { drush_print(dt("  Found organism \"@organism\" in the database.", array('@organism' => $organism_name))); }
  }

  // ----- PROJECT ID -----
  $project_id = chado_query(
     'SELECT project_id
        FROM {project}
        WHERE name=:name',
     array(':name' => $options['project_name'])
  )->fetchField();
  if (!$project_id) {
    if ($options['no_drush']) { print 'Ln267'; return FALSE; }
    else {
      return drush_set_error(dt("ERROR: Could not find project \"@project_name\" in the database.",
        array('@project_name' => $options['project_name'])));
    }
  } else {
    if (!$options['silent']) { drush_print(dt("  Found project \"@project_name\" in the database.", array('@project_name' => $options['project_name']))); }
    $options['project_id'] = $project_id;
  }

  if (!$options['silent']) { drush_print('  Checking for samples [' . $db_options[ $options['insert_samples'] ] . '] and for germplasm [' . $db_options[ $options['insert_germplasm'] ] . '] in the database...'); }

  // ---------------------------
  //       SAMPLES MAPPING
  // ---------------------------
  // Open the sample mapping file and ensure we have the expected number of columns.
  // Save it as an associative array and close it
  $SAMPLE_MAP = fopen($sample_file, 'r') or die ("ERROR: Unable to open $sample_file!\n");
  $header = fgetcsv($SAMPLE_MAP, 0, "\t");
  $num_columns = count($header);
  // Originally I didn't negate the following if statement, creating some frustration @OnAFriday...
  if (!(($num_columns >= 5) && ($num_columns <= 7))) {
    if ($options['no_drush']) { print 'Ln289'; return FALSE; }
    else {
      return drush_set_error(dt("ERROR: Unexpected number of columns of $num_columns in '@file'\nSee: 'drush help load-genotypes''",
        array('@file' => $sample_file)));
    }
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

    // Setting the following as defaults if the 6th and/or 7th columns are not provided
    $germplasm_type = 'Individual';
    $organism_id = $options['organism_id'];

    // User can optionally supply a stock_type for each germplasm if they are inserting
    if ($num_columns >= 6) {
      $germplasm_type = array_shift($current_line);

      if (!$terms[$germplasm_type]) {
        // Query to retrieve type IDs was set above:
        // SELECT cvterm_id FROM {cvterm} WHERE name=:name AND cvterm.cv_id IN (SELECT cv_id FROM {cv} WHERE name=:cv_name)
        $cv_name = 'stock_type';
        $terms[$germplasm_type] = array('name' => $germplasm_type, 'cv_name' => $cv_name);
        $terms[$germplasm_type]['cvterm_id'] = chado_query($type_query, array(':name' => $germplasm_type, ':cv_name' => $cv_name))->fetchField();
      }
      if (!$terms[$germplasm_type]['cvterm_id']) {
        if ($options['no_drush']) { print 'Ln232'; return FALSE; }
        else {
          return drush_set_error(dt("ERROR: Could not find germplasm type '@term' with the vocabulary '@cv_name'.", array('@term' => $germplasm_type, '@cv_name' => $cv_name,)));
        }
      }
    }

    // User can optionally supply an organism for each germplasm if they are inserting germplasm into the database.
    // We need to allow spaces in the genus and species, so use a custom query to check that what the user
    // input matches what is in the database.
    if ($num_columns == 7) {
      $organism = array_shift($current_line);
      $organism_query = "SELECT organism_id FROM {organism} WHERE genus||' '||species=:user_input";
      $organism_id = chado_query($organism_query, array(':user_input' => $organism))->fetchField();
      if (!$organism_id) {
        if ($options['no_drush']) { print 'Ln338'; return FALSE; }
        else {
          return drush_set_error(dt("ERROR: Could not find an organism \"@organism_name\" in the database.",
            array('@organism_name' => $organism)));
        }
      }
    }

    // ----------------------------
    //    LOOKUP/INSERT SAMPLES
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
      'organism_id' => $organism_id,
      // @ASSUMPTION: The original stock for which the marker is associated to is of type 'genomic_DNA'.
      'type_id' => $terms['sample_type']['cvterm_id'],
    ), array(
      'name' => $sample_name,
    ));
    if (!$stock_id) {
      if ($options['no_drush']) { print 'Ln366'; return FALSE; }
      else {
        return drush_set_error(dt('ERROR: Could not find a stock ID for @stock', array('@stock' => $sample_accession)));
      }
    }
//    if (!$options['silent']) { drush_print("Found stock ID: $stock_id for sample: $sample_name"); }

    // ----- GERMPLASM -----
    $germplasm_id = genotypes_loader_helper_add_record_with_mode('Germplasm', 'stock', $options['insert_germplasm'], array(
      'uniquename' => $germplasm_accession,
      // @ASSUMPTION: If a germplasm already exists, we don't assume its type, but we do require
      //              a type if we're inserting. So unless it was specified in the samples list, we
      //              are assuming type 'Individual'. Ideally, this user could provide the default type
      //              as an option like with organism. *wink wink nudge nudge*
      'type_id' => $terms[$germplasm_type]['cvterm_id'],
      'organism_id' => $organism_id,
    ), array(
      'name' => $germplasm_name,
    ));

    if (!$germplasm_id) {
      if ($options['no_drush']) { print 'Ln387'; return FALSE; }
      else {
        return drush_set_error(dt('ERROR: Could not find a germplasm ID for @germplasm', array('@germplasm' => $germplasm_accession)));
      }
    }
//    if (!$options['silent']) { drush_print("Found stock ID: $germplasm_id for germplasm: $germplasm_name");

    // ----- GERMPLASM TO SAMPLE LINK -----
    $status = genotypes_loader_helper_add_record_with_mode('Germplasm to Sample Link', 'stock_relationship', $both, array(
      'subject_id' => $stock_id,
      'type_id' => $terms['sample_germplasm_relationship']['cvterm_id'],
      'object_id' => $germplasm_id,
    ));
    if (!$status) {
      if ($options['no_drush']) { print 'Ln401'; return FALSE; }
      else {
        return drush_set_error(dt('ERROR: Could not link germplasm @germplasm to stock @sample', array(
          '@germplasm' => $germplasm_name,
          '@sample' => $sample_name)));
      }
    }
//    if (!$options['silent']) { drush_print("Linked stock ID: $stock_id to stock ID: $germplasm_id"); }

    // Save the sample name (according to Chado) and its stock id in the options array
    $options['sample_list'][$source_name]['sample_name'] = $sample_name;
    $options['sample_list'][$source_name]['sample_stock_id'] = $stock_id;
  }

  if (!$options['silent']) { drush_print("  Successfully mapped stocks and germplasm."); }

  // ----- DETECT INPUT FILE TYPE -----
  // @TODO: Deprecate this and ask the user :'-(
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
      if (!$options['silent']) { drush_print("  Detected VCF file format. \nProceeding to load genotypes..."); }
      genotypes_loader_load_VCF($input_file, $options, $terms);
      break;
    case "matrix":
      if (!$options['silent']) { drush_print("  Detected genotype matrix file format. \nProceeding to load genotypes..."); }
      genotypes_loader_load_matrix($input_file, $options, $terms);
      break;
    case "legacy":
      if (!$options['silent']) { drush_print("  Detected flat file format. \nProceeding to load genotypes..."); }
      genotypes_loader_load_legacy($input_file, $options, $terms);
      break;
    default:
      if ($options['no_drush']) { print 'Ln43'; return FALSE; }
      else {
        return drush_set_error(dt("ERROR: Could not determine the format for '@file'. Please ensure your file is tab-delimited in the form of a VCF, a genotype matrix or genotype flat file.", array('@file' => $input_file)));
      }
  }
}
