
Usage
=====

Drush Command
-------------

 - Command: load-genotypes
 - Alias: load-geno
 - Arguments:
     - input-file: The filename of the matrix file for upload
     - sample-file: The filename of a tab-delimited file specifying for each sample name in the genotypes file: the name of the stock in the database, the stock accession ID, the name of the germplasm, the germplasm Accession ID, type fo germplasm, and organism (optional). See "samples.list" in the sample_files folder for an example.
 - Options:
     - variant-type: The Sequence Ontology (SO) term name that describes the type of variants in the file (eg. SNP, MNP, indel).
     - marker-type: A free-text title that describes the marker technology used to generate the genotypes in the file (e.g. "Exome Capture", "GBS", "KASPar", etc.).
     - ndgeolocation: A meaningful location associated with this natural diversity experiment. For example, this could be the location the assay was completed in, the location the germplasm collection was from, or the location the markers were developed at. This should be the description field of your ndgeolocation.
     - organism: The organism of the reference genome which was used for aligning reads to call the variants. If there is an empty value in the "Organism" column of the sample file, the loader will default to this parameter.
     - project-name: All genotypes will be grouped via a project to allow users to specify a particular dataset.

This loader supports 3 different file formats (described under file formats below) and will auto-detect which format you have provided.

Example Usage
-------------

- Load a genotype matrix file (mymatrix.tsv) using the sample/germplasm information provided in samples.list. With this example, you will be prompted to enter each of the options listed above.

.. code::

  drush load-genotypes mymatrix.tsv samples.list


- Load a VCF file (mygenotypes.vcf) using the sample/germplasm information provided in samples.list but provide the command with all the options upfront to avoid prompting.

.. code::

  drush load-genotypes mygenotypes.vcf samples.list --organism="Lens culinaris" --variant-type="SNP" --marker-type="genetic_marker" --project-name="My SNP Discovery Project" --ndgeolocation="here"``
