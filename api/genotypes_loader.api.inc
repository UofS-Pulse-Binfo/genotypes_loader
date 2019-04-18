<?php
/**
 * @file
 * Helper functions to aid in loading genotypes into the database.
 */

/**
 * Determines the correct postgreSQL version, otherwise assumes version 9.2.
 *
 * @return
 *   The numbered version of PostgreSQL (ex. 9.2, 9.3, etc)
 */
function genotypes_loader_recheck_postgresql_version() {
  $version_string = db_query('SELECT version()')->fetchField();
  $version = '9.2';
  if (preg_match('/PostgreSQL (\d+\.\d+)\.\d+/', $version_string, $matches)) {
    $version = $matches[1];
  }
  variable_set('pgsql_version', $version);

  return $version;
}

/**
  * Retrieve the postgreSQL version.
  */
function genotypes_loader_get_postgresql_version() {
  return variable_get('pgsql_version', '9.2');
}

/**
 * Adds/Selects a record based on the mode. Helper function for loading genotype matrix
 * that does the select/insert based on mode logic.
 *
 * @TODO: Give the different errors that are possible separate error codes. Also allow a parameter to be given to
 * either "handle errors" or not- this way, if we see a certain error (such as not unique), we can handle it
 * from outside of the function.
 *
 * @param $record_type
 *   A human-readable term for your record for error messages.
 * @param $table
 *   The chado table to select/insert from/into.
 * @param $mode
 *   One of 0: Select Only, 1: Insert Only, 2: Insert & Select.
 * @param $select_values
 *   An array of [table column] => [value] mappings. These values will be used for both the
 *   select and the insert, but are specific to select
 * @param $insert_values
 *   An array of [table column] => [value] mappings that is specific to the insert. This is not required if mode is
 *   select only, and will be combined with select values for an insert.
 * @return
 *   The value of the primary key for the inserted/selected record or FALSE if failed.
 */
function genotypes_loader_helper_add_record_with_mode($record_type, $table, $mode, $select_values, $insert_values = array()) {

  // Set some variables to abstract mode
  $select_only = 0;
  $insert_only = 1;
  $both = 2;

  // the name of the primary key.
  $pkey = $table . '_id';

  // First we select the record to see if it already exists.
  $record = chado_select_record($table, array($pkey), $select_values);

  // If it exists and the mode is 1 (Insert Only) then return an error to drush.
  if (sizeof($record) == 1) {
    if ($mode == $insert_only) {
      tripal_report_error(
        'genotypes_loader',
        TRIPAL_ERROR,
        'Record "@record_type" already exists but you chose to only insert (mode=@mode). Values: '.print_r($select_values, TRUE), 
        array('@record_type' => $record_type, '@mode' => $mode)
      );
      return FALSE;
    }
    // Otherwise the mode allows select so return the value of the primary key.
    else {
      return $record[0]->{$pkey};
    }
  }

  // If more then one result is returned then this is NOT UNIQUE and we should report an
  // error to the user - not just run with the first one.
  elseif (sizeof($record) > 1) {
    tripal_report_error(
      'genotypes_loader',
      TRIPAL_ERROR,
      'Record "@record_type" is not unique. (mode=@mode). Values: ' . print_r($select_values,TRUE), 
      array('@record_type' => $record_type, '@mode' => $mode)
    );
    return FALSE;
  }

  // If there is no pre-existing sample but we've been given permission to create it,
  // then insert it
  elseif ($mode == $both) {

    // If we want to insert values, we can merge our values to have all the information we need
    $values = array_merge($select_values, $insert_values);

    $record = chado_insert_record($table, $values);

    // If the primary key is available then the insert worked and we can return it.
    if (isset($record[$pkey])) {
      return $record[$pkey];
    }
    // Otherwise, something went wrong so tell the user
    else {
      tripal_report_error(
        'genotypes_loader',
        TRIPAL_ERROR,
        'Tried to insert "@record_type" but the primary key is returned empty (mode=@mode). Values: '.print_r($values,TRUE), 
        array('@record_type' => $record_type, '@mode' => $mode)
      );
      return FALSE;
    }
  }
  // If there is no pre-existing record and we are not allowed to create one
  // then return an error.
  else {
    tripal_report_error(
      'genotypes_loader',
      TRIPAL_ERROR,
      'Record "@record_type" doesn\'t already exist but you chose to only select (mode=@mode). Values: ', 
      array('@record_type' => $record_type, '@mode' => $mode)
    );
    return FALSE;
  }
}

