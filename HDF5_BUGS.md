# HDF5 Bugs

---

# Possible HDF5 bugs

---

## HDF5 1.12.0
### The native VOL disregard dataset transfer property passed from the VOL dispatcher 
  * Description
    + The H5VL__native_dataset_write function ignored the dataset transfer property list (dxpl) passed to it.
    + It uses the dxpl saved in an internal variable set in functions between H5Dwrite (user-level API) and VOL dispatcher.
    + When VOLs call H5VLDataset_write (the internal version of H5Dwrite for VOLs that takes OVL object instead of hid) to write datasets handled by another
      VOL, the code setting the internal variable is not executed (default property values).
    + The bug makes it impossible for VOLs to write datasets managed by the native VOL using a non-default dxpl.
    + The log VOL writes metadata entries to a chunked dataset using the native VOL.
    + It can only write the dataset using independent I/O since collective I/O requires setting the dxpl, resulting in deficient performance.
  * Performance
    + E3SM F case ne 120
      + 32 cori haswell ndoes, 32 process/node
      + Metadata flush time: 198 sec
      + Metadata size: 8.23 GiB
    + IOR 32 block, 1 MiB block, 1 MiB transfer size, 32 blocks
      + 8 cori haswell ndoes, 32 process/node
      + Metadata flush time: 30 sec
      + Metadata size: 12 KiB
  * Status
    + Fixed in the develop branch
    + Cannot verify the fix due to another bug in the develop branch that causes a segmentation fault

### HDF5 dispatcher creates a wrapped file object during H5Fcreate, but never close it
  * Description
    + In H5Fcreate, the HDF VOL dispatcher calls H5VLwrap_object on the file just created by H5Vfile_create.
    + According to the VOL manual, the VOl should return a standalone file object (a clone of the file just opened) that needs to be closed separately.
    + HDF5 never closes the wrapped file object with the log VOL.
    + The log VOL will delay file closing until all opened object is closed so that the application can continue to use opened object after calling H5Fclose.
    + The bug keeps the log VOL from closing the file.
  * Status
    + A bug report (https://jira.hdfgroup.org/browse/HDFFV-11140) is created
    + No update so far
  * Workaround: temporarily disable delayed close feature.
    + The log VOL will close the file immediately even if there are opened objects, and those objects will become invalid.
    + The application must ensure all objects are closed before closing the file
  * Performance: N/A (Does not run)

### Log VOL cannot support parallel E3SM I/O benchmark using NetCDF4 API due to a bug in the native VOL
  * Description
    + The program crashes on nc_enddef when we run it on more than one process
    + In one of the H5Aiterate call, the native VOL fails an H5VLattr_specific call due to a packet cannot be decoded correctly
    + The exact reason within the native VOL is still unknown.
    + We found that the bug does not trigger if the log VOL does not create internal attributes on datasets.
    + Those attributes stores metadata related to the dataset (such as its shape) used by the log VOL.
    + We can recreate this issue in the pass_through VOL by creating dataset attributes on all newly created datasets.
    + Since the native VOL is also affected, we believe it is a bug in HDF5
  * status
    + A bug report (https://jira.hdfgroup.org/browse/HDFFV-11154) is created
    + No update so far
  * Workaround: N/A
    + We temporarily use a modified version of the E3SM benchmark that calls HDF5 APIs directly without NetCDF4.
    + All of our performance reports are based on this version

## HDF5 develop
### H5VL_native_file_close crashes when the upper VOL writes a chunked dataset on H5VLfile_close with a dataset transfer property
  * Description
    + We link the log VOL against the develop branch, which contains the fix to the bug of H5VLdataset_write ignoring dataset transfer property list (dxpl)
    + On H5VLfile_close, the log VOL creates a chunked dataset and write metadata to it by calling H5VLdataset_write
    + The log VOL created a new dxpl and set the MPI-IO property to use collective I/O.
    + The H5VLdataset_write call succeeds, but a segmentation fault occurs in H5VL_native_file_close afterward. 
    + We do not know the exact cause.
    + We found that the bug does not trigger if we use the dxpl_id passed to the H5VLfile_close call of our VOL.
    + Any other dxpl, even a clone of the given one (H5Pcopy), will result in a crash.
    + We can recreate this issue in the pass_through VOL by writing to a chunked dataset on H5VLfile_close using a newly created dxpl.
    + Since the native VOL is also affected, we believe it is a bug in HDF5
  * status
    + A bug report (https://jira.hdfgroup.org/browse/HDFFV-11161) is created
    + No update so far
  * Workaround: Use MPI-IO to write metadata
    + We redesign the file format of the log VOL to use a contiguous dataset for metadata.
    + The log VOL writes to the metadata dataset directly using MPI-IO


