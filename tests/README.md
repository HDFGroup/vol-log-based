## Log-based VOL test suit 

The test suit contains the internal test programs and external test programs.
Internal test programs are maintained as part of the log-based VOL distribution.
External test programs are a collection of benchmarks and applications that uses log-based VOL.

### Building Steps
* Build log-based VOL plugin.
  + See [README.md](../README.md) under the root folder
* (Optional) Enable external test programs 
  + Update submodules
    + External test programs are incorporated into log-based VOL as submodules.
    + Run git submodule init and git submodule update to check out the external test programs.
      ```
      % git submodule init
      % git submodule update
      ```
  + Enable extern test programs in the build configuration
    + Add option --enable-external-tests option when configuring log-based VOL build
      ```
      % ./configure --prefix=${HOME}/Log_IO_VOL --with-hdf5=${HOME}/HDF5 --enable-shared --enable-zlib --enable-external-tests
      ```
* Compile test programs.
  + Run make check under the tests folder to build and run all test programs.
    ```
    % cd tests
    % make check
    ```
  + Run make tests under the tests folder to build all test programs without running

### Internal test programs
  * The folder [basic](basic) contains simple test programs testing basic HDF5 functions.
    + file
      + Test H5Fcreate, H5Fopen, and H5Fclose APIs.
    + group
      + Test H5Gcreate, H5Gopen, and H5Gclose APIs.
    + attr
      + Test H5Acreate, H5Aopen, H5Awrite, and H5Aclose APIs.
      + Test creating an attribute under both the file and a group. 
    + dset 
      + Test H5Dcreate, H5Dopen, and H5Dclose APIs.
      + Test creating a dataset under both the file and a group. 
    + dwrite
      + Test writing to a dataset with dataspace selection.
    + dread
      + Test reading back the data written to a dataset with dataspace selection.
      + Verify the data read matches the data written.
    + memsel
      + Test H5Dwrite and H5Dread APIs when the memory space selection is not contiguous.
  * The folder [dynamic](dynamic) contains a single test program to test whether log-based VOL
    can be enabled through environment variables on existing applications.
  * The folder [testcases](testcases) contains test programs representing previously identified and fixed bugs.
    + null_req 
      + Test the H5Dwrite APIs when the dataset space selection is empty.
    + scalar
      + Test creating and writing to a scalar dataset.

### External test programs
  * hdf5_io-test
    + https://github.com/HDFGroup/hdf5-iotest
    + A simple HDF5 I/O performance tester developed by the HDF5 team.
  * HDFGroup vol-tests
    + https://github.com/HDFGroup/vol-tests
    + A test suit from HDFGroup to test VOLs' support to each HDF5 API.
    + A VOL is not expected to pass all tests.
    + The log-based VOL currently passes around 87% of the test.
  * qmcpack restart
    + https://github.com/QMCPACK/qmcpack.git
    + An open-source Quantum Monte Carlo code for computing the electronic structure of atoms, molecules, 2D nanomaterials, and solids.
    + The restart test program in qmcpack is used to test log-based VOL.
  * openPMD write benchmark
    + https://github.com/openPMD/openPMD-api
    + An open meta-data schema
    + The 8a_benchmark_write_parallel program in openPMD is used to test log-based VOL.
  * E3SM I/O benchmark
    + https://github.com/Parallel-NetCDF/E3SM-IO
    + Mimic parallel I/O kernel from the E3SM climate simulation model.
    + Use the I/O pattern recorded by the PIO library.
  
  
