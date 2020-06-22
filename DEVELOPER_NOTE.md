# Note for Developers

### Table of contents
- [Known problem](#characteristics-and-structure-of-neutrino-experimental-data)

---

# Known problem

## HDF5 native VOL dynamic dataype init

The predefined datatype are not defined as constants in HDF5.
They are global variables initialized by the native VOL when the VOL get initialized.
As a result, we need to initialize the native VOL before using any derived datatype.
H5Fcreate will call the initialization callback function if the VOL supports it.
It requires harnessing the introspect interface so the dispatcher knows whether the VOL support the init call back.
Also, the file optional interface, which includes the init callback, must be implemented as well.

## HDF5 H5Sget_select_hyper_nblocks breakdown selection

It is a behavior observed in version 1.12.
H5Sget_select_hyper_nblocks always break down the selection into unit cells even the selection is contiguous and non-interleving
Even when the selection is a row of a 2-D data space, the returned hyperblocks from H5Sget_select_hyper_nblocks are individual cells.

## HDF5 Dispatcher calls file close if any object reains open on MPI_Finalize

If the application left any HDF5 object open on MPI_Finalize, the dispatcher would call the VOL file close.
If the file has been closed by the application a prior, it causes a double-free error.
A potential solution is to implement delayed close as the native driver does.

## HDF5 reference count hasn't been implemented

HDF5 dispatcher includes a reference count mechanism to track opened objects.
When the reference count of an object becomes 0, the dispatcher will try to recycle it.
This feature requires the VOL to maintain the reference count of the objects it uses.
Without it, the dispatcher may not be aware that an object is still used by the VOL and tries to recycle it.

---

