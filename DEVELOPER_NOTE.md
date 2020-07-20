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
## HDF5 Predefined Data Type Initialization
### Problem Description
  * User VOLs are responsible to define and manage data types. Data types are not inhereted from the native VOL. 
  * [Predefined data types](https://support.hdfgroup.org/HDF5/doc/RM/PredefDTypes.html) (e.g. H5T_STD_*, H5T_NATIVE_*) are datatype managed by the native VOL.
  * In the native VOL, the predefined data types are not defined as constants.
  * Instead, they are defined as global variables, which are later initialied by the native VOL.
  * Before their initializetion, they are assigned value of -1 (see H5T.c:341)
  * Initializing the native VOL will not automatically initialize the predefined data types.
  * They should be initialized by a callback function when the user VOL is initialized (part of file_specific).
  * The HDF5 dispatcher first calls the introspect API of the user VOL to check for existence of the initialization routine.
  * If succeeded, the dispatcher will call the user VOL's initialization function when needed (**WKL: when needed?**).
  * This is not documented in HDF5 user guides. This behavior was found when reading into the source codes of the native VOL.
  * Therefore, a user VOL must call the native VOL's initialization routine first. Otherwise, the values of those global variables will not be valid (remained to be -1).
  * (**WKL: I assume the 2 sentences below desribed our earlier implementation and it has been fixed now. If this is the case, please remove them.**)  
  However, our VOL does not support the introspect APIs as well as the special initialization routine.
  In consequence, the initialization routine of the native VOL is not called, and passing any predefined datatype to the native VOL will cause an error (unrecognized data type).

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
## H5Sget_select_hyper_nblocks does not combine selection blocks
**Problem description**:
  In older versions, H5Sget_select_hyper_nblocks will combine selections if they together form a larger rectangular block.
  The feature is removed in 1.12.0.
  Should the application declare a rectangular selection as many 1x1 blocks instead of a single block, the overhead of metadata can be significant.
  HDF5 uses the argument "count" to refer to the number of blocks, and "block" to refer to the size of the blocks.
  NetCDF selection has only one block, and "count" refers to the block size.
  Developers transitioning from NetCDF to HDF5 may mistake the "count" argument as the block size and run into performance issues.
  * HDF5 versions: 1.12.0
  * Environment settings: N/A
  * Trigger condition: Explicitly performed by the user
**potential solutions**:
  * Ignore (current)
    + We left it to the user to use the library in an efficient way.
    + Adding the overhead of a merging routine to fix a possible developer mistake may not be worth it.
  * Implement merging in the VOL

## The log VOL does not report reference count to the underlying VOL it is using 
**Problem description**:
  HDF5 keeps a reference count to the ID (hid_t) of some type of internal objects.
  VOL is one of the objects tracked by reference count. The reference count applies to the ID, not the object. 
  It can be understood as the number of copies of the ID that currently resides in the memory.
  By HDF5's current design, whichever routine that keeps the ID for use must increase the reference count.
  When the ID is no longer needed, the routine that increased the reference must call an API to decrease the reference count.
  If the count reaches 0, the underlying object will be freed.

  Our VOL uses the native VOl as the back end. It uses the ID of the native VOL to refer to it.
  Previously, our VOL does not report reference count to HDF5.
  Should any other internal function finish using the native VOL and try to decrease the reference count, the dispatcher will think that no one is using the native VOL and free it.
  In that case, our VOL will get an error on every operation it performs with the native VOL as the native VOL is already deallocated.
  * HDF5 versions: 1.12.0
  * Environment settings: N/A
  * Trigger condition: All API that triggers a reference count check (decrease) on the native VOL ID.
**implemented solution**:
  * Ignore (current)
    + We call H5Iinc_ref to report a new reference every time we copied the ID the native VOL.
    + We call H5Idec_ref every time the variable holding the native VOL ID is freed.
    + Code: logvol_file.cpp:78, 174, 377
* I assume each HDF5 API calls the dispatcher.
  + Some function, such as H5P*, is not.
* When the dispatcher detects a zero count to an object in an HDF% API, does it immediately free the internal data structures allocated for that object?
  + Yes, but only for tracked object types.

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

