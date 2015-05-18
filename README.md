eos-shard
=========

A simple archive format for storing data/metadata pairs.

This format was created to satisfy the following requirements:

* Archives tons of files into a single binary format
* Fast random access to files based on a primary key
* Provides optional zlib (de)compression for individual files
* Convenient storage/access of both data and metadata
* Provides interface for inferring file content-type
* Provides GObject introspectable library for reading/writing shards