/**
 * Places records into a CSV file, and then checks if it is time to bulk load the file via a COPY command
 *
 * @param $record_count
 *   The current count of records that are being loaded in.
 * @param $record_type
 *   A human-readable term for your record for error messages.
 * @param $table
 *   The chado table to insert into.
 * @param $insert_values
 *   An array of [table column] => [value] mappings that is specific to the insert.
 * @param $file_name
 *   An optional name for the file to write the values to
 * @param $final_chunk
 *   If set to true, then the remaining genotypes have been added to the file and it is time to copy them in.
 * @return
 *   TRUE if success (meaning, we reached the end of the function, not neccesarily that everything went swimmingly), FALSE if failed.
 */
function genotypes_loader_helper_copy_records($record_count, $record_type, $table, $insert_values = array(), $file_name = NULL, $final_chunk = FALSE) {

  // The number of records we want to reach before copying them in all at once
  $copy_chunk_size = 10000;

  // If we weren't given the name of the file to copy from, then try to find it using the record_type and table name
  // that we were given
  if ($file_name === NULL) {
    // Pull out the name of the database to ensure the file is not overwritten by a concurrent upload
    // to another database
    global $databases;
    $db_name = $databases['default']['default']['database'];

    $file_stub = '/tmp/genotype_loader.';
    $record_type = str_replace(' ', '_', $record_type);
    $record_type = strtolower($record_type);
    $file_name = $file_stub . $db_name . '-' . $table . '-' . $record_type . '.remotecopy.csv';
  }

  // If this is the last chunk to be entered into the database, then we don't have a record to add to the file.
  // Otherwise, open the file to write to since this is needed by fputcsv.
  // @TODO "Lacey: I'm concerned this might be a point of slowness. Perhaps there is a better
  //       way to create CSV that doesn't require fputcsv and is more reliable
  //       then simply imploding the values with comma's in-between."
  if (!$final_chunk) {
    // If we don't have a record to load in, something has defintiely gone wrong
    if ($record_count == 0) {
      return drush_set_error(dt('There is no genotype call to print to @filename', array('@filename' => $file_name)));
    }
    $file_handle = fopen($file_name, 'a') or die ("ERROR: Unable to create $file_name to bulk load records!\n");;
    ksort($insert_values); // to ensure they're always in the same order.
    fputcsv($file_handle, $insert_values);
    fclose($file_handle);
  }

  if (($record_count%$copy_chunk_size == 0) || ($final_chunk)) {
    // Write out the COPY command
    // Columns are listed in alphabetical order because we used ksort() before printing the values to a file.
    $copy_command = "COPY chado.genotype_call (
      genotype_id,
      marker_id,
      meta_data,
      project_id,
      stock_id,
      variant_id)
      FROM STDIN WITH CSV";

    genotypes_loader_remote_copy($copy_command, $file_name);

    // Wipe the file clean by reopening it as write-only
    $file_handle = fopen($file_name, 'w');
    fclose($file_handle);

    print("Copied in " . $record_count . " records.\n");
  }

  return TRUE;
}

/**
 * Uses drush to safely copy data quickly. ONLY USE IN TRIPAL JOBS!!
 *
 * PostgreSQL COPY is extremely effective at copying/inserting large amounts of
 * data. This modules uses it to make inserting massive genotype files more efficient.
 * However, using chado_query('COPY...') poses many security risks in a web setting.
 * Instead we use Drupal to determine the psql connection string, and psql's \copy command
 * to take advantage of the fact that Tripal Jobs are run on the command-line.
 *
 * @param $copy_command
 *   The full PostgreSQL copy command to be executed.
 * @param $file_name
 *   The name of the file from which COPY gets its contents to copy into the database
 */
