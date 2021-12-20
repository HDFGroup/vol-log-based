## Log Layout Based HDF5 Virtual Object Layer

### Version _VERSION_ (_RELEASE_DATE_)
* New features.
  + Support parallel test.
    + run make ptest to run test programs in parallel.
  + Support data dump in h5ldump.
    + add -d command-line option to dump data of each selection block.
* New test programs.
  + Scalar.
    + Read/write scalar datasets.
* Bug fixes.
  + Correct missing starts and counts assignments in example program dwrite_n and dread_n.
  + Fix bug on metadata flush when part of the processes have no entry to flush.
    + Some MPI implementation does not allow creating empty compound datatype.
  + Fix H5Oopen on dataset crashing issue.
  + Correct mistyped variable type for file info.
  + Fix bug on metadata encoding not following the file configure flag.
  + Fix bug on generating MPI datatype for reading.
    + check subarray interleaving only when the target falls into the same block of data.
    + File offset must include selection block offset.
  + Fix bug on scalar and 1-D datasets.
  + Fix referenced metadata decoding bug.
  + Fix bug on metadata flush barrier using the wrong communicator.
  + Fix metadata flag bug after metadata compression fails.
  + Fix metadata index wrongly erased bug.
  + Fix dataset I/O bug when using H5S_ALL
* Other updates.
  + Registered with HDF5 with ID 514.
  + Switch from HDF5 develop branch to 1.13.0.
  + Support to macOS build.
  + Solve compatibility issues with OpenMPI.
  + Use MPI_COMM_SELF if the communicator is not set to allow single process I/O.

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
