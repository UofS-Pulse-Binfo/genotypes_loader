
Demonstration
===============

This demonstration will walk you through loading the sample files that come with the module. This is meant to show you the full process of loading a VCF file using this module and also as a means to evaluate the module.

.. warning::

	These instructions should only be followed on a DEVELOPMENT site. Data will be inserted into your database.

Step 1: Preparation
---------------------

Organism
^^^^^^^^^^

Before loading a VCF file you need to ensure your have a chado organism record for the species the data is from. This can be done through the administrative interface by navigating to Content > Tripal Content > Add Tripal Content and clicking on Organism. For our sample file we need to add `Citrus sinensis` as they did in the `Tripal Tutorial: Organism <https://tripal.readthedocs.io/en/latest/user_guide/example_genomics/organisms.html>`_.

.. note::

	The system does handle using a different species for the genomic backbone and variants then for the germplasm the data is associated with. For example, when genotyping wild species you will often align your data against a cultivated reference. In this case, you would supply the cultivated species for the ``--organism`` parameter and then the wild species would be indicated with each individual.

Genome
^^^^^^^^

Next, we need to import the genome our genotypic data was aligned to. This can be done through the built-in Tripal GFF3 and/or FASTA importers as shown in the `Tripal Tutorial: Genomes & Genes <https://tripal.readthedocs.io/en/latest/user_guide/example_genomics/genomes_genes.html>`_. Our genotypic data is aligned to the Citrus sinensis scaffold00001 imported in the linked Tripal tutorial.

Project
^^^^^^^^^^

All genotypic data points from a single file are grouped using a chado project. To create a chado project go to Content > Tripal Content > Add Tripal Content and then select "Project" under General.

 - **Name**: Citrus Demonstration Genotypic Data
 - **Description**: This project contains demonstration data imported via the University of Saskatchewan, Pulse Bioinformatcis Genotypes Loader. This data is not real and should not be used in analysis.

Step 2: Prepare the Files
---------------------------

This has already been done for you with the VCF file at ``sample_files/sample.vcf`` and the associated germplasm samples file at ``sample_files/samples.list``. For importing your own data, any VCF file following the VCF 4+ specification. The samples file then describes each of the germplasm samples in the VCF file with one row per sample. Full information on these file formats is available under "File Formats".

Step 3: Import genotypic data
-------------------------------

Now we bring all that preparation into a single command to start the import process.

.. code::

  cd $DRUPAL_ROOT/sites/all/modules/genotypes_loader/sample_files
  drush load-genotypes sample.vcf samples.list --organism="Citrus sinensis" \
    --variant-type="SNP" --marker-type="genetic_marker" \
    --project-name="Citrus Demonstration Genotypic Data"

.. note::

  You will be prompted for your database user password during this process.
