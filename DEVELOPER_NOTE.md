# Note for Developers

### Table of contents
- [Logvol file format proposals](#characteristics-and-structure-of-neutrino-experimental-data)

---

# Progress
## Done
* H5Fcreate - Jan 26 2020
  Create the HDF5 and all hidden data objects used by the VOL
  Cache file metqadata (not the metadata table)

* H5Fopen - Jan 26 2020
  Open the HDF5 and all hidden data objects used by the VOL
  Cache file metqadata (not the metadata table)

* H5Fclose - Jan 26 2020
  Clase all hidden data objects used by the VOL and the HDF5 file
  Flush all staged I/O request
  Build the metadata table
  Wrtie back file metqadata

* H5Fflush Mar 23 2020
  Flush all staged I/O request
  Mark the metadata table as outdated

* H5Dcreate Mar 11 2020
  Create the anchor dataset
  Cache dataset metadata (ndim, type ... etc)

* H5Dopen Mar 11 2020
  Open the anchor dtaset
  Cache dataset metadata (ndim, type ... etc)

* H5Dclose Mar 11 2020
  close the anchor dataset

## Working
* H5Dread
  Stage a read request in the read queue.
  If the call is non-blocking, flush the request immediately.

* H5Dwrite
  Stage a write request in the write queue.
  If the call is non-blocking, make a copy of the data.

## ToDo
* H5Dget*
* H5Dset*
* H5Fis_hdf5

* Non-blocking I/O and property
* Async I/O
* Memory space
* Overlapping read
* Type conversion

## Do not need special care (passthrough)
* H5A* - Mar 3 2020
* H5G* - Mar 3 2020
* H5Fget* - Mar 23 2020
* H5T*
* H5P*

## No plan to support
* H5Dgather
* H5Dscatter
* H5Diterate
* H5Dvlen*
* H5Dfill
* H5Dcreate_anon
* H5Fget_file_image
* H5Freopen
* H5Fmount
* H5Funmount
* H5Fget*
* H5Fset*

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

---

