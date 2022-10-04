# WRF case study

* [Build Instructions](#build-instructions)
* [Run Instructions](#run-instructions)
* [Three cases from E3SM production runs](#three-cases-from-e3sm-production-runs)
* [Performance Results](#performance-results)

This case study uses [WRF](https://github.com/Parallel-NetCDF/E3SM-IO) to evaluet the
performance of the HDF5 log-layout based VOL, compared with methods using other I/O libraries.

TODO: A brief intro. of WRF.

## Build Instructions
* Prerequisite
  + HDF5 1.13.0, required by any HDF5 VOL
  + HDF5 log-layout based VOL version 1.3.0
  + netcdf 4.1.3 or higher
  + Jasper-1.900.1 or higher
  + libpng-1.2.50 or higher
  + zlib-1.2.7 or higher
* Clone WRF from its github repository:
  ```
    git clone https://github.com/wrf-model/WRF.git
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

    export NETCDF_DIR=${HOME}/NetCDF/4.9.0
    export HDF5_DIR=${HOME}/HDF5/1.13.2
    export PHDF5_DIR=${HOME}/HDF5/1.13.2
    export PNETCDF=${HOME}/PnetCDF/1.12.3
    export ZLIB=${HOME}/zlib/1.2.7
    export WRF_DIR=${HOME}/WRF

    export NETCDF=$NETCDF_DIR
    export NETCDFPAR=$NETCDF_DIR
    export HDF5=$HDF5_DIR
    export PHDF5=$PHDF5_DIR

    export CPPFLAGS="$CPPFLAGS -I${JASPERINC} -I$HDF5/include -I$NETCDF/include -I$PNETCDF/include"
    export LDFLAGS="$LDFLAGS -L${JASPERLIB} -L$HDF5/lib -L$NETCDF/lib -L$PNETCDF/lib"
    export LD_LIBRARY_PATH="$HDF5/lib:$NETCDF/lib:$PNETCDF/lib:$LD_LIBRARY_PATH"
    export PATH="$HDF5_DIR/bin:$NETCDF_DIR/bin:$PNETCDF/bin:$PATH"
  ```
* Configure E3SM-IO with NetCDF4 features enabled.
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
    Two executables (ideal.exe, and wrf.exe) will be built under `test/em_les`.

* For more details, please refer to [WRF's official compilation tutorial](https://www2.mmm.ucar.edu/wrf/OnLineTutorial/compilation_tutorial.php?fbclid=IwAR3GIOcbAA4rwEjYAeeFDbaywNm4UvHz3CbeXtJbJRIaS9OF03BP5wfX-u8).

## Run Instructions
* Prepare the input data for WRF simulation
  ```
    cd test/em_les
    ./ideal.exe
  ```
* Set HDF5 default VOL to log-layout based VOL
  ```
    source ${HOME}/logvol/1.3.0//bin/h5lenv.bash
  ```
* Run em_les application with log-layout based VOL
  ```
    mpiexec -np 4 ./wrf.exe
  ```

## Performance Results
The performance numbers presented here compare three I/O methods used in WRF:
the log-layout based VOL, [PnetCDF](https://github.com/Parallel-NetCDF/PnetCDF),
and [ADIOS](https://github.com/ornladios/ADIOS2).

The PnetCDF method stores E3SM variables in files in a cononical storage layout.

TODO: Intro to WRF I/O pattern.

### Evaluation on Cori at NERSC
Performance chart below shows the execution time, collected in ???, on
[Cori](https://docs.nersc.gov/systems/cori/) at [NERSC](https://www.nersc.gov).
All runs were on the KNL nodes, with 64 MPI processes allocated per node.

For PnetCDF, the Lustre file system is configured to use striping count of 128 OSTs
and striping size of 16 MiB.
Both Log-layout based VOL and ADIOS runs enabled their subfiling feature, which
creates one file per compute node.
The Lustre striping configuration is set to striping count of 1 OST and striping
size of 16 MiB

<p align="center">
<img align="center" src="e3sm_cori_wr.jpg" alt="Performance of E3SM-IO on Cori" width="400">
</p>

### Evaluation on Summit at OLCF
Performance chart below shows the execution time, collected in ???, on
[Summit](https://www.olcf.ornl.gov/summit/) at [OLCF](https://www.olcf.ornl.gov/).
All runs allocated 84 MPI processes per node.
Summit's parallel file system, Spectrum file system (GPFS), was used.

<p align="center">
<img align="center" src="e3sm_summit_wr.jpg" alt="Performance of E3SM-IO on Summit" width="400">
</p>
