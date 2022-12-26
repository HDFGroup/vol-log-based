# The Log VOL connector APIs

This file explains new APIs introduced by the Log VOL connector.
While some of these APIs are optional to use, they provide applications the opportunity to fine-tune the Log VOL connector for better performance.

## Dataset APIs (H5D*)
### H5Dread_n
The function `H5Dread_n` reads "n" blocks of a variable in an opened dataset by providing a list of selection blocks instead of using dataspace. It is equivalent to calling `H5Dread` "n" times with dataspace selection set to `starts[i]` and `counts[i]` on the i-th call. It is analogous to the `ncmpi_get_varn*` APIs in PnetCDF.  

#### Usage:
```c
herr_t H5Dread_n (hid_t     did,
                  hid_t     mem_type_id,
                  int       n,
                  hsize_t **starts,
                  hsize_t **counts,
                  hid_t     dxplid,
                  void     *buf);
```

+ Inputs:  
  + `did`: the id of the dataset to read, from previous call to `H5Dopen` or `H5Dcreate`. 
  + `mem_type_id`: the HDF5 datatype of the data elements.
  + `n`: the number of blocks to read.
  + `starts`: an array of the starting coordinates of the blocks to read.
    + `starts[i]`: the starting coordinates of the i-th block to read in the dataset dataspace.
    + `starts[i][j]`: the starting coordinates of the i-th block, along the j-th dimension
  + `counts`: an array of the sizes of the blocks to read.
    + `counts[i]`: the size of the i-th block to read.
    + `counts[i][j]`: the size of the i-th block, along the j-th dimension
  + `dxplid`: dataset transfer property list ID.
+ Outs:
  + `buf`: buffer to save read-in data.
    + The data in `buf` is ordered according to the order of blocks.
    + Data for block i is followed immediately by data for block i + 1.
+ Returns:
  + This function returns `0` on success. Fail otherwise.
  
    
### H5Dwrite_n
  The function `H5Dwrite_n` writes "n" blocks of a variable in an opened dataset by providing a list of selection blocks instead of using dataspace. It is equivalent to calling `H5Dwrite` "n" times with dataspace selection set to `starts[i]` and `counts[i]` on the i-th call. It is analogous to the `ncmpi_put_varn*` APIs in PnetCDF.

#### Usage:
```c
herr_t H5Dwrite_n ( hid_t     did,
                    hid_t     mem_type_id,
                    int       n,
                    hsize_t **starts,
                    hsize_t **counts,
                    hid_t     dxplid,
                    void     *buf);
```

  + Inputs:
    + `did`: the id of the dataset to write, from previous call to `H5Dopen` or `H5Dcreate`. 
    + `mem_type_id`: the HDF5 datatype of the data elements.
    + `n`: the number of blocks to write.
    + `starts`: an array of the starting coordinates of the blocks to write.
      + `starts[i]`: the starting coordinates of the i-th block to write in the dataset dataspace.
      + `starts[i][j]`: the starting coordinates of the i-th block, along the j-th dimension
    + `counts`: an array of the sizes of the blocks to write.
      + `counts[i]`: the size of the i-th block to write.
      + `counts[i][j]`: the size of the i-th block, along the j-th dimension
    + `dxplid`: dataset transfer property list ID.
    + `buf`: data to write.
      + The data in `buf` is ordered according to the order of blocks.
      + Data for block i is followed immediately by data for block i + 1.
  + Returns:
    + This function returns `0` on success. Fail otherwise.
  

## Properties APIs (H5P*)
### H5PSet_buffered
The function `H5PSet_buffered` sets a boolean value that indicates whether data should be buffered internally in the Log VOL connector when an `H5Dwrite` call is made. 

If it is set to `false`, the Log VOL connector does not keep a copy of the input write buffer used by `H5Dwrite`. Since `H5Flush` is when writes actually happen, the input buffer cannot be modified until an `H5Fflush` call is made. If it is set to `true`, a copy of the input buffer is saved internally and the user can modify the input buffer immediately after `H5Dwrite` returns.

If not set, the default property is buffered.

#### Usage:
```c
  herr_t H5PSet_buffered (hid_t plist, hbool_t buffered);
```
  + Inputs:
    + `plist`: the id of the dataset ransfer property list to attach the setting.
    + `buffered`: a boolean value that indicates whether to keep a copy of the input write buffer of `H5Dwrite` calls in the Log VOL connector.
        + `true`: (default)
          + keep a copy. i.e. needs addtional memory  
          + the input buffer can be modified immediately after H5Dwrite returns
        + `false`:
          + the input buffer shall not be modified immediately after `H5Dwrite`s.
          + The user buffer shall not be modified until an `H5Fflush` is made.
  + Returns:
    + This function returns `0` on success. Fail otherwise.
