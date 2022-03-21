## Release Notes of the Log Layout Based HDF5 Virtual Object Layer

### Version 1.1.0 (Feburary 11, 2022)
* New features
  + Support multiple opened instance of the same dataset.
    + Commit 37eed305581653cbb1e021a3c23389a55043b2e5
* New optimization
  + Coalesce hyper-slab and points selections to reduce metadata size and index searching time on reading.
    + Commit 181abe3094e0e961e7e04bb76bab6e6ac45cbfd1
  + Improve indexing memory consumption on dataset read with a redesigned compact index data structure.
    + Commit 3b08ff37f43dae46de74408ef15871ccf1f23b02
* New native HDF5 APIs
  + H5Ovisit, H5Literate, and H5Lvisit.
* Bug fixes
  + Fix missing starts and counts assignments in example programs "examples/dwrite_n.cpp and "examples/dread_n.cpp".
    + Commit bab2f2f03c1a6c9ed179ef18062366874ef72e1b
  + Fix bug on metadata flush when some processes have no data to flush.
    + Avoid creating empty compound datatypes, as some MPI implementation does not allow it.
    + COmmit ea081e8cf479336244337e4d9411b380c6fccdb3
  + Fix H5Oopen on dataset crashing issue.
    + Commit 36ff7f323b8264d24a7876f882dd39ffdb03acb9
  + Fix mistyped variable data type for file info.
  + Fix bug on metadata encoding not following the file configure flag.
    + Commit c8003451ab540b32ed6392646d0ebb671020ce36
  + Fix bug on generating MPI datatype for reading.
    + Check subarray interleaving only when the target falls into the same block of data.
    + File offset must include selection block offset.
  + Fix bug for accessing scalar and 1-D datasets.
    + Commit bc3b94d77a00be583e8d6b668d12fdc9b8e6c2da
  + Fix referenced metadata decoding bug.
    + Commit e806234498cf605fad9f4d861396aca4caf8af89
  + Fix using the wrong MPI communicator on metadata flush barrier.
    + Commit 4df75f826a2f81ef1329250f1e02fcd5a33484a9
  + Fix metadata flag bug after metadata compression fails.
    + Commit 34609d90a752b9b0bac574ed76d4ad602bd3540a
  + Fix a bug that incorrectly erases metadata index.
    + Commit 5859e6014aa04700f1fe7aed5665f9aa5f0382e3
  + Fix dataset I/O bug when using "H5S_ALL".
    + Commit b5302d004d80473d6aa93ed5efb2cbae3f57e827
  + Fix calculation for determining whether the file space is interleaving.
    + Commit 8959e59798066ec166b51f3696b93efe594f0c1d
  + Fix a bug when memory space and file space have different number of dimensions.
  + Fix a reference count bug when VOL info is created from a string.
    + Commit 646a0c640d68a1bc15ddf24a16d9bfbc7e6263fa
* New test program
  + Added tests of parallel runs. Command "make ptest" will run test programs
    in parallel using 4 MPI processes.
  + A new test program 'tests/testcases/scalar.cpp'  to read/write test scalar
    datasets.
  + An external test program qmcpack restart 
    + https://github.com/QMCPACK/qmcpack.git
  + An external test program hdf5-iotest   
    + https://github.com/HDFGroup/hdf5-iotest
  + An external test program openPMD-api benhmark
    + https://github.com/openPMD/openPMD-api
* Updated utility program
  + Add command-line option "-d" to utility program "h5ldump" for dumping data
    of each selection block.
  + h5dump can now read a log-based VOL output file using log-based VOL.
  + Add utility script to set HDF5 environment to use log-based VOL.
    + h5lenv.bash, h5lenv.csh
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
