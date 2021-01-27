## Log-based VOL - an HDF5 VOL Plugin that stores HDF5 datasets in a log-based storage layout

This software repository contains source codes implementing an [HDF5](https://www.hdfgroup.org) Virtual Object Layer ([VOL](https://bitbucket.hdfgroup.org/projects/HDFFV/repos/hdf5doc/browse/RFCs/HDF5/VOL/developer_guide/main.pdf))) plugin that stores HDF5 datasets in a log-based storage layout. It allows applications to generate efficient log-based I/O requests using HDF5 APIs.

### Software Requirements
* [HDF5 1.12.0](https://github.com/HDFGroup/hdf5/tree/1.12/master)
  + Parallel I/O support (--enable-parallel) is required
* MPI C and C++ compilers
  + The plugin uses the constant initializer; a C++ compiler supporting std 11 is required
* Autotools utility
  + autoconf 2.69
  + automake 1.16.1
  + libtoolize 2.4.6
  + m4 1.4.18

### Building Steps
* Build HDF5 with VOL and parallel I/O support
  + Clone the develop branch from the HDF5 repository
  + Run command ./autogen.sh
  + Configure HDF5 with parallel I/O enabled
  + Run make install
  + Example commands are given below. This example will install
    the HD5 library under the folder `${HOME}/HDF5`.
    ```
    % git clone https://github.com/HDFGroup/hdf5.git
    % cd hdf5
    % git checkout hdf5-1_12_0
    % ./autogen
    % ./configure --prefix=${HOME}/HDF5 --enable-parallel CC=mpicc
    % make -j4 install
    ```
* Build this VOL plugin, `log-based vol.`
  + Clone this VOL plugin repository
  + Run command autoreconf -i
  + Configure log-based VOL 
    + Shared library is required to enable log-based VOL by environment variables
    + Compile with zlib library to enable metadata compression
  + Example commands are given below.
    ```
    % git clone https://github.com/DataLib-ECP/log_io_vol.git
    % cd log_io_vol
    % autoreconf -i
    % ./configure --prefix=${HOME}/Log_IO_VOL --with-hdf5=${HOME}/HDF5 --enable-shared --enable-zlib
    % make -j 4 install
    ```
    The VOL plugin library is now installed under the folder `${HOME}/Log_IO_VOL.`

### Compile user programs that use this VOL plugin
* Enable log-based VOL programmatically
  * Include header file.
    + Add the following line to your C/C++ source codes.
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
  * Edit the source code to use log-based VOL when opening HDF5 files
    + Register VOL callback structure using `H5VLregister_connector`
    + Callback structure is named `H5VL_log_g`
    + Set a file creation property list to use log-based vol
    + For example,
        ```
        fapl_id = H5Pcreate(H5P_FILE_ACCESS); 
        H5Pset_fapl_mpio(fapl_id, comm, info);
        H5Pset_all_coll_metadata_ops(fapl_id, true);
        H5Pset_coll_metadata_write(fapl_id, true);
        log_vol_id = H5VLregister_connector(&H5VL_log_g, H5P_DEFAULT);
        H5Pset_vol(fapl_id, log_vol_id, NULL);
        ```
    + See a full example program in `examples/create_open.c`
* Enable log-based VOL dynamically through environment variables
  + No additional action required on programming and compiling
  + Tell HDF5 to use the log-based VOL through environment variables
    + Append log-based VOL lib directory to shared object search path (LD_LIBRARY_PATH)
      ```
      % export LD_LIBRARY_PATH=${HOME}/Log_IO_VOL/lib
      ```
    + Append log-based VOL lib directory to HDF5 VOL search path (HDF5_PLUGIN_PATH)
      ```
      % export HDF5_PLUGIN_PATH=${HOME}/Log_IO_VOL/lib
      ```
    + Set log-based VOL as HDF5's default VOL (HDF5_VOL_CONNECTOR)
      ```
      % export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}"
      ```

### Current limitations
  + Not compatible with parallel NetCDF4 applications
  + Does not support dataset reading.
    + Feature to read datasets in log-based storage layout is under development
  + Utility to repack dataset in log-based storage layout into conventional storage layout is under development 

### References
* [HDF5 VOL application developer manual](https://bitbucket.hdfgroup.org/projects/HDFFV/repos/hdf5doc/browse/RFCs/HDF5/VOL/developer_guide/main.pdf)
* [HDF5 VOL plug-in developer manual](https://bitbucket.hdfgroup.org/projects/HDFFV/repos/hdf5doc/browse/RFCs/HDF5/VOL/user_guide)
* [HDF5 VOL RFC](https://bitbucket.hdfgroup.org/projects/HDFFV/repos/hdf5doc/browse/RFCs/HDF5/VOL/RFC)

### Project funding supports:
Ongoing development and maintenance of Log-based VOL are supported by the Exascale Computing Project (17-SC-20-SC), a joint project of the U.S. Department of Energy's Office of Science and National Nuclear Security Administration, responsible for delivering a capable exascale ecosystem, including software, applications, and hardware technology, to support the nation's exascale computing imperative.