
# VOL functions
* Framework - Done
  + Modify form passthrough VOL
  + Autoconf script
* File creation and internal objects - Done
  + Create the file along with all internal data object used by the VOL
* Dataset creation - Done
  + Create the dataset along with all internal attributes
  + Dataset metadata query
* Dwrite and flush - Done
  + Internal buffer for blocking Dwrite
  + Property to set the buffer size
  + Property to toggle non-blocking I/O
  + Collective write of aggregated data to the log dataset
* Metadata table - Done
  + Table generation when closing the file
  + Parsing the metadata table after opening the file
  + Merging new entries to existing metadata table - TODO
* Dread - Done
  + Searching through metadata
  + Generate file and memory type for MPI read
  + Support overlapping read
* Nonblocking Dread - Done
  + Searching through metadata across datasets
  + Generate file and memory type for MPI read across datasets
  + Support overlapping read across datasets
* Datatype and endianness conversion - Done
* Memory space selection - DONE
  + Allow memory space selection in Dwrite
  + Allow memory space selection in Dread
* Test programs
  + Basic test for VOL functions - Done
  + Comprehensive test with different I/O patterns - TODO
* Async I/O - Future work

# Status of HDF5 API supported
## Supported
* H5Fcreate - Jan 26 2020
  Create the HDF5 and all hidden data objects used by the VOL
  Cache file metadata (not the metadata table)

* H5Fopen - Jan 26 2020
  Open the HDF5 and all hidden data objects used by the VOL
  Cache file metadata (not the metadata table)

* H5Fclose - Jan 26 2020
  Clase all hidden data objects used by the VOL and the HDF5 file
  Flush all staged I/O request
  Build the metadata table
  Write-back file metadata

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

* Non-blocking I/O and property
* Overlapping read
* Type conversion

## Work in progress
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
