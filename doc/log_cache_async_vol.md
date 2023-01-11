# Running the Log VOL Connector on top of the Cache and Async VOL Connectors - Using E3SM-IO  as a Demonstration.
* [Build Instructions](#build-instructions)
* [Run Instructions](#run-e3sm-io)

This demo uses [E3SM-IO](https://github.com/Parallel-NetCDF/E3SM-IO) to show how to run the Log VOL connector on top of the [Cache VOL connector](https://github.com/hpc-io/vol-cache) and the [Async VOL connector](https://github.com/hpc-io/vol-async).

E3SM-IO is an I/O benchmark suite that measures the performance I/O kernel of
[E3SM](https://github.com/E3SM-Project/E3SM), a state-of-the-art Earth system modeling,
simulation, and prediction project. The I/O patterns of E3SM captured by the E3SM's I/O module,
[Scorpio](https://github.com/E3SM-Project/scorpio) from the production runs, are used in the benchmark.

The Cache VOL connector is an HDF5 plugin that incorporates fast storage layers (e.g, burst buffer, node-local storage) into parallel I/O workflow for caching and staging data to improve the I/O efficiency.

The Async VOL connector is another HDF5 plugin that takes advantage of an asynchronous interface by scheduling I/O as early as possible and overlaps computation or communication with I/O operations, which hides the cost associated with I/O and improves the overall performance.

The Log, Cache, and Async VOL connectors can be enabled by directly setting the environment variables without modifying test program's codes. This demo gives an instruction on how to install the Log, Cache and Async VOL connectors and the E3SM-IO benchmark, and gives an instruction of how to run.

## Build Instructions
### Prerequisite
+ Set up environment variables. These environment variables will be used during installing the libraries and running the benchmark.

    ```shell
    % export WORKSPACE=${HOME}  # all libraries will be installed to $WORKSPACE
    % export HDF5_DIR=${WORKSPACE}/HDF5
    % export ABT_DIR=${WORKSPACE}/Argobots
    % export ASYNC_DIR=${WORKSPACE}/Async
    % export CAHCE_DIR=${WORKSPACE}/Cache
    % export LOGVOL_DIR=${WORKSPACE}/Log

    % export HDF5_ROOT=${HDF5_DIR}
    ```

+ HDF5 1.13.3: `--enable-parallel`, `--enable-threadsafe`, and `--enable-unsupported` are [required by Async VOL](https://hdf5-vol-async.readthedocs.io/en/latest/gettingstarted.html#build-async-i-o-vol) at configure time.

    ```shell
    # create a new folder "HDF5" under $WORKSPACE
    % mkdir ${HDF5_DIR}
    % cd ${HDF5_DIR}
    
    # download HDF5 source codes
    % wget -cq https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.13/hdf5-1.13.3/src/hdf5-1.13.3.tar.gz
    % tar -zxf hdf5-1.13.3.tar.gz
    % cd hdf5-1.13.3
    
    # configure, output saved to log.config
    % ./configure --prefix=${HDF5_DIR} \
        --silent \
        --disable-fortran \
        --disable-tests \
        --enable-parallel \
        --enable-threadsafe \
        --enable-unsupported \
        --enable-build-mode=production \
        CC=cc \
        &> log.config

    # make, output saved to log.make
    % make -j 16 &> log.make

    # install, output saved to log.makeinstall
    % make install &> log.makeinstall
    ```

+ Argobots, [required by Async VOL](https://hdf5-vol-async.readthedocs.io/en/latest/gettingstarted.html#build-async-i-o-vol):

    ```shell
    # create a new folder "Argobots" under $WORKSPACE
    % mkdir ${ABT_DIR}
    % cd ${ABT_DIR}

    # download Argobots source codes
    % wget -qc https://github.com/pmodels/argobots/archive/refs/tags/v1.1.tar.gz
    % tar -xf v1.1.tar.gz
    % cd argobots-1.1

    # create configure file
    % ./autogen.sh &> log.autogen

    # configure, output saved to log.config
    % ./configure --prefix=${ABT_DIR} \
        --silent \
        CC=cc \
        CXX=CC \
        &> log.config
    
    # make, output saved to log.make
    % make -j 16 &> log.make
    
    # install, output saved to log.makeinstall
    % make install &> log.makeinstall
    ```

+ Async VOL connector

    ```shell
    # create a new folder "Async" under $WORKSPACE
    % mkdir ${ASYNC_DIR}
    % cd ${ASYNC_DIR}

    # download Async VOL source codes and create a build folder
    % wget -qc https://github.com/hpc-io/vol-async/archive/refs/tags/v1.4.tar.gz
    % tar -xf v1.4.tar.gz
    % cd vol-async-1.4
    % mkdir build
    % cd build

    # cmake, output saved to log.cmake
    % CC=cc CXX=CC \
        cmake .. -DCMAKE_INSTALL_PREFIX=${ASYNC_DIR} \
        &> log.cmake
    
    # make, output saved to log.make
    %  make -j 16 &> log.make

    # install, output saved to log.make
    % make install &> log.makeinstall
    ```

+ Cache VOL connector

    ```shell
    # create a new folder "Cache" under $WORKSPACE
    % mkdir ${CAHCE_DIR}
    % cd ${CAHCE_DIR}

    # set up env variable LD_LIBRARY_PATH
    % export LD_LIBRARY_PATH="$ABT_DIR/lib:$LD_LIBRARY_PATH"

    # download Cache VOL source codes and create a build folder
    % git clone https://github.com/hpc-io/vol-cache.git
    % cd vol-cache
    % mkdir build
    % cd build

    # cmake, output saved to log.cmake
    % CC=cc CXX=CC \
        HDF5_VOL_DIR=${ASYNC_DIR} \
        cmake .. -DCMAKE_INSTALL_PREFIX=${CAHCE_DIR} \
        &> log.cmake
    
    # make, output saved to log.make
    %  make -j 16 &> log.make

    # install, output saved to log.make
    % make install &> log.makeinstall
    ```
 + Log VOL connector.

    ```shell
    # create a new folder "Log" under $WORKSPACE
    % cd ${LOGVOL_DIR}

    # download Log VOL source codes
    % git clone git@github.com:DataLib-ECP/vol-log-based.git
    % cd vol-log-based

    # create configure file, output saved to log.autoreconf
    % autoreconf -i &> log.autoreconf
    
    # configure
    % ./configure \
        --with-hdf5=${HDF5_DIR} \
        --prefix=${LOGVOL_DIR}/install \
        CC=cc \
        CXX=CC \
        &> log.config
    
    # make, output saved to log.make
    %  make -j 16 &> log.make

    # install, output saved to log.make
    % make install &> log.makeinstall
    ```

+ E3SM-IO Benchmark
    ```shell
    # create a new folder "E3SM" under $WORKSPACE
    % mkdir ${WORKSPACE}/E3SM
    % cd ${WORKSPACE}/E3SM

    # download E3SM-IO source codes
    % git clone git@github.com:Parallel-NetCDF/E3SM-IO.git
    % cd E3SM-IO

    # create configure file, output saved to log.autoreconf
    % autoreconf -i &> log.autoreconf
    
    # configure
    % CFLAGS="-fno-var-tracking-assignments ${CFLAGS}" \
        CXXFLAGS="-fno-var-tracking-assignments ${CXXFLAGS}" \
        LD_LIBRARY_PATH="${HDF5_DIR}/lib:${LD_LIBRARY_PATH}" \
        ../E3SM-IO/configure \
        --with-hdf5=${HDF5_DIR} \
        --with-pnetcdf=${WORKSPACE}/PnetCDF/1.12.3 \
        --prefix=${WORKSPACE}/E3SM/install \
        --enable-threading \
        CC=cc \
        CXX=CC
    
    # make, output saved to log.make
    %  make -j 16 &> log.make

    # install, output saved to log.make
    % make install &> log.makeinstall
    ```

## Run E3SM-IO
In this section we provide instructions on how to run E3SM-IO F/G/I case using the corresponding input file `map_[f/g/i]_case_16p.h5` using 16 MPI processes.

We also provide the example performance outputs from Perlmutter @ NERSC using:
+ 1 Node
+ 16 MPI processes per node
+ Lustre File Settings: 1MiB Stripe Size, 16 OSTs.

### Using the Log VOL Connector Only

1. Environment Setting
   
    ```shell
    % export HDF5_PLUGIN_PATH="${LOGVOL_DIR}/install/lib"
    % export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}"
    % export LD_LIBRARY_PATH="${HDF5_DIR}/lib":"${HDF5_PLUGIN_PATH}":${LD_LIBRARY_PATH}
    ```
1. Run Commands
   1. If using Perlmutter @ NERSC, an example job script is provided below. It uses 16 MPI processes per node and 1 node.
        <details> <summary> Click here to see the example job script </summary>

        ```shell
        #!/bin/bash  -l

        #SBATCH -A m844
        #SBATCH -t 00:15:00
        #SBATCH --nodes=1
        #SBATCH -C cpu
        #SBATCH --qos=debug

        cd $PWD

        if test "x$SLURM_NTASKS_PER_NODE" = x ; then
        SLURM_NTASKS_PER_NODE=16
        fi

        # calculate the total number of MPI processes
        NP=$(($SLURM_JOB_NUM_NODES * $SLURM_NTASKS_PER_NODE))

        echo "------------------------------------------------------"
        echo "---- Running on Cori ----"
        echo "---- SLURM_JOB_QOS           = $SLURM_JOB_QOS"
        echo "---- SLURM_CLUSTER_NAME      = $SLURM_CLUSTER_NAME"
        echo "---- SLURM_JOB_PARTITION     = $SLURM_JOB_PARTITION"
        echo "---- SBATCH_CONSTRAINT       = $SBATCH_CONSTRAINT"
        echo "---- SLURM_JOB_NUM_NODES     = $SLURM_JOB_NUM_NODES"
        echo "---- SLURM_NTASKS_PER_NODE   = $SLURM_NTASKS_PER_NODE"
        echo "---- Number of MPI processes = $NP"
        echo "------------------------------------------------------"
        echo ""

        ulimit -c unlimited
        module unload darshan

        EXE_PATH=/global/u2/z/zanhua/VOLS/E3SM/install/bin
        DATA_PATH=/global/u2/z/zanhua/VOLS/E3SM/E3SM-IO/datasets
        E3SM_IO_EXE=/tmp/${USER}_e3smio_exe

        sbcast -v ${EXE_PATH}/e3sm_io ${E3SM_IO_EXE}

        EXE_OPTS="-c 16 --cpu_bind=cores ${E3SM_IO_EXE}"

        # ${OUT_DIR} points to the directory where you want to save the output files.
        OUT_DIR=/pscratch/sd/z/zanhua/FS_1M_16/e3sm_io

        # F case
        srun -n $NP $EXE_OPTS ${DATA_PATH}/map_f_case_16p.h5 -k -o ${OUT_DIR}/log_F_out.h5 -a hdf5 -x log -r 25 &> log.f.out

        # G case
        srun -n $NP $EXE_OPTS ${DATA_PATH}/map_g_case_16p.h5 -k -o ${OUT_DIR}/log_G_out.h5 -a hdf5 -x log -r 25 &> log.g.out

        # I case
        srun -n $NP $EXE_OPTS ${DATA_PATH}/map_i_case_16p.h5 -k -o ${OUT_DIR}/log_I_out.h5 -a hdf5 -x log -r 25 &> log.i.out
        ```

        </details>
    1. Commands for other platforms.
        ```shell
        # ${OUT_DIR} points to the directory where you want to save the output files.

        # F case
        mpiexec -np 16 ${WORKSPACE}/E3SM/install/bin/e3sm_io ${WORKSPACE}/E3SM/E3SM-IO/datasets/map_f_case_16p.h5 -k -o ${OUT_DIR}/log_F_out.h5 -a hdf5 -x log -r 25 &> log.f.out

        # G case
        mpiexec -np 16 ${WORKSPACE}/E3SM/install/bin/e3sm_io ${WORKSPACE}/E3SM/E3SM-IO/datasets/map_g_case_16p.h5 -k -o ${OUT_DIR}/log_G_out.h5 -a hdf5 -x log -r 25 &> log.g.out

        # I case
        mpiexec -np 16 ${WORKSPACE}/E3SM/install/bin/e3sm_io ${WORKSPACE}/E3SM/E3SM-IO/datasets/map_i_case_16p.h5 -k -o ${OUT_DIR}/log_I_out.h5 -a hdf5 -x log -r 25 &> log.i.out
        ```
2.  Example Performance Output
    1.  <details> <summary> Click here to see log.f.out (performance for F case): </summary>
        
        ```txt
        ==== Benchmarking F case =============================
        Total number of MPI processes      = 16
        Number of IO processes             = 16
        Input decomposition file           = /global/u2/z/zanhua/VOLS/E3SM/E3SM-IO/datasets/map_f_case_16p.h5
        Number of decompositions           = 3
        Output file/directory              = /pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/log_F_out.h5
        Using noncontiguous write buffer   = no
        Variable write order: same as variables are defined
        ==== HDF5 using log-based VOL through native APIs=====
        History output file                = /pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/log_F_out_h0.h5
        No. variables use no decomposition =     27
        No. variables use decomposition D0 =      1
        No. variables use decomposition D1 =    323
        No. variables use decomposition D2 =     63
        Total no. climate variables        =    414
        Total no. attributes               =   1421
        Total no. noncontiguous requests   = 4207968
        Max   no. noncontiguous requests   = 272135
        Min   no. noncontiguous requests   = 252670
        Write no. records (time dim)       =      1
        I/O flush frequency                =      1
        No. I/O flush calls                =      1
        -----------------------------------------------------------
        Total write amount                         = 16.97 MiB = 0.02 GiB
        Time of I/O preparing              min/max =   0.0006 /   0.0011
        Time of file open/create           min/max =   0.0289 /   0.0292
        Time of define variables           min/max =   0.0312 /   0.0312
        Time of posting write requests     min/max =   0.1431 /   0.1999
        Time of write flushing             min/max =   0.0000 /   0.0000
        Time of close                      min/max =   0.0376 /   0.0378
        end-to-end time                    min/max =   0.2990 /   0.2993
        Emulate computation time (sleep)   min/max =   0.0000 /   0.0000
        I/O bandwidth in MiB/sec (write-only)      = 99812006.9999
        I/O bandwidth in MiB/sec (open-to-close)   = 56.7017
        -----------------------------------------------------------
        ==== Benchmarking F case =============================
        Total number of MPI processes      = 16
        Number of IO processes             = 16
        Input decomposition file           = /global/u2/z/zanhua/VOLS/E3SM/E3SM-IO/datasets/map_f_case_16p.h5
        Number of decompositions           = 3
        Output file/directory              = /pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/log_F_out.h5
        Using noncontiguous write buffer   = no
        Variable write order: same as variables are defined
        ==== HDF5 using log-based VOL through native APIs=====
        History output file                = /pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/log_F_out_h1.h5
        No. variables use no decomposition =     27
        No. variables use decomposition D0 =      1
        No. variables use decomposition D1 =     22
        No. variables use decomposition D2 =      1
        Total no. climate variables        =     51
        Total no. attributes               =    142
        Total no. noncontiguous requests   = 1993966
        Max   no. noncontiguous requests   = 129303
        Min   no. noncontiguous requests   = 119706
        Write no. records (time dim)       =     25
        I/O flush frequency                =      1
        No. I/O flush calls                =     25
        -----------------------------------------------------------
        Total write amount                         = 7.96 MiB = 0.01 GiB
        Time of I/O preparing              min/max =   0.0000 /   0.0000
        Time of file open/create           min/max =   0.0118 /   0.0121
        Time of define variables           min/max =   0.0029 /   0.0029
        Time of posting write requests     min/max =   0.0698 /   0.0966
        Time of write flushing             min/max =   0.0000 /   0.0000
        Time of close                      min/max =   0.0154 /   0.0158
        end-to-end time                    min/max =   0.1272 /   0.1276
        Emulate computation time (sleep)   min/max =   0.0000 /   0.0000
        I/O bandwidth in MiB/sec (write-only)      = 3696198.8207
        I/O bandwidth in MiB/sec (open-to-close)   = 62.3723
        -----------------------------------------------------------
        read_decomp=0.05 e3sm_io_core=0.43 MPI init-to-finalize=0.49
        -----------------------------------------------------------
        ```
        </details>

    2.  <details> <summary> Click here to see log.g.out (performance for G case): </summary>

        ```txt
        ==== Benchmarking G case =============================
        Total number of MPI processes      = 16
        Number of IO processes             = 16
        Input decomposition file           = /global/u2/z/zanhua/VOLS/E3SM/E3SM-IO/datasets/map_g_case_16p.h5
        Number of decompositions           = 6
        Output file/directory              = /pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/log_G_out.h5
        Using noncontiguous write buffer   = no
        Variable write order: same as variables are defined
        ==== HDF5 using log-based VOL through native APIs=====
        History output file                = /pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/log_G_out.h5
        No. variables use no decomposition =     11
        No. variables use decomposition D0 =      6
        No. variables use decomposition D1 =      2
        No. variables use decomposition D2 =     25
        No. variables use decomposition D3 =      2
        No. variables use decomposition D4 =      2
        No. variables use decomposition D5 =      4
        Total no. climate variables        =     52
        Total no. attributes               =    851
        Total no. noncontiguous requests   = 252526
        Max   no. noncontiguous requests   =  18145
        Min   no. noncontiguous requests   =  13861
        Write no. records (time dim)       =     25
        I/O flush frequency                =      1
        No. I/O flush calls                =     25
        -----------------------------------------------------------
        Total write amount                         = 182.12 MiB = 0.18 GiB
        Time of I/O preparing              min/max =   0.0002 /   0.0003
        Time of file open/create           min/max =   0.0133 /   0.0136
        Time of define variables           min/max =   0.0191 /   0.0191
        Time of posting write requests     min/max =   0.0172 /   0.0203
        Time of write flushing             min/max =   0.0000 /   0.0000
        Time of close                      min/max =   0.2201 /   0.2204
        end-to-end time                    min/max =   0.2735 /   0.2738
        Emulate computation time (sleep)   min/max =   0.0000 /   0.0000
        I/O bandwidth in MiB/sec (write-only)      = 66323349.6261
        I/O bandwidth in MiB/sec (open-to-close)   = 665.2089
        -----------------------------------------------------------
        read_decomp=0.02 e3sm_io_core=0.27 MPI init-to-finalize=0.30
        -----------------------------------------------------------
        ```
        </details>
    3.  <details> <summary> Click here to see log.i.out (performance for I case): </summary>

        ```txt
        ==== Benchmarking I case =============================
        Total number of MPI processes      = 16
        Number of IO processes             = 16
        Input decomposition file           = /global/u2/z/zanhua/VOLS/E3SM/E3SM-IO/datasets/map_i_case_16p.h5
        Number of decompositions           = 5
        Output file/directory              = /pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/log_I_out.h5
        Using noncontiguous write buffer   = no
        Variable write order: same as variables are defined
        ==== HDF5 using log-based VOL through native APIs=====
        History output file                = /pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/log_I_out_h0.h5
        No. variables use no decomposition =     14
        No. variables use decomposition D0 =    465
        No. variables use decomposition D1 =     75
        No. variables use decomposition D2 =      4
        No. variables use decomposition D3 =      1
        No. variables use decomposition D4 =      1
        Total no. climate variables        =    560
        Total no. attributes               =   2768
        Total no. noncontiguous requests   = 37712870
        Max   no. noncontiguous requests   = 2743440
        Min   no. noncontiguous requests   = 2086560
        Write no. records (time dim)       =     25
        I/O flush frequency                =      1
        No. I/O flush calls                =     25
        -----------------------------------------------------------
        Total write amount                         = 835.94 MiB = 0.82 GiB
        Time of I/O preparing              min/max =   0.0019 /   0.0020
        Time of file open/create           min/max =   0.0139 /   0.0143
        Time of define variables           min/max =   0.0487 /   0.0487
        Time of posting write requests     min/max =   1.8931 /   2.6983
        Time of write flushing             min/max =   0.0000 /   0.0000
        Time of close                      min/max =   0.9931 /   0.9937
        end-to-end time                    min/max =   3.7565 /   3.7570
        Emulate computation time (sleep)   min/max =   0.0000 /   0.0000
        I/O bandwidth in MiB/sec (write-only)      = 209141973.7942
        I/O bandwidth in MiB/sec (open-to-close)   = 222.5018
        -----------------------------------------------------------
        ==== Benchmarking I case =============================
        Total number of MPI processes      = 16
        Number of IO processes             = 16
        Input decomposition file           = /global/u2/z/zanhua/VOLS/E3SM/E3SM-IO/datasets/map_i_case_16p.h5
        Number of decompositions           = 5
        Output file/directory              = /pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/log_I_out.h5
        Using noncontiguous write buffer   = no
        Variable write order: same as variables are defined
        ==== HDF5 using log-based VOL through native APIs=====
        History output file                = /pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/log_I_out_h1.h5
        No. variables use no decomposition =     14
        No. variables use decomposition D0 =    465
        No. variables use decomposition D1 =     69
        No. variables use decomposition D2 =      2
        No. variables use decomposition D3 =      1
        No. variables use decomposition D4 =      1
        Total no. climate variables        =    552
        Total no. attributes               =   2738
        Total no. noncontiguous requests   = 1508910
        Max   no. noncontiguous requests   = 109766
        Min   no. noncontiguous requests   =  83484
        Write no. records (time dim)       =      1
        I/O flush frequency                =      1
        No. I/O flush calls                =      1
        -----------------------------------------------------------
        Total write amount                         = 34.61 MiB = 0.03 GiB
        Time of I/O preparing              min/max =   0.0011 /   0.0011
        Time of file open/create           min/max =   0.0122 /   0.0124
        Time of define variables           min/max =   0.0398 /   0.0398
        Time of posting write requests     min/max =   0.0790 /   0.1104
        Time of write flushing             min/max =   0.0000 /   0.0000
        Time of close                      min/max =   0.0496 /   0.0498
        end-to-end time                    min/max =   0.2133 /   0.2135
        Emulate computation time (sleep)   min/max =   0.0000 /   0.0000
        I/O bandwidth in MiB/sec (write-only)      = 157319196.1005
        I/O bandwidth in MiB/sec (open-to-close)   = 162.0737
        -----------------------------------------------------------
        read_decomp=0.03 e3sm_io_core=3.97 MPI init-to-finalize=4.00
        -----------------------------------------------------------
        ```
        </details>
 
### Using the Log, Cache and Async VOL Connectors Together

1. Environment Setting
   
    ```shell
    % export H5VL_LOG_PASSTHRU=1
    % export HDF5_PLUGIN_PATH=${LOGVOL_DIR}/lib:${CACHE_DIR}/lib:${ASYNC_DIR}/lib
    % export LD_LIBRARY_PATH=${HDF5_PLUGIN_PATH}:${ABT_DIR}/lib:${HDF5_DIR}/lib:${LD_LIBRARY_PATH}
    % export HDF5_VOL_CONNECTOR="LOG under_vol=513;under_info={config=cache.cfg;under_vol=512;under_info={under_vol=0;under_info={}}}"
    ```
2. Put `cache.cfg` file under the folder where you run `srun`/`mpiexec` command. Note that this `cache.cfg` should not end with a new line.
    ```shell
    % cat cache.cfg
    HDF5_CACHE_STORAGE_SCOPE: LOCAL # the scope of the storage [LOCAL|GLOBAL]
    HDF5_CACHE_STORAGE_PATH: ./
    HDF5_CACHE_STORAGE_SIZE: 128188383838 # size of the storage space in bytes
    HDF5_CACHE_WRITE_BUFFER_SIZE: 2147483648 
    HDF5_CACHE_STORAGE_TYPE: MEMORY # local storage type [SSD|BURST_BUFFER|MEMORY|GPU], default SSD
    HDF5_CACHE_REPLACEMENT_POLICY: LRU # [LRU|LFU|FIFO|LIFO]
    ```
3. Run Commands
   1. If using Perlmutter @ NERSC, an example job script is provided below. It uses 16 MPI processes per node and 1 node.
        <details> <summary> Click here to see the example job script </summary>

        ```shell
        #!/bin/bash  -l

        #SBATCH -A m844
        #SBATCH -t 00:30:00
        #SBATCH --nodes=1
        #SBATCH -C cpu
        #SBATCH --qos=debug
        #SBATCH --mail-type=begin,end,fail
        #SBATCH --mail-user=zanhua@u.northwestern.edu

        cd $PWD

        if test "x$SLURM_NTASKS_PER_NODE" = x ; then
        SLURM_NTASKS_PER_NODE=16
        fi

        # calculate the total number of MPI processes
        NP=$(($SLURM_JOB_NUM_NODES * $SLURM_NTASKS_PER_NODE))

        echo "------------------------------------------------------"
        echo "---- Running on Cori ----"
        echo "---- SLURM_JOB_QOS           = $SLURM_JOB_QOS"
        echo "---- SLURM_CLUSTER_NAME      = $SLURM_CLUSTER_NAME"
        echo "---- SLURM_JOB_PARTITION     = $SLURM_JOB_PARTITION"
        echo "---- SBATCH_CONSTRAINT       = $SBATCH_CONSTRAINT"
        echo "---- SLURM_JOB_NUM_NODES     = $SLURM_JOB_NUM_NODES"
        echo "---- SLURM_NTASKS_PER_NODE   = $SLURM_NTASKS_PER_NODE"
        echo "---- Number of MPI processes = $NP"
        echo "------------------------------------------------------"
        echo ""

        ulimit -c unlimited
        module unload darshan

        EXE_PATH=/global/u2/z/zanhua/VOLS/E3SM/install/bin
        DATA_PATH=/global/u2/z/zanhua/VOLS/E3SM/E3SM-IO/datasets
        E3SM_IO_EXE=/tmp/${USER}_e3smio_exe

        sbcast -v ${EXE_PATH}/e3sm_io ${E3SM_IO_EXE}

        EXE_OPTS="-c 16 --cpu_bind=cores ${E3SM_IO_EXE}"

        # ${OUT_DIR} points to the directory where you want to save the output files.
        OUT_DIR=/pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/3vols


        # F case
        srun -n $NP $EXE_OPTS ${DATA_PATH}/map_f_case_16p.h5 -k -o ${OUT_DIR}/log_F_out.h5 -a hdf5 -x log -r 25 &> f.out

        # G case
        srun -n $NP $EXE_OPTS ${DATA_PATH}/map_g_case_16p.h5 -k -o ${OUT_DIR}/log_G_out.h5 -a hdf5 -x log -r 25 &> g.out

        # I case
        srun -n $NP $EXE_OPTS ${DATA_PATH}/map_i_case_16p.h5 -k -o ${OUT_DIR}/log_I_out.h5 -a hdf5 -x log -r 25 &> i.out
        ```

        </details>
   
   2. Commands for other platforms.
        ```shell
        # ${OUT_DIR} points to the directory where you want to save the output files.

        # F case
        mpiexec -np 16 ${WORKSPACE}/E3SM/install/bin/e3sm_io ${WORKSPACE}/E3SM/E3SM-IO/datasets/map_f_case_16p.h5 -k -o ${OUT_DIR}/log_F_out.h5 -a hdf5 -x log -r 25 &> f.out

        # G case
        mpiexec -np 16 ${WORKSPACE}/E3SM/install/bin/e3sm_io ${WORKSPACE}/E3SM/E3SM-IO/datasets/map_g_case_16p.h5 -k -o ${OUT_DIR}/log_G_out.h5 -a hdf5 -x log -r 25 &> g.out

        # I case
        mpiexec -np 16 ${WORKSPACE}/E3SM/install/bin/e3sm_io ${WORKSPACE}/E3SM/E3SM-IO/datasets/map_i_case_16p.h5 -k -o ${OUT_DIR}/log_I_out.h5 -a hdf5 -x log -r 25 &> i.out
        ```

4.  Example Performance Output
    1.  <details> <summary> Click here to see f.out (performance for F case): </summary>
        
        ```txt
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        ------- EXT CACHE VOL DATASET Write
        ------- EXT CACHE VOL DATASET Write
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        ------- EXT CACHE VOL DATASET Write
        ------- EXT CACHE VOL DATASET Write
        ==== Benchmarking F case =============================
        Total number of MPI processes      = 16
        Number of IO processes             = 16
        Input decomposition file           = /global/u2/z/zanhua/VOLS/E3SM/E3SM-IO/datasets/map_f_case_16p.h5
        Number of decompositions           = 3
        Output file/directory              = /pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/3vols/log_F_out.h5
        Using noncontiguous write buffer   = no
        Variable write order: same as variables are defined
        ==== HDF5 using log-based VOL through native APIs=====
        History output file                = /pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/3vols/log_F_out_h0.h5
        No. variables use no decomposition =     27
        No. variables use decomposition D0 =      1
        No. variables use decomposition D1 =    323
        No. variables use decomposition D2 =     63
        Total no. climate variables        =    414
        Total no. attributes               =   1421
        Total no. noncontiguous requests   = 4207968
        Max   no. noncontiguous requests   = 272135
        Min   no. noncontiguous requests   = 252670
        Write no. records (time dim)       =      1
        I/O flush frequency                =      1
        No. I/O flush calls                =      1
        -----------------------------------------------------------
        Total write amount                         = 16.97 MiB = 0.02 GiB
        Time of I/O preparing              min/max =   0.0010 /   0.0011
        Time of file open/create           min/max =   0.0109 /   0.0111
        Time of define variables           min/max =   1.8869 /   1.8869
        Time of posting write requests     min/max =   0.7391 /   0.8164
        Time of write flushing             min/max =   0.0000 /   0.0000
        Time of close                      min/max =   0.2177 /   0.2185
        end-to-end time                    min/max =   2.9332 /   2.9340
        Emulate computation time (sleep)   min/max =   0.0000 /   0.0000
        I/O bandwidth in MiB/sec (write-only)      = 4704197.5278
        I/O bandwidth in MiB/sec (open-to-close)   = 5.7833
        -----------------------------------------------------------
        ==== Benchmarking F case =============================
        Total number of MPI processes      = 16
        Number of IO processes             = 16
        Input decomposition file           = /global/u2/z/zanhua/VOLS/E3SM/E3SM-IO/datasets/map_f_case_16p.h5
        Number of decompositions           = 3
        Output file/directory              = /pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/3vols/log_F_out.h5
        Using noncontiguous write buffer   = no
        Variable write order: same as variables are defined
        ==== HDF5 using log-based VOL through native APIs=====
        History output file                = /pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/3vols/log_F_out_h1.h5
        No. variables use no decomposition =     27
        No. variables use decomposition D0 =      1
        No. variables use decomposition D1 =     22
        No. variables use decomposition D2 =      1
        Total no. climate variables        =     51
        Total no. attributes               =    142
        Total no. noncontiguous requests   = 1993966
        Max   no. noncontiguous requests   = 129303
        Min   no. noncontiguous requests   = 119706
        Write no. records (time dim)       =     25
        I/O flush frequency                =      1
        No. I/O flush calls                =     25
        -----------------------------------------------------------
        Total write amount                         = 7.96 MiB = 0.01 GiB
        Time of I/O preparing              min/max =   0.0000 /   0.0000
        Time of file open/create           min/max =   0.0130 /   0.0131
        Time of define variables           min/max =   0.0383 /   0.0384
        Time of posting write requests     min/max =   0.4424 /   0.5422
        Time of write flushing             min/max =   0.0000 /   0.0000
        Time of close                      min/max =   0.0309 /   0.0313
        end-to-end time                    min/max =   0.6267 /   0.6271
        Emulate computation time (sleep)   min/max =   0.0000 /   0.0000
        I/O bandwidth in MiB/sec (write-only)      = 1508609.7563
        I/O bandwidth in MiB/sec (open-to-close)   = 12.6897
        -----------------------------------------------------------
        read_decomp=0.10 e3sm_io_core=3.56 MPI init-to-finalize=3.66
        -----------------------------------------------------------
        ```

        </details>
    1.  <details> <summary> Click here to see g.out (performance for G case): </summary>
        
        ```txt
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        ------- EXT CACHE VOL DATASET Write
        ------- EXT CACHE VOL DATASET Write
        ==== Benchmarking G case =============================
        Total number of MPI processes      = 16
        Number of IO processes             = 16
        Input decomposition file           = /global/u2/z/zanhua/VOLS/E3SM/E3SM-IO/datasets/map_g_case_16p.h5
        Number of decompositions           = 6
        Output file/directory              = /pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/3vols/log_G_out.h5
        Using noncontiguous write buffer   = no
        Variable write order: same as variables are defined
        ==== HDF5 using log-based VOL through native APIs=====
        History output file                = /pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/3vols/log_G_out.h5
        No. variables use no decomposition =     11
        No. variables use decomposition D0 =      6
        No. variables use decomposition D1 =      2
        No. variables use decomposition D2 =     25
        No. variables use decomposition D3 =      2
        No. variables use decomposition D4 =      2
        No. variables use decomposition D5 =      4
        Total no. climate variables        =     52
        Total no. attributes               =    851
        Total no. noncontiguous requests   = 252526
        Max   no. noncontiguous requests   =  18145
        Min   no. noncontiguous requests   =  13861
        Write no. records (time dim)       =     25
        I/O flush frequency                =      1
        No. I/O flush calls                =     25
        -----------------------------------------------------------
        Total write amount                         = 182.12 MiB = 0.18 GiB
        Time of I/O preparing              min/max =   0.0002 /   0.0003
        Time of file open/create           min/max =   0.0128 /   0.0130
        Time of define variables           min/max =   0.3356 /   0.3357
        Time of posting write requests     min/max =   0.7691 /   0.8372
        Time of write flushing             min/max =   0.0000 /   0.0000
        Time of close                      min/max =   0.2187 /   0.2190
        end-to-end time                    min/max =   1.4158 /   1.4161
        Emulate computation time (sleep)   min/max =   0.0000 /   0.0000
        I/O bandwidth in MiB/sec (write-only)      = 41401209.7864
        I/O bandwidth in MiB/sec (open-to-close)   = 128.6077
        -----------------------------------------------------------
        read_decomp=0.05 e3sm_io_core=1.42 MPI init-to-finalize=1.47
        -----------------------------------------------------------
        ```

        </details>
    1.  <details> <summary> Click here to see i.out (performance for I case): </summary>
        
        ```txt
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        ------- EXT CACHE VOL DATASET Write
        ------- EXT CACHE VOL DATASET Write
        Warning: An error occured in e3sm_io_driver_hdf5::inq_file_info in e3sm_io_driver_hdf5.cpp. Use MPI_INFO_NULL.
        ------- EXT CACHE VOL DATASET Write
        ------- EXT CACHE VOL DATASET Write
        ==== Benchmarking I case =============================
        Total number of MPI processes      = 16
        Number of IO processes             = 16
        Input decomposition file           = /global/u2/z/zanhua/VOLS/E3SM/E3SM-IO/datasets/map_i_case_16p.h5
        Number of decompositions           = 5
        Output file/directory              = /pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/3vols/log_I_out.h5
        Using noncontiguous write buffer   = no
        Variable write order: same as variables are defined
        ==== HDF5 using log-based VOL through native APIs=====
        History output file                = /pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/3vols/log_I_out_h0.h5
        No. variables use no decomposition =     14
        No. variables use decomposition D0 =    465
        No. variables use decomposition D1 =     75
        No. variables use decomposition D2 =      4
        No. variables use decomposition D3 =      1
        No. variables use decomposition D4 =      1
        Total no. climate variables        =    560
        Total no. attributes               =   2768
        Total no. noncontiguous requests   = 37712870
        Max   no. noncontiguous requests   = 2743440
        Min   no. noncontiguous requests   = 2086560
        Write no. records (time dim)       =     25
        I/O flush frequency                =      1
        No. I/O flush calls                =     25
        -----------------------------------------------------------
        Total write amount                         = 835.94 MiB = 0.82 GiB
        Time of I/O preparing              min/max =   0.0019 /   0.0021
        Time of file open/create           min/max =   0.0134 /   0.0135
        Time of define variables           min/max =   4.9734 /   4.9734
        Time of posting write requests     min/max =  85.8112 / 101.2761
        Time of write flushing             min/max =   0.0000 /   0.0001
        Time of close                      min/max =   3.3241 /   3.3314
        end-to-end time                    min/max = 110.4072 / 110.4146
        Emulate computation time (sleep)   min/max =   0.0000 /   0.0000
        I/O bandwidth in MiB/sec (write-only)      = 12885402.1862
        I/O bandwidth in MiB/sec (open-to-close)   = 7.5709
        -----------------------------------------------------------
        ==== Benchmarking I case =============================
        Total number of MPI processes      = 16
        Number of IO processes             = 16
        Input decomposition file           = /global/u2/z/zanhua/VOLS/E3SM/E3SM-IO/datasets/map_i_case_16p.h5
        Number of decompositions           = 5
        Output file/directory              = /pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/3vols/log_I_out.h5
        Using noncontiguous write buffer   = no
        Variable write order: same as variables are defined
        ==== HDF5 using log-based VOL through native APIs=====
        History output file                = /pscratch/sd/z/zanhua/FS_1M_16/e3sm_io/3vols/log_I_out_h1.h5
        No. variables use no decomposition =     14
        No. variables use decomposition D0 =    465
        No. variables use decomposition D1 =     69
        No. variables use decomposition D2 =      2
        No. variables use decomposition D3 =      1
        No. variables use decomposition D4 =      1
        Total no. climate variables        =    552
        Total no. attributes               =   2738
        Total no. noncontiguous requests   = 1508910
        Max   no. noncontiguous requests   = 109766
        Min   no. noncontiguous requests   =  83484
        Write no. records (time dim)       =      1
        I/O flush frequency                =      1
        No. I/O flush calls                =      1
        -----------------------------------------------------------
        Total write amount                         = 34.61 MiB = 0.03 GiB
        Time of I/O preparing              min/max =   0.0011 /   0.0011
        Time of file open/create           min/max =   0.0235 /   0.0238
        Time of define variables           min/max =   6.8212 /   6.8212
        Time of posting write requests     min/max =   1.8862 /   2.0785
        Time of write flushing             min/max =   0.0000 /   0.0000
        Time of close                      min/max =   0.6206 /   0.6213
        end-to-end time                    min/max =   9.5453 /   9.5460
        Emulate computation time (sleep)   min/max =   0.0000 /   0.0000
        I/O bandwidth in MiB/sec (write-only)      = 34199824.4709
        I/O bandwidth in MiB/sec (open-to-close)   = 3.6256
        -----------------------------------------------------------
        read_decomp=0.07 e3sm_io_core=119.96 MPI init-to-finalize=120.03
        -----------------------------------------------------------
        ```

        </details>