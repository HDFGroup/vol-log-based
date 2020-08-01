# Developer's Note

- [Issues encountered](#issues-encountered)

---

# Issues Encountered

```
General comments: I suggest the following when adding a new issue.

## Short title of the issue
**Problem description**:
  * HDF5 versions
  * Environment settings
  * Trigger condition, including an example code fragment
**Proposed solutions**:
  * Solution 1
    + Description
    + Code fragments or reference links to source files
    + Test programs developed to test the fix, which contains
      comments referring to this fix (commit SHA number)
  * Solution 2
  ...
```
---
## HDF5 Predefined Data Type Initialization (Solved)
### Problem Description
  * User VOLs are responsible to define and manage data types. Data types are not inherited from the native VOL.
  * [Predefined data types](https://support.hdfgroup.org/HDF5/doc/RM/PredefDTypes.html) (e.g. H5T_STD_*, H5T_NATIVE_*) are datatype managed by the native VOL.
  * In the native VOL, the predefined data types are not defined as constants.
  * Instead, they are defined as global variables, which are later initialized by the native VOL.
  * Before their initialization, they are assigned value of -1 (see H5T.c:341)
  * Initializing the native VOL will not automatically initialize the predefined data types.
  * They should be initialized by a callback function when the user VOL is initialized (part of file_specific).
  * The HDF5 dispatcher first calls the introspect API of the user VOL to check for existence of the initialization routine.
  * If succeeded, the dispatcher will call the user VOL's initialization function when needed (**WKL: when needed?**).
  * This is not documented in HDF5 user guides. This behavior was found when reading into the source codes of the native VOL.
  * Therefore, a user VOL must call the native VOL's initialization routine first. Otherwise, the values of those global variables will not be valid (remained to be -1).

### Software Environment
  * HDF5 versions: 1.12.0
  * Trigger condition: Using any predefined datatype before calling the initializing routine of the native VOL

### Solutions
  + Implement the initialization routine in our VOL to call the native VOL initialization routine (pass-through).
  + Add the initialization routine so the HDF5 dispatcher can call it.
  + Source files: [logvol_introspect.cpp](/src/logvol_introspect.cpp), and [logvol_file.cpp](https://github.com/DataLib-ECP/log_io_vol/blob/e250487955bb0d498734f2455ad4d095189fe9f4/src/logvol_file.cpp#L324)
  + Commit: [dff337d](https://github.com/DataLib-ECP/log_io_vol/commit/dff337dbc63ef8363984f5e8a9d0dd9eb01f2fe9)
  + Test program [test/basic/dwrite.cpp](/test/basic/dwrite.cpp)

---
## H5Sget_select_hyper_nblocks does not combine selection blocks (Not a bug)
### Problem Description
  HDF5 supports two types of dataspace selection - point list and hyper-slab.
  A point list selection is represented by a list of selected points (unit cell) directly.
  A hyper-slab selection is represented by four attributes - start, count, stride, and block.
  "Start" means the starting position of the selected blocks.
  "Count" is the number of blocks selected along each dimension.
  "Stride" controls the space between selected blocks
  "Block" is the size of each selected block.

  The meaning of "count" in HDF5's hyper-slab selection is different from that in netcdf APIs.
  NetCDF selection use "count" to refer to the block size while HDF5 uses "block" in hyper-slab selection.
  Developers transitioning from NetCDF to HDF5 may mistake the "count" argument as the block size when they want to select a single block.
  In that case, the application will end up selecting the entire region with 1x1 cells.
  It can greatly affect the performance of our log VOL as we create a metadata entry int he index for every selected block.
  Before 1.12.0, H5Sget_select_hyper_blocklist will automatically combine those 1x1 selections into a single block before returning to the user.
  It help mitigates the issue if the developer made mistakes mentioned above.
  However, they removed this feature 1.12.0 and H5Sget_select_hyper_blocklist returns the selected blocks as is.

### Software Environment
  * HDF5 versions: 1.12.0
  * Environment settings: N/A
  * Trigger condition: Explicitly performed by the user

### Solutions
  * Ignore (current)
    + We left it to the user to use the library in an efficient way.
    + Adding the overhead of a merging routine to fix a possible developer mistake may not be worth it.
  * Implement merging in the VOL

## The log VOL does not report reference count to the underlying VOL it is using (Solved)
### Problem description
  Certain types of HDF5 objects can be closed by HDF5 automatically when no longer in use.
  For those objects, HDF5 keeps a reference count on each opened instance.
  The reference count represents the number of copies of the ID (the token used to refer to the instant) that currently resides in the memory.
  Should the reference count reaches 0, HDF5 will close the instance automatically.
  For simplicity, we refer to those objects as "tracked objects."

  To maintain the reference count, HDF5 requires the user to report new references to tracked objects (increase reference count) every time they copy the ID (such as assigning it to a variable) of the object.
  When a copy of the ID is removed from the memory, the user has to decrease the reference count by calling a corresponding API.
  If the reference count is not reported correctly, HDF5 may free the object prematurely while the user still needs it.

  VOLs are tracked objects.
  If our log VOL uses the native VOL internally but does not report reference of the native VOL, HDF5 will close the native VOL when another internal routine that uses the native VOL finishes and decreases the reference count.
  In consequence, our log VOL receives an error on all native VOL operations afterward.

### Software Environment
  * HDF5 versions: 1.12.0
  * Environment settings: N/A
  * Trigger condition: All API that triggers a reference count check (decrease) on the opened native VOL.

### Implemented Solution
  * Report reference our reference of the native VOL as the passthrough VOL does
    + We call H5Iinc_ref to report a new reference every time we copied the ID the native VOL.
    + We call H5Idec_ref every time the variable holding the native VOL ID is freed.
    + Source files:
      + [logvol_file.cpp](/src/logvol_file.cpp)(https://github.com/DataLib-ECP/log_io_vol/commit/1a6753ac69edd830135dd8715b649a3abb087706#L198)
      + [logvol_att.cpp](/src/logvol_att.cpp)(https://github.com/DataLib-ECP/log_io_vol/commit/1a6753ac69edd830135dd8715b649a3abb087706#diff-5df8d5bffeba925af26354bf8ece0287#L47)
      + [logvol_dataset.cpp](/src/logvol_dataset.cpp)(https://github.com/DataLib-ECP/log_io_vol/commit/1a6753ac69edd830135dd8715b649a3abb087706#diff-64f1886c9cbf0d0d9dbe22562cc56283#L56)
      + [logvol_group.cpp](/src/logvol_group.cpp)(https://github.com/DataLib-ECP/log_io_vol/commit/1a6753ac69edd830135dd8715b649a3abb087706#diff-dda415767fad1bd1cea5df6b7503c76b#L49)
      + [logvol_util.cpp](/src/logvol_util.cpp)(https://github.com/DataLib-ECP/log_io_vol/commit/1a6753ac69edd830135dd8715b649a3abb087706#diff-2e6bb9634647126e360e707b54ca2d2b#L24)
    + Commit: [1a6753ac69edd830135dd8715b649a3abb087706](https://github.com/DataLib-ECP/log_io_vol/commit/1a6753ac69edd830135dd8715b649a3abb087706)

############################################################################################################################################
* I assume each HDF5 API calls the dispatcher.
  => Some function, such as H5P*, is not.
* When the dispatcher detects a zero count to an object in an HDF5 API, does it immediately free the internal data structures allocated for that object?
  => Yes, but only for tracked object types.
############################################################################################################################################

## NetCDF4 support
**Problem description**:
  According to the design proposal, our VOL will support basic I/O operations on File, Group, Dataset, and Attribute.
  We did not plan to support advanced HDF5 features such as Link (H5L*), Object (H5O*), Reference (H5R*), etc.
  Our VOL also does not support advanced operation on HDF5 objects such as iterating (*iterate).

  The NetCDF library requires many features beyond the design of our VOL for NetCDF4 files.
  Currently known requirements are Object (H5O*) and Link (H5L*).
  To support the NetCDF integration, we need to significantly improve the compatibility of the VOL beyond our original plant of attribute, group, and attributes.
  Many of them can be tackled by passing the request to the native VOL (pass-through), but some require special handling.
  H5Ocopy is one example. If the object copied is a dataset, we need to update its attribute as well as the metadata in the file since we assign every dataset a unique ID.
  * HDF5 versions: 1.12.0
  * Environment settings: N/A
  * Trigger condition: Using the log VOL in (modified) NetCDF4 library.
**Current solution (onging)**:
  * Extend support to other HDF5 objects and advanced functions
    + We need to implement VOL API for nearly all HDF5 features to support NetCDF4
    + Code: logvol_obj.cpp, logvol_link.cpp


## Native VOL won't close if there are data objects still open
**Problem description**:
  The native VOL keeps track of all data object it opens.
  If there are objects associated with a file still open, the native VOL will refuse to close the file and return an error.
  Our VOL wraps around the native VOL. Every object opened by our VOL contains at least one opened object by the native VOL.
  If the application does not close all opened objects before closing the file, the native VOL will report an error.
  Under our current implementation, the log VOL will abort whenever the native VOL returns an error.
  * HDF5 versions: 1.12.0
  * Environment settings: N/A
  * Trigger condition: Call H5Fclose when there are other file objects left open.
**Potential solutions**:
  * Catch the error and return to the dispatcher without aborting
    + We need to implement VOL API for nearly all HDF5 features to support NetCDF4
    + Code: logvol_obj.cpp, logvol_link.cpp
  * Automatically close all file object when the file is closed
    + Keep track on all opened file objects within the file structure
    + Close and free any object associated with the file when the file is closing.
  * Delay file close until the last opened object is closed
    + Keep an open object count in the file structure.
    + If the count is not 0 when the user closes the file, set a flag to indicate the file should be closed without actually closing the file.
    + When the count reaches 0, and the closing flag is set, the VOL automatically closes the file
    + It may not always be possible as is discussed in https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Close

## Native VOL won't close if there are data objects still open
  HDF5 dispatcher placed a hook on MPI_Finalize to finalize the HDF5 library.
  When finalizing, the dispatcher will close all files not closed by the application by calling the file close VOL API.
  During that time, the library is marked to be in shutdown status, and the function of the native VOL will become limited.
  Calling any native VOL function not related to shutting down (such as the internal function H5L_exists to check whether a link exists in the file) will get an error.

  Our VOL flushes the file when the file is closing.
  It relies on the native VOL to update metadata and to create log datasets.
  However, many functions it uses are not available while the library is in shutdown status (H5L_exists to check whether we already have the metadata log and the lookup table).
  Thus, our file closing API will always fail when called by the dispatcher on MPI_Finalize.
  * HDF5 versions: 1.12.0
  * Environment settings: N/A
  * Trigger condition: Call MPI_Finalize without closing opened files.
**possible solutions**:
  * Contact the HDF5 team to see if there is a way to reactivate the native VOL during the shutdown status

## Registering property changes the signature of the property class
  H5Pregister can be used to register new properties to an existing property class (file access, dataset transfer, etc.).
  After registration, any new property list created under this property class will include the registered property.
  It allows applications or third-party plugins to define its own properties.
  When H5Pregister is called on a property class, HDF5 replaces it with a clone that includes the new property.
  However, HDF5 will not update the existing property lists of that property class to include the new property.
  Thus, those property lists will no longer be compatible with the property class as they contain a different set of properties.
  Passing them to any HDF5 API will result in an error because the sanity check will consider them to be of the wrong property class.

  For example, an application creates a file access property (H5P_FILE_ACCESS) list L.
  Then, it registers a new property P under the file access property class.
  After that, it calls H5Fcreate with L as the file access property list (fapl).
  H5Fcreate will fail because L is no longer a file access property as it does not contain property P.
  H5P_DEFAULT will also become outdated as it maps to an internally cached property list that is initialized when the library initializes.

  In HDF5, properties are not managed by the VOL.
  If a VOL wants to create it's own property, it must use the same H5Pregister API as applications.
  Our log VOL defines two customized properties.
  One is a file access property that indicates the amount of memory available for buffering data.
  Another one is a dataset transfer property to indicate whether an  I/O operation on a dataset is blocking or nonblocking.
  When the log VOL registers those properties during its initialization, it invalidates all existing file access and dataset transfer property lists.
  Also, H5P_DEFAULT can no longer be used to represent property lists of the two property classes.
  The issue is observed when trying to load the log VOL dynamically using environment variables.
  After loading and initializing the log VOL, the dispatcher tries to set it as the default VOL in the default file access property list (the property list used when H5P_DEFAULT is seen) by calling H5Pset_vol.
  H5Pset_vol returns an error because the default file access property list is no longer a file access property.
  * HDF5 versions: 1.12.0
  * Environment settings: N/A
  * Trigger condition: Call H5Pregister2 to register a new file access property, then create a file using H5P_DEFAULT as file access property.
**possible solutions**:
  * Contact the HDF5 team to see if there is a way to update all existing property list after the property class is extended.
  * Avoid registering new properties, try to create our own class.
    + Need to study the way to create our own property class.

## The VOL fail when running E3SM on multi-process
  The issue is still being studied.
  A H5Aiterate call failed on the second process due to some format decoding error.

---
## Improve Performance of Posting Dataset Write Requests
### Problem Description
* If treating individual calls to H5Dwrite independently, then the log-based
  driver must call HDF5 APIs to query properties of a defined dataset in order
  to perform sanity check on the user arguments. Such queries can be expensive
  when the number of H5Dwrite calls is high. This is particularly true for
  applications whose I/O patterns show a large number of noncontiguous subarray
  requests, such as E3SM.
* HDF5 currently does not have an API to read/write a large number of
  noncontiguous subarray requests to the same variable. HDF5 group is currently
  developing "H5Dwrite_multi", a new API that allows to write multiple datasets
  in a single API call. See HDF5 develop branch ["multi_rd_wd_coll_io"](https://bitbucket.hdfgroup.org/projects/HDFFV/repos/hdf5/browse?at=refs%2Fheads%2Finactive%2Fmulti_rd_wd_coll_io).
* Before API "H5Dwrite_multi" is officially released, one must either treat a
  subarray request independently from others, i.e. each by a separate call to
  H5Dwrite, or flatten all subarray requests into points and call
  ["H5Sselect_elements"](https://support.hdfgroup.org/HDF5/doc1.8/RM/RM_H5S.html#Dataspace-SelectElements).
  Both approaches can be expensive.

### Software Environment
* HDF5 versions: 1.12.0

### Solutions
* The log-based driver can cache the dataset's metadata queried at the user's
  first call to H5Dwrite and reuse it if the next call to H5Dwrite is for the
  same dataset, i.e. same dataset ID argument is used. The cached metadata can
  be used until the dataset of a call to H5Dwrite is different. This pattern of
  a long sequence of H5Dwrite to the same dataset is observed in E3SM. Such a
  caching strategy avoids repeated HDF5 calls to query the dataset metadata.
* The above caching strategy can also help aggregate the request metadata to
  the same dataset into the same "entry" as a part of the
  [metadata index table](https://github.com/DataLib-ECP/log_io_vol/blob/master/doc/design_log_base_vol.md#appendix-a-format-of-log-metadata-table).
  This is particularly useful for E3SM I/O pattern that makes a large number of
  subarray requests to a variable after another.

---


