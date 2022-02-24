------------------------------------------------------------------------------
This is essentially a placeholder for the next release note ...
------------------------------------------------------------------------------
(Copy the contents below to RELEASE_NOTE.md when making an official release.)

* New features
  + Flush write requests before handling any read requests.

* New optimization
  + none

* New Limitations
  + none

* Update configure options
  + none

* New constants
  + none

* New APIs
  + none

* New native HDF5 APIs
  + none

* API syntax changes
  + none

* API semantics updates
  + none

* New error code precedence
  + none

* Updated error strings
  + none

* New error code
  + none

* New I/O hint
  + none

* New run-time environment variables
  + none

* Build recipes
  + none

* Updated utility program
  + none

* Other updates:
  + Supports NetCDF4 applications.

* Bug fixes
  + h5ldump always use the native VOL regardless of HDF5_VOL_CONNECTOR. 
  + h5lreplay must query dcplid before extracting filters.
  + h5lreplay prefix internal objects by double '_'.
  + Fix metadata flush after read operations corrupts the file.
  + Fix write reqeust not flushed properly after the first metadata flush.
  + Fix memory error when writing filtered datasets.

* New example programs
  + none

* New programs for I/O benchmarks
  + none

* New test program
  + none

* Conformity with HDF5 library
  + none

* Discrepancy from HDF5 library
  + none

* Issues related to MPI library vendors:
  + none

* Issues related to Darshan library:
  + none

* Clarifications
  + none

