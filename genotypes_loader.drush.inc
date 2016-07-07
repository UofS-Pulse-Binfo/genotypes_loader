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
    'description' => 'Takes a VCF file and associates it with a project in KnowPulse, then imports marker, variant and genotype data into KnowPulse.',
    'aliases' => array('load-vcf'),
    'arguments' => array(
      'input_file' => 'The filename of the VCF file for upload',
    ),
  );

  $items['load-genotype-matrix'] = array(
    'description' => 'Takes a genotype matrix and associates it with a project in Knowpulse, and stores marker, variant and genotype data.',
    'aliases' => array('load-matrix'),
    'arguments' => array(
      'input_file' => 'The filename of the matrix file for upload',
    ),
    'options' => array(
      'organism' => 'The organism to which the genotypes are associated with',
      'variant_type' => 'The Sequence Ontology (SO) term name that describes the type of variants in the file (eg. SNP, MNP, indel)',
      'marker_type' => 'The Sequence Ontology (SO) term name that describes the marker technology used to generate the genotypes in the file (e.g. "Exome Capture", "GBS", "KASPar", etc.).',
      'nd_geolocation' => 'A meaningful location associated with this natural diversity experiment. For example, this could be the location the assay was completed in, the location the germplasm collection was from, or the location the markers were developed at. This should be the description field of your nd_geolocation.',

    ),
  );

  return $items;
}

function drush_genotypes_loader_load_genotype_matrix($input_file = NULL) {
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
    // @TODO: Here, we know the user has provided an organism as an option. Do we need to look up
    // the organism ID at this point?
  }

  // Variant type
  $variant_type = drush_get_option('variant_type');
  if (!$variant_type) {
    //$values = array('name' => 'sequence');
    //$cv = chado_select_record('cv', array('cv_id'), $values);
    //$cv_id = $cv[0]->cv_id;
    $variant_type = drush_prompt(dt('Please enter the Sequence Ontology (SO) term name that describes the type of variants in the file (eg. SNP, MNP, indel).'));
    // @TODO: Validate that this is a SO term?
  }

  // Marker type
  $marker_type = drush_get_option('marker_type');
  if (!$marker_type) {
    $marker_type = drush_prompt(dt('Please enter the Sequence Ontology (SO) term name that describes the marker technology used to generate the genotypes in the file (e.g. "Exome Capture", "GBS", "KASPar", etc.).'));
    // @TODO: Validate that this is a SO term?
  }

  // Project
  // @TODO: Drush_choice or drush_prompt?

  // ND Geolocation
  $nd_geolocation = drush_get_option('nd_geolocation');
  if (!$nd_geolocation) {
    $nd_geolocation = drush_prompt(dt('Please provide a meaningful location associated with this natural diversity experiment. For example, this could be the location the assay was completed in, the location the germplasm collection was from, or the location the markers were developed at. This should be the description field of your nd_geolocation.'));
    // @TODO: Validate?
  }


  // Check the file exists
//   if (!$input_file) {
//     return drush_set_error(dt('ERROR: No matrix file specified. See: \'drush help load-genotype-matrix\''));
//   }
//   // Otherwise, let's assume we need the full path for the file by using the cwd
//   else {
//     $current_directory = drush_cwd();
//     $input_file = $current_directory . "/" . $input_file;
//     // Test the file again, return error if we still can't find it.
//     if (!file_exists($input_file)) {
//       return drush_set_error(dt("ERROR: Could not locate $input_file\nPlease ensure you spelled your file correctly or try using the full path."));
//     }
//   }

  //
  print_r("Organism : $organism\n");
  print_r("Variant type: $variant_type\n");
  print_r("Marker type: $marker_type\n");
  print_r("Project: $project\n");
  print_r("ND_geolocation: $nd_geolocation\n");


  //include('nd_genotypes.loader.inc');
  //nd_genotypes_load_genotype_matrix($input_file);
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
