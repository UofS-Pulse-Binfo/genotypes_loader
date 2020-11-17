
Configuration
=============

Controlled Vocabulary Terms
---------------------------

This module currently expects the following controlled vocabulary terms already exist in chado:

+----------------------+----------------------+--------------------------+
| Purpose              | Term Name            | Controlled Vocabulary    |
+======================+======================+==========================+
| Sample type          | genomic_DNA          | sequence                 |
+----------------------+----------------------+--------------------------+
| Sample => germplasm  | is_extracted_from    | stock_relationship       |
| relationship         |                      |                          |
+----------------------+----------------------+--------------------------+
| Marker => variant    | is_marker_of         | stock_relationship       |
| relationship         |                      |                          |
+----------------------+----------------------+--------------------------+
| Marker type          | Indicated by user    | sequence                 |
+----------------------+----------------------+--------------------------+
| Variant type         | Indicated by user    | sequence                 |
+----------------------+----------------------+--------------------------+
| Property type for    | marker_type          | feature_property         |
| free-text marker     |                      |                          |
| description          |                      |                          |
+----------------------+----------------------+--------------------------+

You can configure these terms through a settings form under module configuration. In the future, you will also be able to configure the controlled vocabularies for these terms as well.

Database Write Options
----------------------

There is configuration to specify whether you wan the loader to insert only, update only or update/insert as needed.
