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

### Building dependencies
* Build HDF5 with VOL and parallel I/O support
  + Download and extract HDF5 1.13.0 source package
  + Configure HDF5 with parallel I/O enabled
  + Run "make install"
  + Example commands are given below. This example will install
    the HD5 library under the folder `${HOME}/HDF5`.
    ```
    % wget https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.13/hdf5-1.13.0/src/hdf5-1.13.0.tar.gz
    % tar -zxf hdf5-1_13_0.tar.gz
    % cd hdf5-1_13_0.tar.gz
    % ./configure --prefix=${HOME}/HDF5 --enable-parallel CC=mpicc
    % make -j4 install
    ```
* Build NetCDF with parallel NetCDF4 support (optional)
  + Download and extract NetCDF source package
  + Configure NetCDF with parallel NetCDF4 enabled
  + Run "make install"
  + Example commands are given below. This example will install
    the NetCDF library under the folder `${HOME}/NetCDF`.
    ```
    % wget https://downloads.unidata.ucar.edu/netcdf-c/4.8.1/netcdf-c-4.8.1.tar.gz
    % tar -zxf netcdf-c-4.8.1.tar.gz
    % cd netcdf-c-4.8.1.tar.gz
    % ./configure --prefix=${HOME}/NetCDF --disable-dap -I${HOME}/HDF5/include--enable-netcdf4 LDFLAGS=-L${HOME}/HDF5/lib LIBS=-lhdf5 CC=mpicc CXX=mpicxx
    % make -j4 install
    ```
### Building log-based VOL
* Build and install this VOL plugin, `log-based vol`.
  + Clone this VOL plugin repository
  + Run command "autoreconf -i"
  + Configure log-based VOL
    + Shared library (--enable-shared) is required when using the log-based VOL
      through setting the HDF5 environment variables. See below for details.
    + Compile with zlib library (--enable-zlib) to enable metadata compression.
    + Compile with NetCDF4 library (--with-netcdf4) to enable NetCDF4 tests.
  + Example build commands are given below.
    ```
    % git clone https://github.com/DataLib-ECP/vol-log-based.git
    % cd log_io_vol
    % autoreconf -i
    % ./configure --prefix=${HOME}/Log_IO_VOL --with-hdf5=${HOME}/HDF5 --with-netcdf4=${HOME}/NetCDF --enable-shared --enable-zlib
    % make -j 4 install
    ```
    The VOL plugin library is now installed under the folder `${HOME}/Log_IO_VOL.`
* Verify log-based VOL build
  + Run command "make tests" to compile the test programs
  + Run command "make check" to run test programs on a single process
  + Run command "make ptests" to run test programs in parallel
  + Example commands are given below.
    ```
    % make tests
    % make check
    % make ptests
    ```
