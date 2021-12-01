## Log-based VOL specific features

This file explains all new APIs and constants introduced by the log-based VOL.

### Dataset APIs (H5D*)
* H5Dread_n
  + Read multiple blocks from a dataset.
    + n - number of blocks.
    + starts - starting coordinate of the blocks to read in the dataset dataspace.
      + starts[i] - starting coordinate of the i-th block to read in the dataset dataspace.
    + counts - the size of the blocks to read.
      + counts[i] - size of the i-th block to read.
  + The data in buf is ordered according to the order of blocks.
    + Data for block i followed immediately by data for block i + 1.
  + Analogous to the ncmpi_get_varn* APIs in PnetCDF.
  + Equivalent to calling H5Dread n times with dataspace selection set to starts[i] and counts[i] on i-th call.
  + Signature
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
    + n - number of blocks.
    + starts - starting coordinate of the blocks to write in the dataset dataspace.
      + starts[i] - starting coordinate of the i-th block to write in the dataset dataspace.
    + counts - the size of the blocks to write.
      + counts[i] - size of the i-th block to write.
  + The data in buf is ordered according to the order of blocks.
    + Data for block i followed immediately by data for block i + 1.
  + Analogous to the ncmpi_put_varn* APIs in PnetCDF.
  + Equivalent to calling H5Dwrite n times with dataspace selection set to starts[i] and counts[i] on i-th call.
  + Signature
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
* H5Pset_nonblocking / H5Pget_nonblocking 
  + Get or set whether an H5Dwrite or H5Dread call is blocking or non-blocking in a dataset transfer property list
    + H5VL_LOG_REQ_BLOCKING (default)
      + The user buffer can be modified after H5Dwrite returns
      + The data is available in the user buffer after H5Dread returns
    + H5VL_LOG_REQ_NONBLOCKING
      + The user buffer shall not be modified before H5Fflush returns
      + The data is not available in the user buffer before H5Fflush returns
  + Signature
    ```
        herr_t H5Pset_nonblocking (hid_t plist, H5VL_log_req_type_t nonblocking);
        herr_t H5Pget_nonblocking (hid_t plist, H5VL_log_req_type_t *nonblocking);
    ```
* H5Pset_idx_buffer_size / H5Pget_idx_buffer_size 
  + Get or set the amount of memory the log-based VOL can use to cache metadata for handling read requests
    + size is in bytes
    + LOG_VOL_BSIZE_UNLIMITED - no limit (default)
  + If the size metadata does not fit into the limit, H5Dread will be carried out in multiple rounds, and the performance can degrade significantly.
  + The size must be enough to contain the largest metadata section in the file, or H5Dread will return an error.
    + A metadata section is the metadata written by one process in a file (open to close) session.
  + Signature
    ```
        herr_t H5Pset_idx_buffer_size (hid_t plist, size_t size);
        herr_t H5Pget_idx_buffer_size (hid_t plist, ssize_t *size);
    ```

### Misc
* H5VL_log_register
  + Register the log-based VOL and return its ID
  + The returned ID must be closed by calling H5VLclose
  + Signature
    ```
        hid_t H5VL_log_register (void);
    ```

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
    