### H5Pget_buffered
The function `H5Pget_buffered` inquries whether data is buffered internally in the Log VOL connector when an `H5Dwrite` call is made, under the current setting. 

If it is set to `false`, the Log VOL connector does not keep a copy of the input write buffer used by `H5Dwrite`. Since `H5Flush` is when writes actually happen, the input buffer cannot be modified until an `H5Fflush` call is made. If it is set to `true`, a copy of the input buffer is saved internally and the user can modify the input buffer immediately after `H5Dwrite` returns.

#### Usage:
```c
  herr_t H5Pget_buffered (hid_t plist, hbool_t *buffered);
```
  + Inputs:
    + `plist`: the id of the dataset ransfer property list to retrieve the setting.
  + Outs:
    + `buffered`: whether to keep a copy of the input write buffer of `H5Dwrite` calls under current setting indicated by `plist`.
        + `true`:
          + keep a copy. i.e. needs addtional memory  
          + the input buffer can be modified immediately after H5Dwrite returns
        + `false`:
          + the input buffer shall not be modified immediately after `H5Dwrite`s.
          + The user buffer shall not be modified until an `H5Fflush` is made.
  + Returns:
    + This function returns `0` on success. Fail otherwise.


### H5Pset_idx_buffer_size
The function `H5Pset_idx_buffer_size` sets the amount of memory that the Log VOL connector can use to index metadata for handling read requests. If the size of the metadata does not fit into the limit, `H5Dread` will be carried out in multiple rounds, and the performance can degrade significantly. The size must be enough to contain the largest metadata section in the file, or `H5Dread` will return an error. (A metadata section is the metadata written by one process in a file (open to close) session.)

#### Usage:
```c
  herr_t H5Pset_idx_buffer_size (hid_t plist, size_t size);
```
  + Inputs:
    + `plist`: the id of the dataset ransfer property list to attach the setting.
    + `size`: the maximum amount of memory in bytes allowed to use on metadata indexing.
      + Use `LOG_VOL_BSIZE_UNLIMITED` to indicate no limit (default) 
  + Returns:
    + This function returns `0` on success. Fail otherwise.

### H5Pget_idx_buffer_size 
The function `H5Pget_idx_buffer_size` gets the amount of memory that the Log VOL connector can use to cache metadata for handling read requests. If the size of the metadata does not fit into the limit, `H5Dread` will be carried out in multiple rounds, and the performance can degrade significantly. The size must be enough to contain the largest metadata section in the file, or H5Dread will return an error. (A metadata section is the metadata written by one process in a file (open to close) session.)

#### Usage:
```c
  herr_t H5Pget_idx_buffer_size (hid_t plist, size_t *size);
```
  + Inputs:
    + `plist`: the id of the dataset ransfer property list to retrieve the setting.
  + Outs:
    + `size`: the maximum amount of memory in bytes allowed to use on metadata indexing.
      + `LOG_VOL_BSIZE_UNLIMITED` to indicate no limit. 
  + Returns:
    + This function returns `0` on success. Fail otherwise.

### H5Pset_meta_share
The function `H5Pset_meta_share` sets the whether to perform deduplication on metadata entries. If enabled, requests of the same I/O pattern will share the same metadata on dataspace selection to reduce metadata size.

#### Usage:
```c
  herr_t H5Pset_meta_share (hid_t plist, hbool_t share);
```
  + Inputs:
    + `plist`: the id of the file access property list to attach the setting.
    + `share`: whether to deduplicate the metadata.
        + `true`:
          + Deduplicate the metadata
        + `false`:
          + Do not deduplicate the metadata
          + Each metadata entry will contain its own dataspace selection information
  + Returns:
    + This function returns `0` on success. Fail otherwise.

### H5Pget_meta_share 
The function `H5Pget_meta_share` gets the metadata deduplication setting in a file access property list.

#### Usage:
```c
  herr_t H5Pset_meta_share (hid_t plist, hbool_t &share);
```
  + Inputs:
    + `plist`: the id of the dataset ransfer property list to retrieve the setting.
  + Outputs:
    + `share`: whether to deduplicate the metadata.
        + `true`:
          + Deduplicate on
        + `false`:
          + Deduplicate off
  + Returns:
    + This function returns `0` on success. Fail otherwise.

### H5Pset_meta_zip
The function `H5Pset_meta_zip` sets the whether to compress the metadata.

