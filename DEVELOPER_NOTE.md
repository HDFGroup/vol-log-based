# Developer's Note

+ [HDF5 Predefined Data Type Initialization](#hdf5-predefined-data-type-initialization-solved)
+ [H5Sget_select_hyper_nblocks does not combine selection blocks](#h5sget_select_hyper_nblocks-does-not-combine-selection-blocks-not-a-bug)
+ [Log VOL does not report reference count to the underlying VOL it is using](#log-vol-does-not-report-reference-count-to-the-underlying-vol-it-is-using-solved)
+ [NetCDF4 support](#netcdf4-support)
+ [VOL fails when running E3SM on multi-process through NetCDF4 API](#vol-fails-when-running-e3sm-on-multi-process-through-netcdf4-api)
+ [Native VOL won't close if there are data objects still open](#native-vol-wont-close-if-there-are-data-objects-still-open)
+ [Native VOL is not available when the library is shutting down](#native-vol-is-not-available-when-the-library-is-shutting-down)
+ [Registering property changes the signature of the property class](#registering-property-changes-the-signature-of-the-property-class)
+ [Improve Performance of Posting Dataset Write Requests](#improve-performance-of-posting-dataset-write-requests)
+ [Memory space is always required in H5Dwrite even if the memory buffer is contiguous](#memory-space-is-always-required-in-h5dwrite-even-if-the-memory-buffer-is-contiguous)
+ [H5VLDataset_write do not follow collective property](#h5vldataset_write-do-not-follow-collective-property)
+ [HDF5 dispatcher creates a wrapped VOL object, but never free it](#hdf5-dispatcher-creates-a-wrapped-vol-object-but-never-free-it)
+ [Metadata size can be larger than data size](#metadata-size-can-be-larger-than-data-size)


---

# Issues Encountered

```
General comments: I suggest the following when adding a new issue.

## Short title of the issue
### Problem description
  * HDF5 versions
  * Environment settings
  * Trigger condition, including an example code fragment
### Proposed solutions
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
  * User VOLs are responsible for defining and managing data types. Data types are not inherited from the native VOL.
  * [Predefined data types](https://support.hdfgroup.org/HDF5/doc/RM/PredefDTypes.html) (e.g. H5T_STD_*, H5T_NATIVE_*) are datatype managed by the native VOL.
  * In the native VOL, the predefined data types are not defined as constants.
  * Instead, they are defined as global variables, which are later initialized by the native VOL.
  * Before their initialization, they are assigned a value of -1 (see H5T.c:341)
  * Initializing the native VOL will not automatically initialize the predefined data types.
  * They should be initialized by a callback function when the user VOL is initialized (part of file_specific).
  * The HDF5 dispatcher first calls the introspect API of the user VOL to check for the existence of the initialization routine.
  * If succeeded, the dispatcher will call the user VOL's initialization function when needed (**when?**).
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
  + A point list selection is represented by a list of selected points (unit cell) directly.
  + A hyper-slab selection is represented by four attributes - start, count, stride, and block.
  + "Start" means the starting position of the selected blocks.
  + "Count" is the number of blocks selected along each dimension.
  + "Stride" controls the space between selected blocks
  + "Block" is the size of each selected block.

  The meaning of "count" in HDF5's hyper-slab selection is different from that
  in netcdf APIs.  NetCDF selection use "count" to refer to the block size
  while HDF5 uses "block" in hyper-slab selection.  Developers transitioning
  from NetCDF to HDF5 may mistake the "count" argument as the block size when
  they want to select a single block.  In that case, the application will end
  up selecting the entire region with 1x1 cells.  It can greatly affect the
  performance of our log VOL as we create a metadata entry int he index for
  every selected block.  Before 1.12.0, H5Sget_select_hyper_blocklist will
  automatically combine those 1x1 selections into a single block before
  returning to the user.  It help mitigates the issue if the developer made
  mistakes mentioned above.  However, they removed this feature 1.12.0 and
  H5Sget_select_hyper_blocklist returns the selected blocks as is.

### Software Environment
  * HDF5 versions: 1.12.0
  * Environment settings: N/A
  * Trigger condition: Explicitly performed by the user

### Solutions
  * Ignore (current)
    + We left it to the user to use the library in an efficient way.
    + Adding the overhead of a merging routine to fix a possible developer mistake may not be worth it.
  * Implement merging in the VOL

---
## Log VOL does not report reference count to the underlying VOL it is using (Solved)
### Problem description
  Certain types of HDF5 objects can be closed by HDF5 automatically when no
  longer in use.  For those objects, HDF5 keeps a reference count on each
  opened instance.  The reference count represents the number of copies of the
  ID (the token used to refer to the instant) that currently resides in the
  memory.  Should the reference count reaches 0, HDF5 will close the instance
  automatically.  For simplicity, we refer to those objects as "tracked
  objects."

  To maintain the reference count, HDF5 requires the user to report new
  references to tracked objects (increase reference count) every time they copy
  the ID (such as assigning it to a variable) of the object.  When a copy of
  the ID is removed from the memory, the user has to decrease the reference
  count by calling a corresponding API.  If the reference count is not reported
  correctly, HDF5 may free the object prematurely while the user still needs
  it.

  VOLs are tracked objects.  If our log VOL uses the native VOL internally but
  does not report reference of the native VOL, HDF5 will close the native VOL
  when another internal routine that uses the native VOL finishes and decreases
  the reference count.  In consequence, our log VOL receives an error on all
  native VOL operations afterward.

### Software Environment
  * HDF5 versions: 1.12.0
  * Environment settings: N/A
  * Trigger condition: All API that triggers a reference count check (decrease) on the opened native VOL.

### Implemented Solution
  * Report reference our reference of the native VOL as the pass-through VOL does
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

---
## NetCDF4 support
### Problem description
  The log VOL fails when running the E3SM I/O benchmark (https://github.com/Parallel-NetCDF/E3SM-IO/tree/nc4) through NetCDF4 API on multiple processes.
  When we run the benchmark on more than one process, the native VOL underneath returns an error on an H5VLattr_specific call on a dataset attribute.
  The error source is that the native VOL fails to decode a packet (HDF5 internal file data structure) on an attribute.
  It does not happen when running on a single process.

  We found that the issue is related to the added attributes on datasets.
  The log VOL creates attributes on every dataset to store its internal metadata.
  If the log VOL does not create those attributes, the E3SM I/O benchmark runs without issue on multiple processes.
  We can recreate the issue in the pass_through VOL by applying modification in [nc4_attr_bug.patch](/nc4_attr_bug.patch).
  We believe it is either a bug in the native VOL or a limitation of the VOL interface.

### Software Environment
  * HDF5 versions: 1.12.0
  * NetCDF version: 4.7.4
  * Environment settings: N/A
  * Trigger condition: Use log VOL to run the nc4 branch of the E3SM I/O benchmark on multiple processes.

### Current solution (on going)
  * Discuss with the HDF5 development team about possible fix of the bug
  * The bug can be avoided by not adding attributes to datasets.
    + We must store dataset metadata elsewhere.
    + We only consider this workaround if the bug takes too long to fix.

---
## VOL fails when running E3SM on multi-process through NetCDF4 API
  The issue is still being studied.
  An H5Aiterate call failed on the second process due to some format decoding error.

---
## Native VOL won't close if there are data objects still open
### Problem description
  The native VOL keeps track of all data object it opens.
  If there are objects associated with a file still open, the native VOL will refuse to close the file and return an error.
  Our VOL wraps around the native VOL. Every object opened by our VOL contains at least one opened object by the native VOL.
  If the application does not close all opened objects before closing the file, the native VOL will report an error.
  Under our current implementation, the log VOL will abort whenever the native VOL returns an error.

### Software Environment
  * HDF5 versions: 1.12.0
  * Environment settings: N/A
  * Trigger condition: Call H5Fclose when there are other file objects left open.

### Potential solutions
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

---
## Native VOL is not available when the library is shutting down
### Problem description
  HDF5 dispatcher placed a hook on MPI_Finalize to finalize the HDF5 library.
  When finalizing, the dispatcher will close all files not closed by the application by calling the file close VOL API.
  During that time, the library is marked to be in shutdown status, and the function of the native VOL will become limited.
  Calling any native VOL function not related to shutting down (such as the internal function H5L_exists to check whether a link exists in the file) will get an error.

  Our VOL flushes the file when the file is closing.
  It relies on the native VOL to update metadata and to create log datasets.
  However, many functions it uses are not available while the library is in shutdown status (H5L_exists to check whether we already have the metadata log and the lookup table).
  Thus, our file closing API will always fail when called by the dispatcher on MPI_Finalize.

### Software Environment
  * HDF5 versions: 1.12.0
  * Environment settings: N/A
  * Trigger condition: Call MPI_Finalize without closing opened files.

### Possible solutions
  * Contact the HDF5 team to see if there is a way to reactivate the native VOL during the shutdown status
  * Delay file close until the last opened object is closed
    + HDF5 team suggests maintaining a reference count on the number of objects opened for every file. The file is only closed when the reference count reaches 0. The VOL close function will always return success after deducting the reference count, but the file will remain open as long as the reference count remains positive. In this way, the file will only be closed when the last object opened through it is closed.

---
## Registering property changes the signature of the property class
### Problem description
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
  If a VOL wants to create its own property, it must use the same H5Pregister API as applications.
  Our log VOL defines two customized properties.
  One is a file access property that indicates the amount of memory available for buffering data.
  Another one is a dataset transfer property to indicate whether an  I/O operation on a dataset is blocking or nonblocking.
  When the log VOL registers those properties during its initialization, it invalidates all existing file access and dataset transfer property lists.
  Also, H5P_DEFAULT can no longer be used to represent property lists of the two property classes.
  The issue is observed when trying to load the log VOL dynamically using environment variables.
  After loading and initializing the log VOL, the dispatcher tries to set it as the default VOL in the default file access property list (the property list used when H5P_DEFAULT is seen) by calling H5Pset_vol.
  H5Pset_vol returns an error because the default file access property list is no longer a file access property.

### Software Environment
  * HDF5 versions: 1.12.0
  * Environment settings: N/A
  * Trigger condition: Call H5Pregister2 to register a new file access property, then create a file using H5P_DEFAULT as file access property.

### Possible solutions
  * Contact the HDF5 team to see if there is a way to update all existing property list after the property class is extended.
  * Avoid registering new properties, try to create our own class.
    + Need to study the way to create our own property class.
  * Solution after discussing with the HDF5 team. Instead of changing the
    property class to include new properties, new properties are inserted
    directly into every property list that needs logvol properties to be set.
    To achieve this, the log VOl exposes a "Set" and "Get" API for each
    property introduced. In the "Set" API, the VOL check whether the property
    exists in the property. If not, the property is inserted into the property
    list.  Then, the value of the property can be in the property list as
    usual. In the "Get" API, the VOL check whether the property exists in the
    property. If so, the value is retrieved from the property list. Otherwise,
    the default value is returned.

---
## Improve Performance of Posting Dataset Write Requests
###  Problem Description
* If treating individual calls to H5Dwrite independently, then the log-based
  driver must call HDF5 APIs to query properties of a defined dataset in order
  to perform sanity checks on the user arguments. Such queries can be expensive
  when the number of H5Dwrite calls is high. This is particularly true for
  applications whose I/O patterns show a large number of non-contiguous subarrays
  requests, such as E3SM.
* HDF5 currently does not have an API to read/write a large number of
  non-contiguous subarray requests to the same variable. HDF5 group is currently
  developing "H5Dwrite_multi", a new API that allows writing multiple datasets
  in a single API call. See HDF5 develop branch ["multi_rd_wd_coll_io"](https://bitbucket.hdfgroup.org/projects/HDFFV/repos/hdf5/browse?at=refs%2Fheads%2Finactive%2Fmulti_rd_wd_coll_io).
* Before API "H5Dwrite_multi" is officially released, one must either treat a
  subarray request independently from others, i.e., each by a separate call to
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
* The caching strategy above can also help aggregate the request metadata to
  the same dataset into the same "entry" as a part of the
  [metadata index table](https://github.com/DataLib-ECP/log_io_vol/blob/master/doc/design_log_base_vol.md#appendix-a-format-of-log-metadata-table).
  This is particularly useful for E3SM I/O pattern that makes a large number of
  subarray requests to a variable after another.

---
## H5Dwrite does not allow writing to multiple blocks within a dataset
###  Problem Description
* The HDF5 API to write data to a dataset is H5Dwrite. 
  Applications specify the region within the dataset to write by selecting the region in the corresponding dataspace.
  The selection can either be a subarray (H5Sselect_hyperslab) or a list of points (H5Sselect_elements).
  Multiple selections can be combined to select an irregular region.
* Although HDF5 allows applications to select multiple blocks by combining multiple hyper-slab selections, 
  HDF5 interprets them as a unit instead of a list of hyper-slabs.
  HDF5 assumes that the data in the memory buffer is ordered according to its file offset in a contiguous data layout.
  For example, in a 2-D dataset, HDF5 will always write the dataset row-wise even if the application makes selections column-wise.
* In the E3SM case study, each process writes many blocks (subarrays) in every dataset.
  The data is arranged block-wise in the memory.
  Based on how HDF5 dataspace selection works, the program must either reorder its data in the memory or call H5Dwrite for each block.
  In the high-resolution (ne120) F case, there are 2.7 M H5Dwrite calls per process. It took around 10.5 sec just for posting the I/O operation with H5Dwrite alone.
* PnetCDF has a set of API (*varn) that allows applications to write multiple blocks within a variable with data arranged by blocks in the memory.
  The varn API gives PnetCDF a huge advantage in posting write requests.

### Software Environment
* HDF5 versions: All

### Solutions
* Provide a customized H5Dwriten API inside the log-based VOL
  + Works similar to varn APIs in PnetCDF.
  + The signature of H5Dwrite (H5VLdataset_write from within the VOL) cannot be modified.
    The dataspace argument is the only argument to indicate the selected regions to write.
    A simple approach is to have the log-based VOL interpret the data in the memory buffer as following the order of dataspace selections.
    However, doing so will alter H5Dwrite semantic, causing compatibility issues.
  + We implemented a separate H5Dwriten function in the log-based VOL.
    Instead of taking a dataspace that contains the selection, the H5Dwriten function takes a list of start and count similar to PnetCDF varn API.
    It writes data directly to the log and record all subarrays in the metadata dataset.
  + Inside the H5Dwriten function, the library needs to retrieve the correct VOL object from the ID (hid) passed by the application.
    The current VOL architecture does not include a VOL API to retrieve VOL objects by IDs (hid_t).
    To retrieve the VOL object from an ID (hid_t), we must call an HDF5 user-level API.
    The HDF5 dispatcher will then call the corresponding VOL callback function with the desired VOL object.
  + We introduce a new dataset transfer property called "H5VL_log_multisel"
    It contains a list of blocks to write in the form similar to the arguments of *varn API in PnetCDF.
    When this property presents in the dataset transfer property list (dxpl), the H5Dwrite call is interpreted as a H5Dwriten call.
    The log-based VOl will disregard the dataspace and use the block list stored in the property as blocks to write, and the data in the memory buffer is assumed to be ordered by blocks.
    The H5Dwriten function sets the H5VL_log_multisel property with arguments passed from the application and calls H5Dwrite.
  + Using H5Dwriten reduces the number of H5Dwrite calls in the high-resolution 
    (ne120) E3SM F case dataset on 1024 processes from 2755063 to 413.
    The time spent on posting write requests reduced from 10.9 sec to 1.7 sec 
    when running on 32 Cori Haswell nodes (32 processes/node).
---
## Memory space is always required in H5Dwrite even if the memory buffer is contiguous
### Problem Description
* According to the specification of H5Dwrite 
  (https://support.hdfgroup.org/HDF5/doc1.6/RM_H5D.html#Dataset-Write) takes a 
  dataspace argument called mem_space_id to describe data layout in the memory buffer. 
  It must be either an allocated dataspace ID or H5S_All. If mem_space_id is H5S_All, 
  the manual states that "The file dataset's data space is used for the memory dataspace 
  and the selection specified with file_space_id specifies the selection within it.".
  That is, if the selection in the file_space_id is non-contiguous, the data in 
  the memory buffer is also assumed to be non-contiguous. 
* Many NetCDF applications are designed to use contiguous memory buffer when writing
  to a variable regardless of the selection. Since there is no built-in dataspace 
  to indicate that the data in the memory is contiguous in HDF5, the developer will 
  need to create a data space for memory buffer on each H5Dwrite call when porting 
  their applications to HDF5. It creates unnecessary overhead that can add up when 
  there are a large number of calls.

### Software Environment
* HDF5 versions: All

### Solutions
* The logvol exposes a macro H5S_CONTIG that expands to a preallocated dataspace ID 
  within the logvol. (Implemented)
  If the dataspace id received by logvol functions as mem_space_id matches H5S_CONTIG,
  the logvol interpret it as a contiguous memory buffer.
  H5S_CONTIG is a scalar dataspace logvol created internally. It serves as a place 
  holder to prevent the same ID to be used in other H5S_create calls.
  H5Sclsoe is not available at VOL finalize stage. Due to this reason, we cannot 
  create and close H5S_CONTIG in the vol init and finalize routine. Instead, we do 
  it for every file open and close. The logvol maintains a reference count on opened
  files, so only the first H5Fopen call creates H5S_CONTIG, and only the last H5Fclose call closes it.
* Another option is to use the same idea of caching proposed for posting write
  requests to avoid querying memory layout metadata. The log-based driver can
  check if the space identifier is the same as previous one. If it is the same,
  then a call to "H5Sget_simple_extent_dims" for example can be skipped.
  + This can be very useful for E3SM, as the majority of memory spaces of its
    contiguous requests share the same dimensions and sizes. For example, the
    I/O pattern of F case below shows the minimum and maximum sizes of
    contiguous write requests are 1 and 3, respectively, for decompositions 2
    and 3. The I/O pattern of G case shows the sizes of all contiguous write
    requests are 80 for decompositions 3, 4, and 5 and 81 for decomposition 6.
    Both cases indicate a lot of write requests sharing the same amounts.
    ```
    For decomposition file f_case_72x777602_21600p.nc
    Var D1.lengths: len=   43200 max= 28 min= 10
    Var D2.lengths: len=  292513 max=  3 min=  1
    Var D3.lengths: len=21060936 max=  3 min=  1

    For decomposition file g_case_11135652x80_9600p.nc
    Var D1.lengths: len= 3692863 max=  2 min=  1
    Var D2.lengths: len= 9804239 max=  7 min=  1
    Var D3.lengths: len= 3693225 max= 80 min= 80
    Var D4.lengths: len=11135652 max= 80 min= 80
    Var D5.lengths: len= 7441216 max= 80 min= 80
    Var D6.lengths: len= 3693225 max= 81 min= 81
    ```
  + To make use of the caching idea above, **E3SM-IO benchmark** program must
    be adjusted to reuse the memory space identifier if the write request's
    memory layout is the same.

---
## H5VLDataset_write do not follow collective property
### Problem Description
* It is a confirmed HDF5 bug. The native VOL ignored the dataset transfer property passed from the dispatcher.
  Instead, it uses the default dataset transfer property set in HDF5.
  The default property is set in H5Dwrite, so the bug does not affect user applications.
  However, when VOLs call H5VLDataset_write (the internal version of H5Dwrite for VOLs that takes OVL object instead of hid) form within the HDF5 library, the default property is not set, and the native VOL will always use independent I/O. 
* The LOG VOL stores the metadata table in an HDF5 chunked dataset called the metadata dataset. 
  It relies on the native VOL to write to the metadata dataset when flushing the metadata.
  The performance can be heavily degraded when the native VOL uses independent I/O on the metadata dataset.
* In a case of IOR benchmark running on 256 processes on 8 cori nodes, it took ~30 sec to write the metadata dataset that is only 12 KiB in size.
  The darshan log from the test, [ior_darshan.txt](/ior_darshan.txt)(https://github.com/DataLib-ECP/log_io_vol/blob/760a8cbb9f29fe53a6f89ad04594a633d6966457/ior_darshan.txt), shows unusually high H5Fclose time.

### Software Environment
* HDF5 versions: 1.12.0

### Solutions
* Replace chunked metadata dataset with a set of contiguous datasets.
  As a workaround, we can use MPI-IO to write the metadata directly.
  For a simpler design, we replace the chunked, extendable metadata dataset with contiguous datasets.
  We create a new metadata dataset when we need to store more metadata.
  The log-based VOL retrieves the file offset of the metadata dataset from the native VOL and then write the metadata in a similar way it writes log data.
  After switching to contiguous metadata datasets, metadata flush time on F case high-resolution (ne120) dataset reduced to around 6 sec.
* Collaborate with the HDF5 team to study the reason the native VOL did not follow the setting in the property list.

---
## The native VOL does now use MPI file view when performing an independent write to chunked datasets
### Problem Description
* When using the MPI VFD driver, H5Dwrite can either perform a collective or independent write to the dataset.
  For a chunked dataset, an H5Dwrite operation can write to multiple chunks, and hence, multiple locations in the file.
  Writing to multiple files offset in a single MPI file write call requires the use of file view to mask off the gap between chunks.
  The native VOL does not use the file view when writing to a chunked dataset independently.
  Instead, it writes one chunk at a time by calling the MPI file write on each individual location to write. 
  The performance may suffer if there is an independent write involving a large number of chunks.
  The reason behind such a design is unknown.
* The log VOL stored the log metadata (index) in a chunked HDF5 dataset.
  Chunking is required because the metadata, and hence the metadata dataset, can grow when more data is written to datasets.
  The log-based VOL relies on the native VOL to write metadata to the metadata dataset.
  In conjunction with the bug that ignores collective property, this issue results in extremely slow metadata flushing.
* In the E3SM case study, it took around 195 sec to write 8 GiB of metadata on the F case high-resolution (ne120) dataset. 

### Software Environment
* HDF5 versions: 1.12.0

### Solutions
* Replace chunked metadata dataset with a set of contiguous datasets.
  As a workaround, we can use MPI-IO to write the metadata directly.
  For a simpler design, we replace the chunked, extendable metadata dataset with contiguous datasets.
  We create a new metadata dataset when we need to store more metadata.
  The log-based VOL retrieves the file offset of the metadata dataset from the native VOL and then write the metadata in a similar way it writes log data.
  After switching to contiguous metadata datasets, metadata flush time on F case high-resolution (ne120) dataset reduced to around 6 sec.
 
---
## HDF5 dispatcher creates a wrapped VOL object but never free it
### Problem Description
* HDF5 allows applications to keep data objects open after the file containing them is closed. 
  Applications should be able to access those objects normally.
* The log VOL support this feature by delaying the file close until all data object is closed. 
  Internally, it keeps a reference count on every file representing the number of opened objects that belong to the file. 
  The count is initialized to 1, representing the file itself.
  It is increased when an object is opened from the file and decreased when the object is closed.
  The file closing API in log VOL does not actually close the file but decreases the reference count by 1.
  When the reference count reaches 0, the log VOL closes the file.
* There is a bug in the HDF5 dispatcher. After creating a new file, the dispatcher calls the VOL wrap API to create a wrapped file object.
  The wrapped file object is never closed. It prevents the VOL from closing the file indefinitely.
* HDF5 registers a callback in MPI_Finalize to clean up the library. The cleanup routine ensures all open HDF5 objects are closed.
  It will call the corresponding VOL's close API and check if the object is closed afterward.
  After few tries to have the log VOL close the file, it assumes the log VOL has crashed and calls abort.

### Software Environment
* HDF5 versions: 1.12.0

### Solutions
* We have reported the bug to the HDF5 development team.
  + A bug report (https://jira.hdfgroup.org/browse/HDFFV-11140) is created

---
## Metadata size can be larger than data size
### Problem Description
* In our original design, the log-based VOL creates a metadata entry per
  hyper-slab selection (subarray request) at each H5Dwrite call and each
  metadata entry stores the following members:
  + The ID of the dataset (a 4-byte integer)
  + A flag describes state the data (a 4-byte integer)
    + Endianess of the log data stored in file.
    + Filters applied to the data
    + Metadata format (allow different version of metadata encoding)
  + Starting coordinate of the subarray selection (8 byte * [number of dataset dimensions])
  + Size of the selection along each dimension (8 bytes * [number of dataset dimensions])
  + Write request's starting offset in the log dataset (8 bytes)
  + Size of write request (8 bytes)
* The total size of each metadata entry is (24 + 16 * [dataset dimension]) bytes.
* For I/O patterns containing a large number noncontiguous requests, such
  as E3SM-IO, the sum of metadata size can be larger than the data size.
* We observed up to 3 times of the metadata size (86.4 GiB) to the data size
  (14.4 GiB) in the mid-size E3SM ne120 F case study. The cost of writing
  metadata negates the performance advantage of using the log-based I/O to
  avoid the cost of inter-process communication to arrange data in its
  canonical order.

### Software Environment
* HDF5 versions: All

### Solutions
* Combine metadata entries that belong to the same dataset.
  + When applications call H5Dwrite multiple times on the same dataset, each
    using a different hyper-slab selection, we can combine metadata entries
    that belong to the same dataset.  The only difference among entries is in
    the selection and the location of the write requests.  The dataset ID and
    flag can be shared among entries and thus saving a space of 8 bytes.
* Encode each selection with its starting and ending coordinates
  + Instead of storing the start indices and request lengths of a subarray
    request, we can reduce the space by storing only the (flattened) starting
    and ending offsets, which takes only 16 bytes of space regardless of the
    dataset's dimensionality.
  + The starting index and request length along each dimension of a request
    can be reconstructed from the two starting and ending offsets, given
    the dataset's dimension sizes.
  + For datasets containing dimensions that are expandable.
    * Because changing the dimension shape of a dataset will change the
      canonical offset of each array element, we must store the dataset's
      current shape (8 bytes * [number of dimensions]), so the starting
      indices and request lengths can be decoded correctly.
    * Note this representation can only reduce the metadata size for datasets
      of 2 or more dimensions when there are more than one selected hyper-slabs
      requests.
* Store data that belong to the same dataset in the adjacent space in the file.
  + It can be achieved in 2 ways:
    * Reordering the same dataset's data to adjacent space when flushing, then 
      combine the metadata entries as described in the first point. It involves 
      only memory operations and does not affect the I/O pattern.
    * Use our customized H5DwriteN API to generate a log entry with multiple 
      blocks that are flushed together (into contiguous space).
  + If multiple blocks of data are stored contiguously in the file, the file 
    offset of a block can be calculated from the file offset of the previous 
    block and the previous block's size. We only need to store the first block's
    file offset to calculate the file offset of the remaining blocks. It reduces
    the metadata size of the remaining blocks (except the first block) by 25%.
  + The block size in the file is the same as the size of the selection in the 
    dataset. We have a file size field in the metadata to support compression or 
    other types of filters. If we filter (compress) all the blocks together, we 
    can eliminate the blocks' file size field after the first block. Doing so 
    gives another 25% reduction in metadata size. A side effect is that all the 
    blocks must be decompressed to read even just one of them.
* Compress the metadata entries
  + When metadata contains a long list of noncontiguous requests, we consider
    to apply data compress on the starting and ending offsets to further reduce
    its size.
  + We enables compression for a dataset when the number of its subarray selections
    exceeds a threshold (currently set to 128).
  + When using ZLIB, we observed a significant metadata size reduction for the
    mid-size E3SM ne120 F case, running 1024 MPI processes on Cori Haswell nodes,
    32 processes per node.
    + Total metadata size is reduced from 38.98 GiB to	4.39 GiB, a compression
      ratios of 8.89.
    + Compression time: 2.90 sec
    + Time of writing metadata is reduced from 199.77 sec to 195.21 sec. Note this
      measurement is done before the
      [HDF5 collective I/O mode bug](#h5vldataset_write-do-not-follow-collective-property)
      is fixed.
      
* The above revised metadata format is now containing the following components:
  + The ID of the dataset to write (4 bytes)
  + A flag containing various state of the data (4 bytes)
    + Endianness of the data in the log dataset
    + Filters applied to the data
    + Metadata format (allow different versions of metadata encoding)
      + Single selection (A)
      + Multiple selections (B)
      + Multiple selections with data blocks stored contiguously in the file (C)
    + Filters applied to metadata
  + Number of selected hyper-slabs (subarray access, 8 bytes)
    + Only appear in metadata format (B) and (C)
  + A list of selections (can be compressed)
    + Starting coordinate of the selection (8 bytes)
    + Ending coordinate of the selection (8 bytes)
    + Starting position of the data in the log dataset (8 bytes)
      + Only appear in the first selection in metadata format and (C)
    + Size of data in the log dataset (8 bytes)
      + Only appear in the first selection in metadata format and (C)
---

## The log-base VOL does not check for dataset write boundary
### Problem Description
* For now, the log-base VOL does not perform a sanity check on dataspace selection.
  On a write operation, the VOL takes the data and metadata as-is and records them in the LOG.
  On a read operation, the VOL searches through LOG entries for matching records without considering the dataspace's boundary.
* It is a feature work to check whether a given dataspace selection is within the dataset's space.
### Software Environment
* HDF5 versions: All

### Solutions
* Implement sanity check in dataspace selection boundary on dataset I/O.

---
## HDF5 cannot be configured on Theta
###  Problem Description
* Trying to configure HDF5 on ALCF Theta results in the following error.
  ```
  CC=cc ./configure --prefix=/home/khou/.local/hdf5/1.12.0 --enable-parallel --enable-hl --enable-build-mode=production --enable-shared
  ...
  configure: error: cannot run C compiled programs.
  If you meant to cross compile, use `--host'.
  See `config.log' for more details
  ```
* The message suggests enabling cross-compiling, but HDF5 does not support it. Enabling cross-compiling will result in the following error.
  ```
  CC=cc ./configure --prefix=/home/khou/.local/hdf5/1.12.0 --enable-parallel --enable-hl --enable-build-mode=production --enable-shared --host=cray
  ...
  configure: error: in `/home/khou/hdf5/1.12.0':
  configure: error: cannot run test program while cross compiling
  See `config.log' for more details
  ```
* On Theta, compute nodes have KNL CPUs while login nodes have Haswell CPUs.
  The error is related to the AVX512 instruction, which is available on KNL, but Haswell CPUs. 
  The default compiler (cc) on Theta builds programs intended to run on compute nodes and hence contains the AVX512 instruction.
  The configure script checks whether the compiler works by compiling and running a test program.
  Since the login node does not support the AVX512 instruction, the test program fails to run, and the configure script reports the error above.
  
### Software Environment
* HDF5 versions: 1.12.0, 1.10.1

### Solutions
* Use the toolchain for Haswell CPUs when configuring HDF5
  + Programs built for Haswell CPUs will run on both login nodes and compute nodes.
  + According to the ALCF helpdesk, using the Haswell toolchain will not affect HDF5 performance on KNL compute nodes.
  + HDF5 calls dlopen to open the shared VOL object if the VOL is not statically linked to the application.
    + dlopen requires libifcoremt.so which is not in the default library path on compute nodes.
    + The program crashes on compute nodes as the system cannot find libifcoremt.so.
    + Adding the directory containing the correct libifcoremt.so solves the problem.
      + export LD_LIBRARY_PATH=/opt/intel/compilers_and_libraries_2020.0.166/linux/compiler/lib/intel64:#{LD_LIBRARY_PATH}
    + Changing the default linking mode to dynamic to make sure we are actually using the libifcoremt.so in LD_LIBRARY_PATH
      + export CRAYPE_LINK_TYPE=dynamic
  + Steps
  ```
  module swap craype-mic-knl craype-haswell 
  CC=cc ./configure --prefix=/home/khou/.local/hdf5/1.12.0 --enable-parallel --enable-hl --enable-build-mode=production --enable-shared
  module swap craype-haswell craype-mic-knl 
  ```
---