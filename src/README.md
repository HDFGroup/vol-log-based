## Log-based VOL source files

This folder contains all source files of log-based VOL implementation.
The VOL callback functions are organized into sub-classes based on the type of HDF5 data object they handle.
The callback functions of each sub-class are implemented in separate source files.
Internal helper functions used by the callback function are also implemented in separate source files.

The source files are named according to the purpose of their content.
The file implementing callback functions for each VOL sub-class are named H5VL_log_<class_name>.cpp/hpp.
For eaxmple, functions in the attribute sub-class (H5VL_attr_class_t) are implemeted in H5VL_log_att.cpp with their signatures defined in H5VL_log_att.hpp.
The source files containing internal helper functions used by the callbacks are named H5VL_log_<class_name>i.cpp/hpp.
The letter 'i' in the file name means internal.
For example, H5VL_log_atti.cpp/hpp contain functions used by attribute callback functions. 
Functions and componenets that is not related to a particular sub-class are placed in source files named H5VL_logi_<component_name>.cpp/hpp.
For example, H5VL_logi_err.cpp/hpp contain functions for error checking and handling.

### List of source files
* H5VL_log.cpp/hpp
  + Log-based VOL specific user APIs
    + H5Dwrite_n/H5Dread_n
* H5VL_log_att.cpp/hpp
  + H5VL_attr_class_t VOL callback functions
* H5VL_log_atti.cpp/hpp
  + Helper functions for handling attributes
* H5VL_log_blob.cpp/hpp
  + H5VL_blob_class_t VOL callback functions
* H5VL_log_dataset.cpp/hpp
  + H5VL_dataset_class_t VOL callback functions
* H5VL_log_dataseti.cpp/hpp
  + Internal functions used to handle dataset operations
* H5VL_log_datatype.cpp/hpp
  + H5VL_datatype_class_t VOL callback functions
* H5VL_log_datatypei.cpp/hpp
  + Internal functions related to datatype operations
* H5VL_log_file.cpp/hpp
  + H5VL_file_class_t VOL callback functions
* H5VL_log_filei.cpp/hpp
  + Internal functions used to handle file operations
* H5VL_log_filei_meta.cpp/hpp
  + Internal functions used to handle log metadata
* H5VL_log_group.cpp/hpp
  + H5VL_group_class_t VOL callback functions
* H5VL_log_groupi.cpp/hpp
  + Not in use. Empty
* H5VL_log_info.cpp/hpp
  + H5VL_info_class_t VOL callback functions
* H5VL_log_introspect.cpp/hpp
  + H5VL_introspect_class_t VOL callback functions
* H5VL_log_link.cpp/hpp
  + H5VL_link_class_t VOL callback functions
* H5VL_log_linki.cpp/hpp
  + Internal functions used to handle link operaations
* H5VL_log_main.cpp/hpp
  + Log-based VOL connector object and other VOL callback functions not bellonging to a sub-class
* H5VL_log_obj.cpp/hpp
  + H5VL_object_class_t VOL callback functions
* H5VL_log_obji.cpp/hpp
  + Internal functions used to handle HDF5 objects
* H5VL_log_req.cpp/hpp
  + H5VL_request_class_t VOL callback functions
  + Not fully implemented
  + Log-based VOL does not support async operation
* H5VL_log_reqi.cpp/hpp
  + Internal functions used to handle HDF5 requests
  + Not fully implemented
  + Log-based VOL does not support async operation
* H5VL_log_token.cpp/hpp
  + H5VL_token_class_t VOL callback functions
* H5VL_log_wrap.cpp/hpp
  + H5VL_wrap_class_t VOL callback functions
* H5VL_logi.cpp/hpp
  + Definition of log-based VOL implementation of each VOL API sub-class
* H5VL_logi_dataspace.cpp/hpp
  + Internal functions used to parse dataspace selections
* H5VL_logi_debug.cpp/hpp
  + Helper functions for debugging
* H5VL_logi_err.cpp/hpp
  + Helper functions for error checking and handling
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
  + Helper functions for memory allocation
* H5VL_logi_meta.cpp/hpp
  + Helper functions for encoding and decoding the metadata entries
* H5VL_logi_nb.cpp/hpp
  + Internal functions for flushing staged dataset read/write requests
* H5VL_logi_profiling.m4/m4h
  + Helper functions for measuring time spent on individual steps
* H5VL_logi_util.cpp/hpp
  + Misc helper functions
* H5VL_logi_wrapper.cpp/hpp
  + Not in use
* H5VL_logi_zip.cpp/hpp
  + Helper functions for operating the zlib library
