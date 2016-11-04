# Genotypes Loader

This module provides a drush command to load genotypic data from a variety of file formats. Data is saved in GMOD Chado with three separate methods supported (See Data Storage below).

## Command
 - Command: load-genotypes
 - Alias: load-geno
 - Arguments:
   - input-file: The filename of the matrix file for upload
   - sample-file: The filename of a tab-delimited file specifying for each sample name in the genotypes file: the name of the stock in the database, the stock accession ID, the name of the germplasm, and the germplasm Accession ID. See "samples.list" in the sample_files folder.
 - Options:
   - variant-type: The Sequence Ontology (SO) term name that describes the type of variants in the file (eg. SNP, MNP, indel).
   - marker-type: The Sequence Ontology (SO) term name that describes the marker technology used to generate the genotypes in the file (e.g. "Exome Capture", "GBS", "KASPar", etc.).
   - ndgeolocation: A meaningful location associated with this natural diversity experiment. For example, this could be the location the assay was completed in, the location the germplasm collection was from, or the location the markers were developed at. This should be the description field of your ndgeolocation.
   - organism: The organism to which the genotypes are associated with.
   - project-name: All genotypes will be grouped via a project to allow users to specify a particular dataset.

**This loader supports 3 different file formats (described under file formats below) and will auto-detect which format you have provided.**

Example Usage:
 - Load a genotype matrix file (mymatrix.tsv) using the sample/germplasm information provided in samples.list. With this example, you will be prompted to enter each of the options listed above.
   - `drush load-genotypes mymatrix.tsv samples.list`
 - Load a VCF file (mygenotypes.vcf) using the sample/germplasm information provided in samples.list but provide the command with all the options upfront to avoid prompting.
   - `drush load-genotypes mygenotypes.vcf samples.list --organism="Lens culinaris" --variant-type="SNP" --marker-type="genetic_marker" --project-name="My SNP Discovery Project" --ndgeolocation="here"`

## Data Storage
Genotypes loaded by this module can be stored in chado in via one of the following admin selected methods:

| Method | Name           | Custom Tables | Supports Meta-data | # Tables | Comments                                                                                                                                          |
|--------|----------------|---------------|--------------------|----------|---------------------------------------------------------------------------------------------------------------------------------------------------|
| 1      | ND Experiment  | No            | Yes                | 14       | Not suitable beyond 3 million genotype calls.                                                                                                     |
| 2      | Genotype Call  | Yes           | Yes                | 10       | Most efficient; although it touches the same number of tables as Method #3 there are less records per genotype call.                              |
| 3      | Stock Genotype | No            | No                 | 10       | A good alternative if you don't want to use custom tables but have a lot of data. Similar efficiency to Method #2 but less support for meta-data. |

For more information see [ND Genotypes: How to Store your Data](https://github.com/UofS-Pulse-Binfo/nd_genotypes/wiki/How-to-Store-your-Data).
