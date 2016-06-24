<?php

/**
 * @file
 * Implements a drush command to load a VCF file
 */

/**
 * Implements hook_drush_command().
 */
function genotypes_loader_drush_command() {

  $items['load-VCF'] = array(
    'description' => 'Takes a VCF file and associates it with a LIMS record to link to germplasm in KnowPulse, then imports marker, variant and genotype data into KnowPulse.',
    'aliases' => array('load-vcf'),
    'arguments' => array(
      'input_file' => 'The filename of the VCF file for upload',
    ),
  );
  return $items;
}

/**
 * Callback for load-VCF command
 */
function drush_genotypes_loader_load_VCF($input_file = NULL) {
  // Check the file exists
  if (!$input_file) {
    return drush_set_error(dt('ERROR: No VCF file specified. See: \'drush help load-VCF\''));
  }
  // Otherwise, let's assume we need the full path for the file by using the cwd
  else {
    $current_directory = drush_cwd();
    $input_file = $current_directory . "/" . $input_file;
    // Test the file again, return error if we still can't find it.
    if (!file_exists($input_file)) {
      return drush_set_error(dt("ERROR: Could not locate $input_file\nPlease ensure you spelled your file correctly or try using the full path."));
    }
  }

  // Open the file and check the first line to ensure it is in VCF format
  // First line should be: ##fileformat=VCFv4.x
  $VCFfile = fopen($input_file, 'r');
  $first_line = fgets($VCFfile);
  if (!preg_match('/^##fileformat/', $first_line)) {
    return drush_set_error(dt("ERROR: First line of file $VCFfile does not meet VCF file requirements."));
  }
  // Iterate through remaining meta lines until we reach our header
  while(!feof($VCFfile)) {
    # Get each meta line to skip processing them
    $current_line = fgets($VCFfile);
    if (preg_match('/^#CHROM/', $current_line)) {
      $current_line = trim($current_line);
      $header = explode("\t",$current_line);
      break;
    }
  }
  // Now processing the SNP calls
  while(!feof($VCFfile)) {
    $current_line = fgetcsv($VCFfile, 0, "\t");
    if (empty($current_line)) continue;
    // Use the header to create the keys for each marker array
    $marker = array_combine($header, $current_line);
    /** Let's organize the FORMAT cell by storing it as an array
    @TODO: As an aside... how are we going to handle the information provided in the INFO field?
           This involves stats that are specific to the variant, not by SNP. Lacey suggests using
           the feature_prop table and storing as an array. The issue is whether or not we want to
           trust this information, since this is dependent on how the file was filtered. If individual
           SNPs are filtered out, was the INFO updated properly or is it even useful for anything other
           than for filtering? */
    $format_keys = [];
    if (preg_match('/:/', $marker['FORMAT'])) {
      $format_keys = explode(':',$marker['FORMAT']);
    }
    // For each sample, explode the format fields so that we can pull out the genotype directly
    for ($i=9;$i<count($header);$i++) {
      $sample = $header[$i];
      if ($format_keys) {
        $num_fk = count($format_keys);
        $format_fields = explode(":",$marker[$sample]);
        $num_ff = count($format_fields);
        // NOTE: Why the following comparison between format key and field counts?
        // Occasionally we may see that the format field will specify additional sub-fields than what is
        // actually present for that sample. This is because of questionably loose requirements for VCF:
        // missing sub-fields are shown as ".", except for trailing fields which can be dropped. Therefore,
        // if there are fewer format fields for a sample than expected, we will add onto the end of the
        // array the appropriate number of missing values.
        while ($num_fk > $num_ff)
        {
          array_push($format_fields,".");
          $num_ff = count($format_fields);
        }
        $geno_field_arr = array_combine($format_keys,$format_fields);
        // Now we nest the marker array with this new genotype fields array for each sample
        // Example: Marker -> Sample1 -> GT -> 0/1
        //                            -> DP -> 25
        $marker[$sample] = $geno_field_arr;
      } else {
        $genotype_field = [];
        $genotype_field[$marker['FORMAT']] = $marker[$sample];
        $marker[$sample] = $genotype_field;
      }
    }

    /**
      Now to save this in chado!
      Blueprint so I can pick it up on Monday:
      - Marker names and variant names need to be different. Format we're thinking:
        Marker: Lc<chrom>p<position>_GBS    Variant: Lc<chrom>p<position>
      - How to link this to an experiment?
      - Linking samples to stocks currently in KP through LIMS
    */

    $marker_name = 'Lc' . $marker['#CHROM'] . 'p' . $marker['POS'] . '_GBS';
    print_r("$marker_name => ");
    print_r($marker);
  }
}
