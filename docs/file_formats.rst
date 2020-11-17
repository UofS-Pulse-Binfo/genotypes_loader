File Formats
==============

Genotypes File
-----------------

This module supports loading of three types of genotype files:

1. VCF
^^^^^^^^

.. code::

  ##fileformat=VCFv4.0
  ##fileDate=20090805
  ##source=myImputationProgramV3.1
  ##reference=1000GenomesPilot-NCBI36
  ##phasing=partial
  ##INFO=<ID=NS,Number=1,Type=Integer,Description="Number	of	Samples	With	Data">
  ##INFO=<ID=DP,Number=1,Type=Integer,Description="Total	Depth">
  ##INFO=<ID=AF,Number=.,Type=Float,Description="Allele	Frequency">
  ##INFO=<ID=AA,Number=1,Type=String,Description="Ancestral	Allele">
  ##INFO=<ID=DB,Number=0,Type=Flag,Description="dbSNP	membership,	build	129">
  ##INFO=<ID=H2,Number=0,Type=Flag,Description="HapMap2	membership">
  ##FILTER=<ID=q10,Description="Quality	below	10">
  ##FILTER=<ID=s50,Description="Less	than	50%	of	samples	have	data">
  ##FORMAT=<ID=GT,Number=1,Type=String,Description="Genotype">
  ##FORMAT=<ID=GQ,Number=1,Type=Integer,Description="Genotype	Quality">
  ##FORMAT=<ID=DP,Number=1,Type=Integer,Description="Read	Depth">
  ##FORMAT=<ID=HQ,Number=2,Type=Integer,Description="Haplotype	Quality">
  #CHROM	POS	ID	REF	ALT	QUAL	FILTER	INFO	FORMAT	Ross	Prado	Ash
  1A	14370	.	G	A	29	PASS	NS=3;DP=14;AF=0.5;DB;H2	GT:GQ:DP:HQ	0|0:48:1:51,51	1|0:48:8:51,51	1/1:43:5:.,.
  1A	17330	.	T	A	3	q10	NS=3;DP=11;AF=0.017	GT:GQ:DP:HQ	0|0:49:3:58,50	0|1:3:5:65,3	0/0:41:3
  1A	1110696	rs6040355	A	G,T	67	PASS	NS=2;DP=10;AF=0.333,0.667;AA=T;DB	GT:GQ:DP:HQ	1|2:21:6:23,27	2|1:2:0:18,2	2/2:35:4
  1A	1230237	.	T	.	47	PASS	NS=3;DP=13;AA=T	GT:GQ:DP:HQ	0|0:54:7:56,60	0|0:48:4:51,51	0/0:61:2
  1A	11111	1subfield	C	A	50	PASS	A=1;B=2;C=3	GT	0/1	./.	1/1


2. Genotype Matrix
^^^^^^^^^^^^^^^^^^^^^

A tab-delimited data file where each line corresponds to a SNP and columns correspond to germplasm assayed. Expected columns: (1) Marker Name, (2) Chromosome Name, (3) Position on Chromosome, (4+) Sample Genotype Calls.

.. code::

  Marker name	Chromosome	Position	1048-8R	964a-46	Giftgi
  FcChr1Ap11111	1A	11111	CC	AC	AA
  FcChr1Ap22222	1A	22222	GG	GC	GG
  FcChr1Ap33333	1A	33333	TA	AA	GA

3. Genotype Flat-file
^^^^^^^^^^^^^^^^^^^^^^^

A tab delimited data file where each line is a genotypic call. Expected columns: (1) Marker name, (2) Chromosome Name, (3) Position on Chromosome, (4) Sample Name, (5) Genotype call.

.. code::

  Marker name	Chromosome	Position	Sample name	Genotype call
  FcChr1Ap11111	1A	11111	Ross	CC
  FcChr1Ap11111	1A	11111	Prado	CC
  FcChr1Ap11111	1A	11111	Ash	CC
  FcChr1Ap11111	1A	11111	Piero	CT
  FcChr1Ap11111	1A	11111	Tai	CC
  FcChr1Ap11111	1A	11111	Beverly	TC
  FcChr1Ap11111	1A	11111	Argent	CC
  FcChr1Ap11111	1A	11111	Trenus	TT
  FcChr1Ap11111	1A	11111	Zapelli	CC
  FcChr1Ap11111	1A	11111	Amato	CG

Samples File
----------------

All formats require a separate samples file describing the germplasm assayed. This file is expected to be a tab-delimited file with the following columns: (1) Sample name in the genotypes file, (2) Sample name, (3) Sample accession, (4) Germplasm name, (5) Germplasm accession.

The next two columns are optional: (6) Germplasm type (otherwise it is currently assumed to be of type 'Individual' from the stock_type cv) and (7) Organism (this allows multiple organisms in your genotypes file, assuming they have all been aligned to the same genome. Otherwise, the default value is the organism you specified as an option).

.. code::

  Sample name	Sample_name	Sample_Accession	Germplasm_name	Germplasm_Accession	Germplasm_Type	Organism
  Ross	Ross_110201	Catsam1	Ross	Catgerm1	Individual	Felis catus
  Prado	Prado_110201	Catsam2	Prado	Catgerm2	Individual	Felis catus
  Ash	Ash_110201	Catsam3	Ash	Catgerm3	Individual	Felis catus
  Piero	Piero_110201	Catsam4	Piero	Catgerm4	Individual	Felis catus
  Tai	Tai_110201	Catsam5	Tai	Catgerm5	Individual	Felis catus
  Beverly	Beverly_110201	Catsam6	Beverly	Catgerm6	Individual	Felis catus
  Argent	Argent_110201	Catsam7	Argent	Catgerm7	Individual	Felis catus
  Trenus	Trenus_110201	Catsam8	Trenus	Catgerm8	Individual	Felis catus
  Zapelli	Zapelli_110201	Catsam9	Zapelli	Catgerm9	Individual	Felis catus
  Amato Amato_110201 Catsam10 Amato Catgerm10 Individual Felis catus
