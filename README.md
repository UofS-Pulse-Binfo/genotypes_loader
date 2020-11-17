![Tripal Dependency](https://img.shields.io/badge/tripal-%3E=3.0-brightgreen)
![GitHub release (latest by date including pre-releases)](https://img.shields.io/github/v/release/UofS-Pulse-Binfo/genotypes_loader?include_prereleases)

[![Build Status](https://travis-ci.org/UofS-Pulse-Binfo/genotypes_loader.svg?branch=master)](https://travis-ci.org/UofS-Pulse-Binfo/genotypes_loader)
[![Maintainability](https://api.codeclimate.com/v1/badges/14f3972040ba32dff461/maintainability)](https://codeclimate.com/github/UofS-Pulse-Binfo/genotypes_loader/maintainability)
[![Test Coverage](https://api.codeclimate.com/v1/badges/14f3972040ba32dff461/test_coverage)](https://codeclimate.com/github/UofS-Pulse-Binfo/genotypes_loader/test_coverage)

# Genotypes Loader

This module provides a drush command to load genotypic data from a variety of file formats including Variant Call Format (VCF), Genotype Matrix and Genotype Flat-File formats as described below. It stores genotype calls in the custom chado-esque genotype_call table; whereas, all other meta data is stored in a chado-compliant manner.

## Dependencies
1. [Tripal 3.x](https://github.com/tripal/tripal) ([Installation Instructions](https://tripal.readthedocs.io/en/latest/user_guide.html))
2. PostgreSQL 9.3 (9.4+ recommended; tested with 11.3)

## Installation
This module is installed by cloning it and it's dependencies in `[your drupal site]/sites/all/modules` and enabling it through the Drupal Administrative UI. Specifically, once you have a working Tripal environment:

```
cd [drupal root]/sites/all/modules
git clone https://github.com/uofs-pulse-binfo/genotypes_loader
drush pm-enable genotypes_loader
```

## Features
- Extensive configuration allowing for flexiblity in ontology terms used.
- Drush command for easy import of VCF, genotypes matrix and flatfile formats.
- Both prompt and option support for drush command, making it easy for scripting, as well as, human interaction.
- genotype_call table for efficient data storage.

## Documentation [![Documentation Status](https://readthedocs.org/projects/genotypes-loader/badge/?version=latest)](https://genotypes-loader.readthedocs.io/en/latest/?badge=latest)

Please visit our [online documentation](https://genotypes-loader.readthedocs.io/en/latest/index.html) to learn more about installation, usage and features.

## Funding

This work is supported by Saskatchewan Pulse Growers [grant: BRE1516, BRE0601], Western Grains Research Foundation, Genome Canada [grant: 8302, 16302], Government of Saskatchewan [grant: 20150331], and the University of Saskatchewan.

## Citation 

Caron, C. and Sanderson, L.A. (2020). Genotypes Loader: Efficient large-scale genotypic data import into Chado. Version 1.0. University of Saskatchewan, Pulse Crop Research Group, Saskatoon, SK, Canada.
