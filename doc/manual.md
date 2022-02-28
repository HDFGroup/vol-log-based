# Log-based VOL user guide

This file explains new features and APIs introduced by the log-based VOL.

## APIs
### Dataset APIs (H5D*)
* H5Dread_n (did, mtype, n, starts, counts, dxplid, buf)
  + Read multiple blocks from a dataset.
  + Analogous to the ncmpi_get_varn* APIs in PnetCDF.
  + Equivalent to calling H5Dread n times with dataspace selection set to starts[i] and counts[i] on i-th call.
  + Syntax
    + H5Dread_n (did, mtype, n, starts, counts, dxplid, buf)
      + IN did initial address of send buffer (choice)
      + IN mtype number of elements in send buffer (non-negative integer)
      + IN n number of blocks to read
      + IN starts starting coordinate of the blocks to read in the dataset dataspace.
        + starts[i] - starting coordinate of the i-th block to read in the dataset dataspace.
      + IN counts the size of the blocks to read.
        + counts[i] - size of the i-th block to read.
      + IN dxplid dataset transfer property list ID.
      + OUT buf data read.
        + The data in buf is ordered according to the order of blocks.
        + Data for block i followed immediately by data for block i + 1.
  + C binding
    ```
        herr_t H5Dread_n (hid_t did,
                        hid_t mem_type_id,
                        int n,
                        hsize_t **starts,
                        hsize_t **counts,
                        hid_t dxplid,
                        void *buf);
    ```
* H5Dwrite_n
  + Write multiple blocks to a dataset.
  + Analogous to the ncmpi_put_varn* APIs in PnetCDF.
  + Equivalent to calling H5Dwrite n times with dataspace selection set to starts[i] and counts[i] on i-th call.
  + Syntax
    + H5Dwrite_n (did, mtype, n, starts, counts, dxplid, buf)
      + IN did initial address of send buffer (choice)
      + IN mtype number of elements in send buffer (non-negative integer)
      + IN n number of blocks to write
      + IN starts starting coordinate of the blocks to write in the dataset dataspace.
        + starts[i] - starting coordinate of the i-th block to write in the dataset dataspace.
      + IN counts the size of the blocks to write.
        + counts[i] - size of the i-th block to write.
      + IN dxplid dataset transfer property list ID.
      + IN buf data to write.
        + The data in buf is ordered according to the order of blocks.
        + Data for block i followed immediately by data for block i + 1.
  + C binding
    ```
        herr_t H5Dwrite_n (hid_t did,
                        hid_t mem_type_id,
                        int n,
                        hsize_t **starts,
                        hsize_t **counts,
                        hid_t dxplid,
                        void *buf);
    ```

### Properties APIs (H5P*)
* H5Pset_nonblocking
  + Set whether an H5Dwrite or H5Dread call is blocking or non-blocking in a dataset transfer property list
  + Syntax
    + H5Pset_nonblocking (plist, nonblocking)
      + IN plist ID of the dataset transfer property list to attach the setting
      + IN nonblocking Whether to perform blocking or non-blocking I/O
        + H5VL_LOG_REQ_BLOCKING (default)
          + The user buffer can be modified after H5Dwrite returns
          + The data is available in the user buffer after H5Dread returns
        + H5VL_LOG_REQ_NONBLOCKING
          + The user buffer shall not be modified before H5Fflush returns
          + The data is not available in the user buffer before H5Fflush returns
  + C binding
    ```
        herr_t H5Pset_nonblocking (hid_t plist, H5VL_log_req_type_t nonblocking);
    ```
* H5Pget_nonblocking
  + Get whether an H5Dwrite or H5Dread call is blocking or non-blocking in a dataset transfer property list
  + Syntax
    + H5Pget_nonblocking (plist, nonblocking)
      + IN plist ID of the dataset transfer property list to retrieve the setting
      + OUT whether the current setting in the dataset transfer property list is blocking or non-blocking
        + H5VL_LOG_REQ_BLOCKING
          + The user buffer can be modified after H5Dwrite returns
          + The data is available in the user buffer after H5Dread returns
        + H5VL_LOG_REQ_NONBLOCKING
          + The user buffer shall not be modified before H5Fflush returns
          + The data is not available in the user buffer before H5Fflush returns
  + C binding
    ```
        herr_t H5Pget_nonblocking (hid_t plist, H5VL_log_req_type_t *nonblocking);
    ```
* H5Pset_idx_buffer_size
  + Set the amount of memory the log-based VOL can use to index metadata for handling read requests
  + If the size metadata does not fit into the limit, H5Dread will be carried out in multiple rounds, and the performance can degrade significantly.
  + The size must be enough to contain the largest metadata section in the file, or H5Dread will return an error.
    + A metadata section is the metadata written by one process in a file (open to close) session.
  + Syntax
    + H5Pset_idx_buffer_size (plist, size)
      + IN plist ID of the dataset transfer property list to attach the setting
      + IN size maximum amount of memory in bytes allowed to use on metadata indexing
        + Use LOG_VOL_BSIZE_UNLIMITED to indicate no limit (default)
  + C binding
    ```
        herr_t H5Pset_idx_buffer_size (hid_t plist, size_t size);
    ```
