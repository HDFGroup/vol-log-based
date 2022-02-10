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

### Build and install HDF5 library
* Build HDF5 with VOL and parallel I/O support
  + Download and extract HDF5 1.13.0 source package
  + Run command "./autogen.sh"
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
* Build and install this VOL plugin, `log-based vol`.
  + Clone this VOL plugin repository
  + Run command "autoreconf -i"
  + Configure log-based VOL
    + Shared library (--enable-shared) is required when using the log-based VOL
      through setting the HDF5 environment variables. See below for details.
    + Compile with zlib library (--enable-zlib) to enable metadata compression.
  + Example build commands are given below.
    ```
    % git clone https://github.com/DataLib-ECP/vol-log-based.git
    % cd log_io_vol
    % autoreconf -i
    % ./configure --prefix=${HOME}/Log_IO_VOL --with-hdf5=${HOME}/HDF5 --enable-shared --enable-zlib
    % make -j 4 install
    ```
    The VOL plugin library is now installed under the folder `${HOME}/Log_IO_VOL.`

### Compile user programs to use this VOL plugin
There are two ways to use the log-based VOL plugin. One is by modifying the
application source codes to add a few function calls. The other is by setting
HDF5 environment variables.

* Modify the user's application source codes, particularly if you would like to
  use the new APIs, such as `H5Dwrite_n()`, created by this VOL.
  * Include header file.
    + Add the following line to include the log-based VOL header file in your
      C/C++ source codes.
      ```
      #include <H5VL_log.h>
      ```
    + Header file `H5VL_log.h` is located in folder `${HOME}/Log_IO_VOL/include`
    + Add `-I${HOME}/Log_IO_VOL/include` to your compile command line. For example,
      ```
      % mpicc prog.c -o prog.o -I${HOME}/Log_IO_VOL/include
      ```
  * Library file.
    + The library file, `libH5VL_log.a`, is located under folder `${HOME}/Log_IO_VOL/lib`.
    + Add `-L${HOME}/Log_IO_VOL/lib -lH5VL_log` to your compile/link command line. For example,
      ```
      % mpicc prog.o -o prog -L${HOME}/Log_IO_VOL/lib -lH5VL_log \
                             -L${HOME}/HDF5/lib -lhdf5
      ```
  * Edit the source code to use log-based VOL when creating or opening an HDF5 file.
    + Register VOL callback structure through a call to `H5VLregister_connector()`
    + Callback structure is named `H5VL_log_g`
    + Set a file creation property list to use log-based vol
    + Below shows an example code fragment.
        ```
        fapl_id = H5Pcreate(H5P_FILE_ACCESS);
        H5Pset_fapl_mpio(fapl_id, comm, info);
        H5Pset_all_coll_metadata_ops(fapl_id, true);
        H5Pset_coll_metadata_write(fapl_id, true);
        log_vol_id = H5VLregister_connector(&H5VL_log_g, H5P_DEFAULT);
        H5Pset_vol(fapl_id, log_vol_id, NULL);
        ```
    + See a full example program in `examples/create_open.c`

* Use HDF5 environment variables, without modifying the user's application
  source codes.
  + The log-based VOL can be enabled at the run time through setting the
    environment variables below. No source code changes of existing user
    programs is required. (Note this is an HDF5 feature, applicable to all VOL
    plugins.)
  + Append log-based VOL library directory to shared object search path,
    `LD_LIBRARY_PATH`.
    ```
    % export LD_LIBRARY_PATH=${HOME}/Log_IO_VOL/lib
    ```
  + Set or add the log-based VOL library directory to HDF5 VOL search path,
    `HDF5_PLUGIN_PATH`.
    ```
    % export HDF5_PLUGIN_PATH=${HOME}/Log_IO_VOL/lib
    ```
  + Set the HDF5's default VOL, `HDF5_VOL_CONNECTOR`.
    ```
    % export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}"
    ```

