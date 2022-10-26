# WRF case study

* [Build Instructions](#build-instructions)
* [Run Instructions](#run-instructions)
* [Example Results](#example-results)

This case study uses [the Weather Research and Forecasting (WRF) Model](https://github.com/wrf-model/WRF) to demonstrate the usage of HDF5 log-layout based VOL. WRF can choose to use NetCDF4 I/O option which can perform underlying I/O using parallel HDF5. Enabling  log-layout based VOL for this case is easy since all we need to do is to set up several environment variables.

## Build Instructions
* Prerequisite
  + HDF5 1.13.0 or higher, required by any HDF5 VOL
  + HDF5 log-layout based VOL version 1.3.0
  + netcdf 4.1.3 or higher
  + Jasper-1.900.1 or higher
  + libpng-1.2.50 or higher
  + zlib-1.2.7 or higher
* Clone WRF from its github repository:
  ```
    git clone https://github.com/wrf-model/WRF/releases/download/v4.4.1/v4.4.1.tar.gz
  ```  
* Setup environment variables for configuring WRF
  ```
    export DIR=${HOME}
    export JASPERLIB=${HOME}/grib2/lib
    export JASPERINC=${HOME}/grib2/include
    export CC=mpicc
    export CXX=mpicxx
    export FC=mpif90
    export FCFLAGS=-m64
    export F77=mpif77
    export F90=mpif90
    export FFLAGS=-m64

    export NETCDF=${HOME}/NetCDF/4.9.0
    export HDF5=${HOME}/HDF5/1.13.2
    export ZLIB=${HOME}/zlib/1.2.7
    export WRF_DIR=${HOME}/WRF

    export NETCDFPAR=$NETCDF_DIR
    export PHDF5=$HDF5

    export CPPFLAGS="$CPPFLAGS -I${JASPERINC} -I$HDF5/include -I$NETCDF/include"
    export LDFLAGS="$LDFLAGS -L${JASPERLIB} -L$HDF5/lib -L$NETCDF/lib"
    export LD_LIBRARY_PATH="$HDF5/lib:$NETCDF/lib:$LD_LIBRARY_PATH"
    export PATH="$HDF5_DIR/bin:$NETCDF_DIR/bin:$PATH"
  ```
* Configure WRF with NetCDF4 features enabled.
  ```
    cd WRF
    ./clean -a
    ./configure
  ```
  + The WRF configuration is interactive.
    + Select the distributed memory only (dmpar) option that corresponds to the target system.
    + Other options can be left default.
* Compile one of WRF applications  
    + An example compiling the em_les test case
      ```
          ./compile em_les
      ```
    Two executables (ideal.exe, and wrf.exe) will be built under `main` forlder. The two 
    executables should also exist in `test/em_les`, which are linked to those under `main` folder.

* For more details, please refer to [WRF's official compilation tutorial](https://www2.mmm.ucar.edu/wrf/OnLineTutorial/compilation_tutorial.php?fbclid=IwAR3GIOcbAA4rwEjYAeeFDbaywNm4UvHz3CbeXtJbJRIaS9OF03BP5wfX-u8).

## Run Instructions
* Prepare the input data for WRF simulation
  ```
    cd test/em_les
    ./ideal.exe
  ```
* Enable log-layout based VOL by setting environment variables
  ```
    source ${HOME}/logvol/1.3.0/bin/h5lenv.bash
  ```
* Run em_les application with log-layout based VOL
  ```
    mpiexec -np 4 ./wrf.exe
  ```

## Example Results
We provide some example results running WRF with NetCDF4 via HDF5, with
the log-layout based VOL enabled

WRF write two kinds of outputs: restart files and history files. An history file contains the
simulatioin results for the user selected simulation timesteps. And a restart file serves as
a check point and contains all the necessary information to restart the WRF simulation from a
certain timestep. Each history file or restart file may contain multiple time steps.

In our experiments, we use the COUNS2.5 run case and produce one history file and one restart file.

The following table summarizes the information for the history and restart file in our experiment.

| file type | grid size (e\_we x e\_sn) | num time steps | num variables | file size (GB) |
|---|---|---|---|---|
| history | 1900 x 1300 | 13 | 202 | 103 |
| restart | 1900 x 1300 | 1 | 565 | 34 |

### Example Results on Cori at NERSC
Performance chart below shows the execution time, collected in Oct/2022, on
[Cori](https://docs.nersc.gov/systems/cori/) at [NERSC](https://www.nersc.gov).
All runs were on the KNL nodes, with 64 MPI processes allocated per node.

The Lustre file system is configured to use striping count of 8 OSTs
and striping size of 1 MiB.

| num nodes | num MPI processes | timing history (sec) | timing restart (sec) |
|---|---|---|---|
| 4 | 256 | 9.25 | 4.05 |
| 8 | 512 | 7.94 | 3.78 |
| 16 | 1024 | 7.83 | 3.75 |
| 32 | 2048 | 8.28 | 3.83 |

