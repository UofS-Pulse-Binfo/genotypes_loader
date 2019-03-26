
Chado Schema and Extensions
===========================

There are currently two ways to store your genotypic data in Chado v1.3 with this module providing a third, more efficient way. You can see a comparison of the various methods below which should make it clear why we’ve gone with the storage method we have.

Comparison of Methods
---------------------

+--------+----------------+---------------+--------------------+----------+---------------------------------+
| Method | Name           | Custom Tables | Supports Meta-data | # Tables | Comments                        |
+========+================+===============+====================+==========+=================================+
| 1      | ND Experiment  | No            | Yes                | 14       | Not suitable beyond 3 million   | 
|        |                |               |                    |          | genotype calls.                 |
+--------+----------------+---------------+--------------------+----------+---------------------------------+ 
| 2      | Stock Genotype | No            | No                 | 10       | A good alternative if you don't |
|        |                |               |                    |          | want to use custom tables but   |
|        |                |               |                    |          | have a lot of data. Similar     |
|        |                |               |                    |          | efficiency to Method #2 but     |
|        |                |               |                    |          | less support for meta-data.     |
+--------+----------------+---------------+--------------------+----------+---------------------------------+
| 3      | Genotype Call  | Yes           | Yes                | 10       | Most efficient; although it     |
|        |                |               |                    |          | touches the same number of      |
|        |                |               |                    |          | tables as Method #3 there are   | 
|        |                |               |                    |          | less records per genotype call  |
+--------+----------------+---------------+--------------------+----------+---------------------------------+

**All three methods store Markers & Variants in the same way.** For the purposes of this module, a variant is a location on the genome where variation has been detected and has a type of SNP, MNP, Indel, etc. A marker then indicates which method the genotype calls associated with it were determined by. For example, you may have a variant on Chromosome 1 at position 45678 that you detected variation through two different methods. Each method would be indicated as a marker and all the genotype calls detected by that method would be attached to the appropriate marker and not directly to the variant. This has been determined necessary since the level of trust and how you interpret any quality meta-data will depend on the method.

Our Method: Custom Genotype Call Table
--------------------------------------

.. image:: http://creately.com/jupiter/diagram/image/ipkbi8wt

Now, lets consider the same example as in Method 1 (one VCF line with three alleles and six samples):

+-----------+----------------------+-------------------------------+-------------------------------------------+
| # Records | Tables               | Example                       | Explanation                               |
+===========+======================+===============================+===========================================+
| 2         | feature              | "LcChr1p555" and              | One each for variant and marker where the |
|           |                      | "LcChr1p555 GBS Marker"       | variant may already exist.                |
+-----------+----------------------+-------------------------------+-------------------------------------------+
| 2         | featureloc           | Chr1:554-555 for each.        | Locate each of the variant and marker on  |
|           |                      |                               | the chromsome.                            |
+-----------+----------------------+-------------------------------+-------------------------------------------+
| 1         | feature_relationship | "LcChr1p555 GBS Marker"       | Link the marker and variant.              |
|           |                      | is_marker_of "LcChr1p555"     |                                           |
+-----------+----------------------+-------------------------------+-------------------------------------------+
| 6         | genotype_call        | All Foreign Keys with the     | This links the marker, variant, allele    |
|           |                      | exception of any quality      | call, stock and project all in one and    |
|           |                      | information you want to store | stores any addition quality information   |
|           |                      | in the meta-data column       | in the meta-data column.                  |
+-----------+----------------------+-------------------------------+-------------------------------------------+

**Total: 11 records per line in a VCF with only 6 stocks and 3 alleles per variant.**

Notice how efficient this method is. This is because (1) most of the foreign key connections are taking place in a single table (genotype_call) and (2) there now only needs to be a single record in the genotype table for "AA" rather than one record per marker using the previous method. For further comparison, the same 100,000 line VCF file would now only take 1,100,000 records to store not including the records for your chromosomes, which already exist, those for your stocks, only 6 per file, and those for you alleles (genotype table), which likely already exist. Furthermore, storing meta-data doesn't increase the number of records like it would in the first method.

.. note::
  For more information about the two methods supported by core Chado, see the `ND Genotypes documentation on schema <https://nd-genotypes.readthedocs.io/en/latest/data_storage/schema.html>`_.