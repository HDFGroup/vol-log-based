# Note for Developers

### Table of contents
- [Known problem](#characteristics-and-structure-of-neutrino-experimental-data)

---

# Known issues

```
General comments: I suggest the followings when adding a new issue.

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

## HDF5 native VOL dynamic dataype init

The predefined datatypes are not defined as constants in HDF5 header files.
Instead, they are defined as global variables and initialized by the native VOL when the user's VOL is initialized.
As a result, we need to initialize the native VOL before using any derived datatype.
H5Fcreate will call the initialization callback function if the VOL supports it.
It requires harnessing the introspect interface so the dispatcher knows whether the VOL support the init call back.
Also, the file optional interface, which includes the init callback, must be implemented as well.
```
* Add reference to "VOL dynamic datatype init" in HDF5 doc.
* "derived datatype"? Do you mean compound data type?
* Please refer to the source code locations of your fixes.
```

## HDF5 H5Sget_select_hyper_nblocks breakdown selection

It is a behavior observed in version 1.12.
H5Sget_select_hyper_nblocks always break down the selection into unit cells even the selection is contiguous and non-interleving
Even when the selection is a row of a 2-D data space, the returned hyperblocks from H5Sget_select_hyper_nblocks are individual cells.
```
Please update this issue, as we have figured out it is our misunderstanding of H5Sselect_hyperslab.
See https://github.com/NU-CUCIS/ph5concat/commit/e651e83169257cb44b98473970927d34c6cd4c31
```

## HDF5 Dispatcher calls file close if any object reains open on MPI_Finalize

If the application left any HDF5 object open on MPI_Finalize, the dispatcher would call the VOL file close.
If the file has been closed by the application a prior, it causes a double-free error.
A potential solution is to implement delayed close as the native driver does.
```
This description is not clear. Do you mean HDF5 will close the still-opened objects during MPI_Finalize?
If the applications close the file after done with the file, why would HDF5 native VOL still want to free the file object?
```
## HDF5 reference count hasn't been implemented
```
Is this title referring to HDF5 native VOL or our log-based VOL?
```
This is temporarily solved by following the way the passthrough VOL report reference counts.

HDF5 dispatcher includes a reference count mechanism to track opened objects.
When the reference count of an object becomes 0, the dispatcher will try to recycle it.
This feature requires the `(user's?)` VOL to maintain the reference count of the objects it uses.
Without it, the dispatcher may not be aware that an object is still used by the VOL and tries to recycle it.
```
I assume each HDF5 API calls the dispatcher.
When the dispatcher detects a zero count to an object in an HDF% API, does it immediately free the internal data structures allocated for that object?
Please clarify whether this is done in Native or user's VOL.
```

## NetCDF4 support

NetCDF uses many advanced HDF5 features to implement NetCDF4.
```
What are the advanced HDF5 features? Give a few examples and references.
```
To support the NetCDF integration, we need to significantly improve the compatibility of the VOL beyond our original plant of attribute, group, and attributes.
Most of them can be tackled by connecting to the native VOL, but some require special handling.
```
This statement is vague, need to be specific. Use examples.
```
Currently, the log-based VOL does not support "links", "tokens", "requests", and "optional" APIs.
It partially supports "object" and "context" APIs.
```
These need to be specific by giving names of APIs and HDF5 doc if there is any.
```

## Crash if data objects are not closed

Currently, the `(log-based ?)` VOL does not maintain reference counts to opened objects.
It relies on the application to properly close every object they opened.
If the application does not close objects opened previously (such as datasets) before closing the file, the dispatcher will consider the file as not closed and call the VOL file close again when it finalizes `(during MPI_Finalize?)`.

However, our log-based VOL already freed the file object, and the pointer passed from the dispatcher becomes invalid, causing the program to crash. 
```
This statement is not clear.
For example, if user application programs do not close a dataset but close the file, then the HDF5 dispatcher will try closing the dataset and then closing the file. While closing the file, the already closed file will cause a crash?
```
One way to solve it is to close all objects automatically.
```
DO you mean that our log-bsed VOL keeps track of opened objects and closes them when H5Fclose is called by the user program?
```
Another way is to implement delayed close as the native VOL.
```
Is this solution better? i.e. less error prone?
```
The VOL `(native or our log-based?)` maintains a reference to all opened objects.
If there are objects still open, they `(our log-based VOL?)` do not close the file, but just mark the file to be closed.
If the reference becomes empty (all object is closed), it will close the file if it is marked to be closed.

---

