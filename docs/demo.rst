
Demonstration
===============

This demonstration will walk you through loading the sample files that come with the module. This is meant to show you the full process of loading a VCF file using this module and also as a means to evaluate the module.

.. warning::

	These instructions should only be followed on a DEVELOPMENT site. Data will be inserted into your database.

Step 1: Preparation
---------------------

Organism
^^^^^^^^^^

Before loading a VCF file you need to ensure your have a chado organism record for the species the data is from. This can be done through the administrative interface by navigating to Content > Tripal Content > Add Tripal Content and clicking on Organism. For our sample file we need to add `Citrus sinensis` as they did in the `Tripal tutorial <https://tripal.readthedocs.io/en/latest/user_guide/example_genomics/organisms.html>`_.

.. note::

	The system does handle using a different species for the genomic backbone and variants then for the germplasm the data is associated with. For example, when genotyping wild species you will often align your data against a cultivated reference. In this case, you would supply the cultivated species for the ``--organism`` parameter and then the wild species would be indicated with each individual.

Genome
^^^^^^^^
Next, we need to import the genome our genotypic data was aligned to. This can be done through the build-in Tripal GFF3 and FASTA importers.
