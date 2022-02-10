------------------------------------------------------------------------------
This is essentially a placeholder for the next release note ...
------------------------------------------------------------------------------
(Copy the contents below to RELEASE_NOTE.md when making an official release.)

* New features
  + Support multiple opened instance of the same dataset.

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
  + H5Ovisit, H5Literate, and H5Lvisit.

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
  + Add command-line option "-d" to utility program "h5ldump" for dumping data
    of each selection block.
  + h5dump can now read a log-based VOL output file using log-based VOL.

* Other updates:
  + Add utility scripts ("utils/h5lenv.bash.in" and "utils/h5lenv.csh.in") to
    set HDF5 environment to use log-based VOL.
  + This log-layout based VOL has been registered with HDF5 group with
    Connector Identifier 514.
    https://portal.hdfgroup.org/display/support/Registered+VOL+Connectors
  + Change the required HDF5 library from the develop branch to official
    release of 1.13.0.
  + Support build on MacOS.
  + Solve compatibility issues with OpenMPI.
  + Use "MPI_COMM_SELF" if the communicator is not set to allow single process I/O.

* Bug fixes
  + Fix missing starts and counts assignments in example programs "examples/dwrite_n.cpp and "examples/dread_n.cpp".
  + Fix bug on metadata flush when some processes have no data to flush.
    + Avoid creating empty compound datatypes, as some MPI implementation does not allow it.
  + Fix H5Oopen on dataset crashing issue.
  + Fix mistyped variable data type for file info.
  + Fix bug on metadata encoding not following the file configure flag.
  + Fix bug on generating MPI datatype for reading.
    + Check subarray interleaving only when the target falls into the same block of data.
    + File offset must include selection block offset.
  + Fix bug for accessing scalar and 1-D datasets.
  + Fix referenced metadata decoding bug.
  + Fix using the wrong MPI communicator on metadata flush barrier.
  + Fix metadata flag bug after metadata compression fails.
  + Fix a bug that incorrectly erases metadata index.
  + Fix dataset I/O bug when using "H5S_ALL".
  + Fix calculation for determining whether the file space is interleaving.
  + Fix a bug when memory space and file space have different number of dimensions.

* New example programs
  + none

* New programs for I/O benchmarks
  + none

* New test program
  + Added tests of parallel runs. Command "make ptest" will run test programs
    in parallel using 4 MPI processes.
  + A new test program 'tests/testcases/scalar.cpp'  to read/write test scalar
    datasets.

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

