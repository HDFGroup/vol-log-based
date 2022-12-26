# Running the Log VOL Connector on top of the Cache and Async VOL Connectors
* [Build Instructions](#build-instructions)
* [Run Instructions](#run-instructions)
* [Example Output](#example-output)

This demo shows how to run the Log VOL connector on top of the [Cache VOL connector](https://github.com/hpc-io/vol-cache) and the [Async VOL copnnector](https://github.com/hpc-io/vol-async). The test program we will use is the [dwrite.cpp](../tests/passthru/dwrite.cpp) under [src/tests/passthru](../tests/passthru) folder.

The Cache VOL connector is an HDF5 plugin that incorporates fast storage layers (e.g, burst buffer, node-local storage) into parallel I/O workflow for caching and staging data to improve the I/O efficiency.

The Async VOL connector is another HDF5 plugin that takes advantage of an asynchronous interface by scheduling I/O as early as possible and overlaps computation or communication with I/O operations, which hides the cost associated with I/O and improves the overall performance.

The Log, Cache, and Async VOL connectors can be enabled by directly setting the environment variables without modifying test program's codes. This demo gives an instruction on how to install Cache VOL and Async VOL, and gives an demo of how to run the test program with them.

## Build Instructions
### Prerequisite
+ Set up environment:

    ```shell
    export HDF5_DIR=#the dir you want to install HDF5 to
    export ABT_DIR=#the dir you want to install argobots to
    export ASYNC_DIR=#the dir you want to install Async VOL to
    export CACHE_DIR=#the dir you want to install Cache VOL to
 
    export HDF5_ROOT=$HDF5_DIR
    ```

+ HDF5 1.13.3: `--enable-parallel`, `--enable-threadsafe`, and `--enable-unsupported` are [required by Async VOL](https://hdf5-vol-async.readthedocs.io/en/latest/gettingstarted.html#build-async-i-o-vol) at configure time.

    ```shell
    # the following env variable will be used:
    # HDF5_DIR

    % export $HDF5_DIR=#path to the directory to install HDF5
    % wget https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.13/hdf5-1.13.3/src/hdf5-1.13.3.tar.gz
    % tar -xf hdf5-1.13.3.tar.gz
    % cd hdf5-1.13.3
    
    % ./configure --prefix=$HDF5_DIR --enable-parallel --enable-threadsafe --enable-unsupported CC=mpicc CXX=mpicxx
    % make
    % make install
    ```

+ Argobots, [required by Async VOL](https://hdf5-vol-async.readthedocs.io/en/latest/gettingstarted.html#build-async-i-o-vol):

    ```shell
    # the following env variable will be used:
    # ABT_DIR

    % wget -qc https://github.com/pmodels/argobots/archive/refs/tags/v1.1.tar.gz
    % tar -xf v1.1.tar.gz
    % cd argobots-1.1
    % ./autogen.sh

    % ./configure --prefix=$ABT_DIR CC=mpicc CXX=mpicxx
    % make
    % make install
    ```

+ Async VOL connector

    ```shell
    # the following env variables will be used:
    # HDF5_DIR, ABT_DIR, ASYNC_DIR, HDF5_ROOT

    % wget https://github.com/hpc-io/vol-async/archive/refs/tags/v1.4.tar.gz
    % tar -xf v1.4.tar.gz
    % cd vol-async-1.4
    % mkdir build
    % cd build
    % CC=mpicc CXX=mpicxx cmake .. -DCMAKE_INSTALL_PREFIX=$ASYNC_DIR
    % make
    % make install
    ```

+ Cache VOL connector

    ```shell
    # the following env variables will be used:
    # ABT_DIR, ASYNC_DIR, CAHCE_DIR

    % git clone https://github.com/hpc-io/vol-cache.git
    % cd vol-cache
    % mkdir build
    % cd build
    % export LD_LIBRARY_PATH="$ABT_DIR/lib:$LD_LIBRARY_PATH"
    % CC=mpicc CXX=mpicxx HDF5_VOL_DIR=$ASYNC_DIR cmake .. -DCMAKE_INSTALL_PREFIX=$CAHCE_DIR
    % make
    % make install
    ```
 

### Compile the Test Program
We will use the [dwrite.cpp](../tests/passthru/dwrite.cpp) under [src/tests/passthru](../tests/passthru) folder to demonstrate how to compile a program. To compile the program, only the HDF5 library is necessary. The Log, Cache, and Async VOL connectors will only be used at runtime. 

Note that it is [required by Async VOL](https://hdf5-vol-async.readthedocs.io/en/latest/gettingstarted.html#explicit-mode) that a program should use `MPI_Init_thread` instead of `MPI_Init`.

```shell
export HDF5_DIR=#path to hdf5 install dir

mpicxx -o dwrite dwrite.cpp -g -O2 \
    -I${HDF5_DIR}/include \
    -L${HDF5_DIR}/lib -lhdf5 \
```

## Run the Test Program
1. Set up environment:

    ```shell
    # the followings are already set during installation.
    export HDF5_DIR=#path to hdf5 install dir
    export ABT_DIR=#path to argobots install dir
    export ASYNC_DIR=#path to Async VOL install dir
    export CACHE_DIR=#path to Cache VOL install dir
    export LOGVOL_DIR=#path to the Log VOL install dir
    export HDF5_ROOT=$HDF5_DIR

    # the followings are newly added env variables.
    export HDF5_PLUGIN_PATH=${LOGVOL_DIR}/lib:${CACHE_DIR}/lib:${ASYNC_DIR}/lib
    export LD_LIBRARY_PATH=${LOGVOL_DIR}/lib:${CACHE_DIR}/lib:${ASYNC_DIR}/lib:${ABT_DIR}/lib:${HDF5_DIR}/lib:${LD_LIBRARY_PATH}
    export HDF5_VOL_CONNECTOR="LOG under_vol=513;under_info={config=cache_1.cfg;under_vol=512;under_info={under_vol=0;under_info={}}}"
    export H5VL_LOG_PASSTHRU_READ_WRITE=1
    ```


1. Run commands
    ```shell
    % mpiexec -n 16 ./dwrite
    ```