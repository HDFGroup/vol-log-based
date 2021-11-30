## Log-based VOL examples 

### Building Steps
* Build log-based VOL plugin.
  + See [README.md](../README.md) under the root folder
* Compile example programs.
  + Example programs are part of test programs
  + Run make check under the examples folder to build and run all example programs
    ```
    % cd examples
    % make check
    ```
  + Run make tests under the examples folder to build all example programs without running

### Example programs
  * Log-based VOL examples
    + create_open
      + Demonstrates the way to use log-based vol programmatically
    + dwrite_n
      + Demonstrates the use of H5Dwrite_n API of log-based VOL.
    + non_blocking
      + Demonstrates the way to set non-blocking dataset transfer property
      + It tells log-based VOL to keep the data in the user buffer instead of making a copy in the internal buffer
  * Supported HDF5 examples modified to use log-based VOL
    + h5_attribute
      + Example of H5A* APIs
    + h5_crtdat
      + Example to create a dataset
    + h5_crtatt 
      + Example to create an attribute
    + h5_crtgrp 
      + Example to create a group
    + h5_crtgrpar 
      + Example to create an attribute in a group
    + h5_crtgrpd 
      + Example to create a dataset in a group
    + h5_group 
      + Example of H5G* APIs
    + h5_interm_group 
      + Creating a group and all its parent groups if not already exist
    + h5_rdwt 
      + Open a file for reading and writing
    + h5_write
      + Writing to a HDF5 dataset 
    + h5_read 
      + Reading from a HDF5 dataset 
    + h5_select 
      + Demonstrate different way to set HDF5 dataspace selection
    + h5_subset
      + Writing and reading part of a HDF5 dataset