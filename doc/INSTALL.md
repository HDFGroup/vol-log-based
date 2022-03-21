## Log-based VOL - Build Instructions

### Software Requirements
* [HDF5 1.13.0](https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.13/hdf5-1.13.0/src/hdf5-1.13.0.tar.gz)
  + Configured with parallel I/O support (--enable-parallel)
* MPI C and C++ compilers
  + The plugin uses the constant initializer; a C++ compiler supporting std 17 is required
* Autotools utility
  + [autoconf](https://www.gnu.org/software/autoconf/) 2.69
  + [automake](https://www.gnu.org/software/automake/) 1.16.1
  + [libtool](https://www.gnu.org/software/libtool/) 2.4.6
  + [m4](https://www.gnu.org/software/m4/) 1.4.18

### Building HDF5 libraries
* HDF5 1.13.0 and later (**required**)
  + Download HDF5 official release version 1.13.0.
    ```
    % wget https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.13/hdf5-1.13.0/src/hdf5-1.13.0.tar.gz
    ```
  + Configure HDF5 with parallel I/O enabled.
    ```
    % tar -zxf hdf5-1_13_0.tar.gz
    % cd hdf5-1_13_0.tar.gz
    % ./configure --prefix=${HOME}/HDF5/1.13.0 --enable-parallel CC=mpicc
    % make -j4 install
    ```
  + The above example commands will install the HD5 library under the folder
    `${HOME}/HDF5/1.13.0`.

### Building log-based VOL
* Obtain the source code package by either downloading the official release or
  cloning the github repository.
  + Download the latest official release version 1.1.0.
    ```
    % wget https://github.com/DataLib-ECP/vol-log-based/archive/refs/tags/logvol.1.1.0.tar.gz
    % tar -zxf logvol.1.1.0.tar.gz
    % cd vol-log-based-logvol.1.1.0
    ```
  + Clone from the github repository.
    ```
    % git clone https://github.com/DataLib-ECP/vol-log-based.git
    % cd log_io_vol
    % autoreconf -i
    ```
* Example configure and make commands are given below.
  ```
  % ./configure --prefix=${HOME}/Log_IO_VOL --with-hdf5=${HOME}/HDF5/1.13.0
  % make -j 4 install
  ```
  + The above commands will install the log-vol library under the folder `${HOME}/Log_IO_VOL`.

* Check log-based VOL build
  + Command `make tests` compiles the test programs (build only, no run).
  + Command `make check` runs test programs on a single process.
  + Command `make ptest` runs test programs on 4 MPI processes in parallel.

* Configure command-line options
  + The full list of configure command-line options can be obtained by running
    the following command:
    ```
    % ./configure --help
    ```
  + Important configure options
    + **--enable-shared** Shared libraries are required if users intent to enable
      the log-based VOL through setting the two HDF5 VOL environment variables,
      `HDF5_VOL_CONNECTOR` and `HDF5_PLUGIN_PATH`. [default: enabled].
      See [Usage Guide](usage.md) for details.
    + **--enable-zlib**  to enable metadata compression using zlib. [default:
      disabled].
    + **--enable-test-qmcpack** to enable tests using
      [QMCPACK](https://github.com/QMCPACK/qmcpack.git) [default: disabled].
      This option first downloads and builds QMCPACK. Its test program
      `restart.c` will be used to test log-based VOL during `make check`.
    + **--enable-test-hdf5-iotest** to enable tests using
      [hdf5-iotest](https://github.com/HDFGroup/hdf5-iotest) [default:
      disabled]. This option first downloads and builds hdf5-iotest. Its test
      program `hdf5_iotest.c` will be used to test log-based VOL during
      `make check`.
    + **--enable-test-openpmd** to enable tests using
      [OpenPMD](https://github.com/openPMD/openPMD-api) [default: disabled].
      This option first downloads and builds OpenPMD. Its test program
      `8a_benchmark_write_parallel.c` and `8b_benchmark_read_parallel.c` will
      be used to test log-based VOL during `make check`.
    + **--enable-test-netcdf4[=INC,LIB | =DIR]** to enable tests using
      [NetCDF-C](https://github.com/Unidata/netcdf-c) [default: disabled]. This
      option can be used to provide the NetCDF installation path(s). It first
      downloads a few test programs from NetCDF-C, which will be to test
      log-based VOL during `make check`.