#### Usage:
```c
  herr_t H5Pset_meta_zip (hid_t plist, hbool_t zip);
```
  + Inputs:
    + `plist`: the id of the file access property list to attach the setting.
    + `share`: whether to compress the metadata.
        + `true`:
          + Compress the metadata
        + `false`:
          + Do not compress the metadata
          + Each metadata entry will contain its own dataspace selection information
  + Returns:
    + This function returns `0` on success. Fail otherwise.

### H5Pget_meta_zip 
The function `H5Pget_meta_zip` gets the metadata comrpession setting in a file access property list.

#### Usage:
```c
  herr_t H5Pget_meta_zip (hid_t plist, hbool_t &zip);
```
  + Inputs:
    + `plist`: the id of the dataset ransfer property list to retrieve the setting.
  + Outputs:
    + `share`: whether to compress the metadata.
        + `true`:
          + Compression on
        + `false`:
          + Compression off
  + Returns:
    + This function returns `0` on success. Fail otherwise.

### H5Pset_subfiling
The function `H5Pset_subfiling` enables subfiling.

#### Usage:
```c
  herr_t H5Pset_subfiling (hid_t plist, int nsubfiles);
```
  + Inputs:
    + `plist`: the id of the file create property list to attach the setting.
    + `nsubfiles`: number of subfiles.
        + `-1`:
          + One subfile per compute node.
        + `0`:
          + Disable subfiling.
        + `N`:
          + Creatae N subfiles.
          + N must be a positive integer.
  + Returns:
    + This function returns `0` on success. Fail otherwise.

### H5Pget_subfiling 
The function `H5Pget_subfiling` gets the subfiling setting in a file create property list.

#### Usage:
```c
  herr_t H5Pget_subfiling (hid_t plist, int *nsubfiles);
```
  + Inputs:
    + `plist`: the id of the file create property list to retrieve the setting.
  + Outputs:
    + `nsubfiles`: number of subfiles.
        + `-1`:
          + One subfile per compute node.
        + `0`:
          + Subfiling disabled.
        + `N`:
          + Number of subfiles used.
          + N is a positive integer.
  + Returns:
    + This function returns `0` on success. Fail otherwise.

### H5Pset_passthru
The function `H5Pset_passthru` makes the Log VOL connector perform as a
terminal VOL connector if `enable` is set to `false` and perform as a
passthrough VOL connector otherwise. As a terminal VOL connector, the Log VOL
connector performs all writes using MPI-IO. As a passthrough VOL connector, the
Log VOL connector uses the underlying VOL to perform all writes.  If users do
not specify the underlying VOL, then the native VOL is used by default.

#### Usage:
```c
herr_t H5Pset_passthru (hid_t faplid, hbool_t enable)
```
  + Inputs:
    + `faplid`: the id of the file access property list to set the setting.
    + `enable`: whether passthrough VOL should be used.
      + `ture`: the Log VOL connector behaves as a passthrough VOL.
      + `false`: the Log VOL connector behaves as a terminal VOL.
  + Returns:
    + This function returns `0` on success. Fail otherwise.
  
### H5Pget_passthru
The function `H5Pget_passthru` gets the setting of the Log VOL connector on
whether it perform as a terminal VOL connector or a passthrough VOL connector.
As a terminal VOL connector, the Log VOL connector performs all writes using
MPI-IO. As a passthrough VOL connector, the Log VOL connector uses the
underlying VOL to perform all writes.

#### Usage:
```c
herr_t H5Pget_passthru (hid_t faplid, hbool_t *enable);
```
  + Inputs:
    + `faplid`: the id of the file access property list to retrieve the setting.
  + Outputs:
    + `enable`: whether passthrough VOL is used.
      + `ture`: the Log VOL connector behaves as a passthrough VOL.
      + `false`: the Log VOL connector behaves as a terminal VOL.
  + Returns:
    + This function returns `0` on success. Fail otherwise.

## Misc
### H5VL_log_register
The function `H5VL_log_register` register the Log VOL connector connector and return its ID. The returned ID can be used to set the file access properties so that `HDF5` knows whether or not to use the Log VOL connector. The returned ID must be closed by calling `H5VLclose` before file close.
#### Usage:
```c
  hid_t H5VL_log_register (void);
```
  + Returns:
    + This function returns the non-negative ID for the Log VOL connector connector on success, and returns `H5I_INVALID_HID` (`-1`) on failure.
    
    
  + C example:
    ```c
      // ... some other codes
      hid_t fapl_id = H5Pcreate (H5P_FILE_ACCESS);
      H5Pset_fapl_mpio (fapl_id, MPI_COMM_WORLD, MPI_INFO_NULL);
      hid_t logvol_id = H5VL_log_register ();
      H5Pset_vol (fapl_id, logvol_id, NULL);
      // ... some other codes
      hid_t err = H5VLclose (log_vol_id);
      ```

