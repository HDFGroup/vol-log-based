## Log Layout Based HDF5 Virtual Object Layer

### Version _VERSION_ (_RELEASE_DATE_)
* New features.
  + Support parallel test.
      + run make ptest to runt est programs in parallel.
* Bug fixes.
  + Correct missing starts and counts assignments in example program dwrite_n and dread_n.
  + Fix bug on metadata flush when part of the processes have no entry to flush.
    + Some MPI implementation does not allow creating empty compound datatype.
  + Fix H5Oopen on dataset crashing issue.
  + Correct mistyped variable type for file info.
* Other updates.
  + Switch from HDF5 develop branch to 1.13.0.
  + Support to macOS build.
  + Solve compatibility issues with OpenMPI.

### Version 1.0.0 (December 2, 2021)
* First public release of log-based VOL.
* Features implemented.
  + Support major HDF5 APIs.
  + Documentation on log-based VOL APIs.
  + Utility programs.
    + h5ldump - Dump the log metadata in a log-based VOL outptu file.
    + h5lreplay - Convert log-based VOL output file to traditional HDF5 file.
  + Utility script for querying configuration and installation options.
    + h5lconfig 
  + Example programs to demonstrate the use of log-based VOL.