* H5Pget_idx_buffer_size 
  + Get the amount of memory the log-based VOL can use to cache metadata for handling read requests
  + If the size metadata does not fit into the limit, H5Dread will be carried out in multiple rounds, and the performance can degrade significantly.
  + The size must be enough to contain the largest metadata section in the file, or H5Dread will return an error.
    + A metadata section is the metadata written by one process in a file (open to close) session.
  + Syntax
    + H5Pset_idx_buffer_size (plist, size)
      + IN plist ID of the dataset transfer property list to retrieve the setting
      + OUT size maximum amount of memory in bytes allowed to use on metadata indexing
        + LOG_VOL_BSIZE_UNLIMITED indicates no limit
  + C binding
    ```
        herr_t H5Pget_idx_buffer_size (hid_t plist, ssize_t *size);
    ```
### Misc
* H5VL_log_register
  + Register the log-based VOL and return its ID
  + The returned ID must be closed by calling H5VLclose
  + Syntax
    + H5VL_log_register ()
  + C binding
    ```
        hid_t H5VL_log_register (void);
    ```

## Constants
* H5S_CONTIG
  + The memory space is contiguous and is the same size as the selected part of the dataset dataspace
  + Usage
    ```
	      H5Dwrite (dset_id, mem_type_id, H5S_CONTIG, dspace_id, dxpl_id, buf);
        H5Dread (dset_id, mem_type_id, H5S_CONTIG, dspace_id, dxpl_id, buf);
    ```
  + Equivalent to
     ```
        hssize_t ret = H5Sget_select_npoints( hdspace_id );
        hsize_t dim = (hsize_t)ret;
        mem_space_id = H5Screate_simple( 1, &dim, &dim );
	      H5Dwrite (dset_id, mem_type_id, H5S_CONTIG, dspace_id, dxpl_id, buf);
        H5Dread (dset_id, mem_type_id, H5S_CONTIG, dspace_id, dxpl_id, buf);
    ``` 
## Utilities
* h5lconfig
  + Script for querying configuration and installation options used to build the log-based VOL
  + Usage
    ```
	    % ${logvol_install_path}/bin/h5lconfig --help
      h5lconfig is a utility program to display the build and installation
      information of the log-based VOL library.

      Usage: h5lconfig [OPTION]

      Available values for OPTION include:

        --help                      display this help message and exit
        --all                       display all options
        --cc                        C compiler used to build log-based VOL
        --cflags                    C compiler flags used to build log-based VOL
        --cppflags                  C pre-processor flags used to build log-based VOL
        --c++                       C++ compiler used to build log-based VOL
        --cxxflags                  C++ compiler flags used to build log-based VOL
        --ldflags                   Linker flags used to build log-based VOL
        --libs                      Extra libraries used to build log-based VOL
        --profiling                 Whether internal profiling is enabled or not
        --debug                     Whether log-based VOL is built with debug mode
        --prefix                    Installation directory
        --includedir                Installation directory containing header files
        --libdir                    Installation directory containing library files
        --version                   Library version
        --release-date              Date of log-based VOL source was released
        --config-date               Date of log-based VOL library was configured
    ```
* h5ldump
  + Convert log-based VOL output file into traditional HDF5 files
  + This utility is sequential
  + Examples
    ```
	    % ${logvol_install_path}/bin/h5ldump -h
      Usage: h5ldump <input file path>

      % ${logvol_install_path}/bin/h5ldump test.h5 
      File: test.h5
          Configuration: metadata encoding, metadata compression, 
          Number of user datasets: 1
          Number of data datasets: 1
          Number of metadata datasets: 2
          Number of metadata datasets: 2
          Metadata dataset 0
              Number of metadata sections: 2
              Metadata section 0: 
                  Metadata entry at 0: 
                      Dataset ID: 0; Entry size: 72
                      Data offset: 2228; Data size: 8
                      Flags: multi-selection, encoded, 
                      Encoding slice size: (2, )
                      Selections: 2 blocks
                          0: ( 0, 0, ) : ( 1, 1, )
                          4: ( 1, 0, ) : ( 1, 1, )
              Metadata section 1: 
                  Metadata entry at 0: 
                      Dataset ID: 0; Entry size: 72
                      Data offset: 2236; Data size: 8
                      Flags: multi-selection, encoded, 
                      Encoding slice size: (2, )
                      Selections: 2 blocks
                          0: ( 0, 1, ) : ( 1, 1, )
                          4: ( 1, 1, ) : ( 1, 1, )
          Metadata dataset 1
              Number of metadata sections: 0
    ```
* h5lreplay
  + Convert log-based VOL output file into traditional HDF5 files
  + This utility support parallel run
  + Usage
    ```
	    % mpiexec -np ${N} ${logvol_install_path}/bin/h5lreplay -h
      Usage: h5reply -i <input file path> -o <output file path>

	    % mpiexec -np ${N} ${logvol_install_path}/bin/h5lreplay -i test.h5 -o test_out.h5
      Usage: h5reply -i <input file path> -o <output file path>

      % h5dump test_out.h5
      HDF5 "test_out.h5" {
      GROUP "/" {
        DATASET "D" {
            DATATYPE  H5T_STD_I32LE
            DATASPACE  SIMPLE { ( 2, 2 ) / ( 2, 2 ) }
            DATA {
            (0,0): 1, 2,
            (1,0): 2, 3
            }
        }
      }
      }
    ```
