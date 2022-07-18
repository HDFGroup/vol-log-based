## Log-based VOL source files

This folder contains all source files of log-based VOL implementation.
The VOL callback functions are organized into sub-classes based on the type of HDF5 data object they handle.
The callback functions of each sub-class are implemented in separate source files.
Internal helper functions used by the callback function are also implemented in separate source files.

The source files are named according to the purpose of their content.
The file implementing callback functions for each VOL sub-class are named `H5VL_log_<class_name>.cpp/hpp`.
For eaxmple, functions in the attribute sub-class (H5VL_attr_class_t) are implemeted in `H5VL_log_att.cpp` with their signatures defined in `H5VL_log_att.hpp`. Callback functions defined in `H5VL_log_<class_name>.cpp/hpp` are functions required by HDF5 to implement any working VOL. 
The source files containing internal helper functions used by the callbacks are named `H5VL_log_<class_name>i.cpp/hpp`.
The letter 'i' in the file names means internal.
For example, `H5VL_log_atti.cpp/hpp` contain functions used by attribute callback functions. 
Functions and componenets that is not related to a particular sub-class are placed in source files named `H5VL_logi_<component_name>.cpp/hpp`.
For example, `H5VL_logi_err.cpp/hpp` contain functions for error checking and handling.

In the implementations, variables `uo` and `uvlid` are frequently used. An HDF5 VOL (log-based VOL or not) can be stacked upon another HDF5 VOL, and the log-based VOL must also be stacked upon another VOL (called "under VOL", e.g. the native HDF5 VOL). This means most of the log-based VOL callback functions need to pass a user's request to the "under VOL", whose `id` is specified `uvlid` (under VOL ID). `uo` is the corresponding object opened/used by the under VOL.

### List of source files
* H5VL_log.cpp/h
  + Additional user APIs defined specifically by Log-based VOL
* H5VL_log_att.cpp/hpp
  + VOL callback functions related to HDF5 attributes (`H5VL_attr_class_t`)
* H5VL_log_atti.cpp/hpp
  + Internal helper functions used by `H5VL_log_att.cpp/hpp` to handle attribute operations.
* H5VL_log_blob.cpp/hpp
  + VOL callback functions related to HDF5 BLOB (Binary Large Object) (`H5VL_blob_class_t`)
* H5VL_log_dataset.cpp/hpp
  + VOL callback functions related to HDF5 datasets (`H5VL_dataset_class_t`).
* H5VL_log_dataseti.cpp/hpp
  + Internal helper functions used by `H5VL_log_dataset.cpp/hpp` to handle dataset operations
* H5VL_log_datatype.cpp/hpp
  + VOL callback functions related to HDF5 datatypes (`H5VL_datatype_class_t`).
* H5VL_log_datatypei.cpp/hpp
  + Internal helper functions used by `H5VL_log_datatype.cpp/hpp` to handle datatype operations
* H5VL_log_file.cpp/hpp
  + VOL callback functions related to HDF5 file objects (`H5VL_file_class_t`).
* H5VL_log_filei.cpp/hpp
  + Internal helper functions used by `H5VL_log_file.cpp/hpp` to handle file operations
* H5VL_log_filei_meta.cpp/hpp
  + Internal helper functions used by `H5VL_log_filei.cpp/hpp` to handle file metadata operations
* H5VL_log_group.cpp/hpp
  + VOL callback functions related to HDF5 groups (`H5VL_group_class_t`)
* H5VL_log_groupi.cpp/hpp
  + Not in use. Empty
* H5VL_log_info.cpp/hpp
  + VOL callback functions related to HDF5 info (`H5VL_info_class_t`)
* H5VL_log_introspect.cpp/hpp
  + VOL callback functions related to VOL connector introspection (`H5VL_introspect_class_t`)
* H5VL_log_link.cpp/hpp
  + VOL callback functions related to HDF5 links (`H5VL_link_class_t`)
* H5VL_log_linki.cpp/hpp
  + Internal functions used by `H5VL_log_link.cpp/hpp` to handle link operations
* H5VL_log_main.cpp/hpp
  + Log-based VOL connector object and other VOL callback functions not bellonging to a sub-class
* H5VL_log_obj.cpp/hpp
  + VOL callback functions related to HDF5 objects (`H5VL_object_class_t`)
* H5VL_log_obji.cpp/hpp
  + Internal functions used by `H5VL_log_obj.cpp/hpp` to handle HDF5 object operations.
* H5VL_log_req.cpp/hpp
  + VOL callback functions related to aysnc operations (`H5VL_request_class_t`)
  + Not fully implemented
  + Log-based VOL does not support async operation
* H5VL_log_reqi.cpp/hpp
  + Internal functions used by `H5VL_log_reqi.cpp/hpp` to handle HDF5 requests
  + Not fully implemented
  + Log-based VOL does not support async operation
* H5VL_log_token.cpp/hpp
  + H5VL_token_class_t VOL callback functions
* H5VL_log_wrap.cpp/hpp
  + VOL callback functions related to wrapping/upwrapping objects and contexts when passing them up and down the VOL chain. (`H5VL_wrap_class_t`)
* H5VL_logi.cpp/hpp
  + Definition of log-based VOL implementation of each VOL API sub-class
* H5VL_logi_dataspace.cpp/hpp
  + Internal functions used to parse dataspace selections
* H5VL_logi_debug.cpp/hpp
  + Internal helper functions for debugging
* H5VL_logi_err.cpp/hpp
  + Internal helper functions for error checking and handling
* H5VL_logi_filter.cpp/hpp
  + Definition of log-based VOL filter class and filter pipeline
* H5VL_logi_filter_deflate.cpp/hpp
  + Deflate (zlib) filter implementation
* H5VL_logi_idx.cpp/hpp
  + Difinition of metadata index class used to search for intersecting log entries for handling read reqeusts
* H5VL_logi_idx_compact.cpp/hpp
  + Array based metadata index with optimizations to reduce memory footprint
* H5VL_logi_idx_list.cpp/hpp
  + Array based metadata index
* H5VL_logi_mem.cpp/hpp
  + Internal helper functions for memory allocation
* H5VL_logi_meta.cpp/hpp
  + Internal helper functions for encoding and decoding the metadata entries
* H5VL_logi_nb.cpp/hpp
  + Internal functions for flushing staged dataset read/write requests
* H5VL_logi_profiling.m4/m4h
  + Helper functions for measuring time spent on individual steps
* H5VL_logi_util.cpp/hpp
  + Misc helper functions
* H5VL_logi_wrapper.cpp/hpp
  + Not in use
* H5VL_logi_zip.cpp/hpp
  + Internal helper functions for operating the zlib library