function genotypes_loader_remote_copy($copy_command, $file_name) {

  // Setup the command within a drush created psql session using \COPY.
  $connection_string = `drush sql-connect`;
  $connection_string = trim($connection_string);
  $command = $connection_string . ' -c "\\' . $copy_command . '" < ' . $file_name;

  // Execute the command
  return `$command`;

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
function genotypes_loader_helper_add_genotypes_nd_exp($v) {

  // Set some variables to abstract mode for genotypes_loader_helper_add_record_with_mode().
  $select_only = 0;
  $insert_only = 1;
  $both = 2;

  // Genotype.
  $genotype_id = genotypes_loader_helper_add_record_with_mode('Genotype', 'genotype', $both, array(
    'name' => $v['marker_name'] . ': ' . $v['allele'],
    'uniquename' => str_replace(' ','_',$v['marker_name']) . '_' . $v['allele'],
    'description' => $v['allele'],
    'type_id' => $v['variant_type_id'],
  ));
  if (!$genotype_id) return FALSE;

  // Feature <=> Genotype Link.
  $feature_genotype_id = genotypes_loader_helper_add_record_with_mode('Marker Genotype Link', 'feature_genotype', $both, array(
    'feature_id' => $v['marker_id'],
    'genotype_id' => $genotype_id,
    'cvterm_id' => $v['variant_type_id'],
    'rank' => 0,
    'cgroup' => 0,
  ));
  if (!$feature_genotype_id) return FALSE;

  // ND Experiment.
  // Has to be done without genotypes_loader_helper_add_record_with_mode() since we need to
  // avoid selecting first. There is no unique key on this table, we just want to create
  // a record in all cases.
  $ndexp = chado_insert_record('nd_experiment', array(
    'nd_geolocation_id' => array('description' => $v['nd_geolocation']),
    'type_id' => $v['variant_type_id'],
  ));
  if ($ndexp) $ndexp_id = $ndexp['nd_experiment_id'];
  if (!$ndexp) {
    return drush_set_error(dt('Unable to create ND Experiment record.'));
  }

  // ND Experiment <=> Genotype.
  $status = genotypes_loader_helper_add_record_with_mode('ND Experiment Genotype Link', 'nd_experiment_genotype', $insert_only, array(
    'nd_experiment_id' => $ndexp_id,
    'genotype_id' => $genotype_id,
  ));
  if (!$status) return FALSE;

  // ND Experiment <=> Stock.
  $status = genotypes_loader_helper_add_record_with_mode('ND Experiment Sample Link', 'nd_experiment_stock', $insert_only, array(
    'nd_experiment_id' => $ndexp_id,
    'stock_id' => $v['stock_id'],
    'type_id' => $v['variant_type_id'],
  ));
  if (!$status) return FALSE;

  // ND Experiment <=> Project.
  $status = genotypes_loader_helper_add_record_with_mode('ND Experiment Project Link', 'nd_experiment_project', $insert_only, array(
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
 * See https://github.com/UofS-Pulse-Binfo/genotypes_loader/wiki/How-to-Store-your-Data#method-2-custom-genotype-call-table
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
 *    - meta_data: any additional data associated with a genotype, in the format of a JSON blob
 * @param $num_calls
 *   The number of genotype calls processed so far
 * @param $file_name
 *   The name of the output file for records to be inserted into the database in batches
 * @return
 *   TRUE if successful and FALSE otherwise.
 */
function genotypes_loader_helper_add_genotypes_genotype_call($v, $num_calls = 0, $file_name = NULL) {

  // Set some variables to abstract mode for genotypes_loader_helper_add_record_with_mode().
  $select_only = 0;
  $insert_only = 1;
  $both = 2;

  // Genotype.
  // The uniquename is required to be unique. By selecting based on it we ensure we don't
  // attempt to insert a duplicate genotype.
  $select = array(
    'uniquename' => $v['allele'],
  );
  $insert = array(
    'name' => $v['allele'],
    'uniquename' => $v['allele'],
    'description' => $v['allele'],
    'type_id' => $v['genotype_type_id'],
  );
  $genotype_id = genotypes_loader_helper_add_record_with_mode('Genotype', 'genotype', $both, $select, $insert);
  if (!$genotype_id) {
    tripal_report_error(
      'genotypes_loader',
      TRIPAL_ERROR,
      'ERROR: Could not insert the genotype into the genotype table. Values: '.print_r($v,TRUE), 
      array('@record_type' => $record_type, '@mode' => $mode)
    );
    return FALSE;
  }

  // Enter the genotype call by adding to a file and copy in as a batch
  $insert_values = array(
    'variant_id' => $v['variant_id'],
    'marker_id' => $v['marker_id'],
    'genotype_id' => $genotype_id,
    'project_id' => $v['project_id'],
    'stock_id' => $v['stock_id'],
    'meta_data' => $v['meta_data'],
  );

  $status = genotypes_loader_helper_copy_records($num_calls, 'Genotype Call', 'genotype_call', $insert_values, $file_name);

  return TRUE;
}

/**
 * Adds genotype records when using the Genotype Stock storage method. This is a
 * helper function for the genotype matrix loader.
 *
 * This function handles all the tables beyond the feature block and up to but not
 * including stock and project.
 * See https://github.com/UofS-Pulse-Binfo/genotypes_loader/wiki/How-to-Store-your-Data#method-3-via-stock-genotype-table
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
function genotypes_loader_helper_add_genotypes_stock_genotype($v) {

  // Set some variables to abstract mode for genotypes_loader_helper_add_record_with_mode().
  $select_only = 0;
  $insert_only = 1;
  $both = 2;

  // Genotype.
  $genotype_id = genotypes_loader_helper_add_record_with_mode('Genotype', 'genotype', $both, array(
    'name' => $v['marker_name'] . ': ' . $v['allele'],
    'uniquename' => str_replace(' ','_',$v['marker_name']) . '_' . $v['allele'],
    'description' => $v['allele'],
    'type_id' => $v['variant_type_id'],
  ));
  if (!$genotype_id) return FALSE;

  // Feature <=> Genotype Link.
  $feature_genotype_id = genotypes_loader_helper_add_record_with_mode('Marker Genotype Link', 'feature_genotype', $both, array(
    'feature_id' => $v['marker_id'],
    'genotype_id' => $genotype_id,
    'cvterm_id' => $v['variant_type_id'],
    'rank' => 0,
    'cgroup' => 0,
  ));
  if (!$feature_genotype_id) return FALSE;

  // Allele <=> Stock.
  $status = genotypes_loader_helper_add_record_with_mode('Genotype Sample Link', 'stock_genotype', $both, array(
    'genotype_id' => $genotype_id,
    'stock_id' => $v['stock_id'],
  ));
  if (!$status) return FALSE;

  return TRUE;
}

/**
 * Prints a progress bar to the screen in drush.
 *
 * @param $step_count
 *   The number of steps currently completed (or can interpret as the step currently on)
 * @param $num_steps
 *   The total number of steps needed to complete the progress bar
 * @param $message
 *   An optional message that allows the user to print a message to the screen next to the
 *   progress bar at a particular step.
 */
function genotypes_loader_print_progress($step_count, $num_steps, $message = "")
{
    // Calculate percentage of steps completed
    $percent_complete = ($step_count/$num_steps)*100;
    // Remove decimal places for simplicity
    $percent_complete = intval($percent_complete);

    $output .= 'Progress: [';
    // Determine the length of the bar
    for ($steps=0; $steps<$percent_complete; $steps=$steps+2)
    {
        $output .= '=';
    }
    // Determine the space remaining after the bar
    $steps_remaining = (100 - $steps);
    for ($i = 0; $i < $steps_remaining; $i = $i+2)
    {
        $output .= ' ';
    }
    // Output percentage then shift cursor to beginning of the line
    $output .= '] ' . $percent_complete . ' %  ' . $message . "\r";

    return $output;
}
