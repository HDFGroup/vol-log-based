## E3SM-I/O case study

The E3SM I/O benchmark reproduces the I/O pattern of the E3SM simulation framework
captured by the Scorpio library during the E3SM production runs.

### Building E3SM I/O
* Prerequisite
  + HDF5 1.13.0
  + log-based VOL 1.3.0
* Clone E3SM I/O from git repository
  ```
    git clone https://github.com/Parallel-NetCDF/E3SM-IO.git
  ```
* Configure E3SM I/O with HDF5 and log-based VOL support
  ```
    cd E3SM-IO
    autoreconf -i
    ./configure --with-hdf5=${HOME}/hdf5/1.13.0 --with-logvol=${HOME}/log_based_vol/1.3.0
  ```
* Build E3SM I/O
  ```
    make -j 64
  ```

### Running E3SM I/O
* Command-line options
  ```
    $ ./src/e3sm_io -h
    Usage: ./src/e3sm_io [OPTION] FILE
       [-h] Print this help message
       [-v] Verbose mode
       [-k] Keep the output files when program exits (default: deleted)
       [-m] Run test using noncontiguous write buffer (default: contiguous)
       [-f num] Output history files h0 or h1: 0 for h0 only, 1 for h1 only,
                -1 for both. Affect only F and I cases. (default: -1)
       [-r num] Number of time records/steps written in F case h1 file and I
                case h0 file (default: 1)
       [-y num] Data flush frequency. (1: flush every time step, the default,
                and -1: flush once for all time steps. (No effect on ADIOS
                and HDF5 blob I/O options, which always flushes at file close).
       [-s num] Stride interval of ranks for selecting MPI processes to perform
                I/O tasks (default: 1, i.e. all MPI processes).
       [-g num] Number of subfiles, used by ADIOS I/O only (default: 1).
       [-o path] Output file path (folder name when subfiling is used, file
                 name otherwise).
       [-a api]  I/O library name
           pnetcdf:   PnetCDF library (default)
           netcdf4:   NetCDF-4 library
           hdf5:      HDF5 library
           hdf5_log:  HDF5 library with Log-based VOL
           adios:     ADIOS library using BP3 format
       [-x strategy] I/O strategy
           canonical: Store variables in the canonical layout (default).
           log:       Store variables in the log-based storage layout.
           blob:      Pack and store all data written locally in a contiguous
                      block (blob), ignoring variable's canonical order.
       FILE: Name of input file storing data decomposition maps.
  ```
* Running with HDF5 native VOL
  ```
    mpiexec -np 16 src/e3sm_io -a hdf5 -x canonical -k -o ${HOME}/e3sm_io_native datasets/f_case_866x72_16p.nc
  ```
* Running with HDF5 log-based VOL
  ```
    mpiexec -np 16 src/e3sm_io -a hdf5_log -x log -k -o ${HOME}/e3sm_io_log datasets/f_case_866x72_16p.nc
  ```

### Performance Results on Cori at NERSC
Below shows the performance in execution times collected in September 2021 for
log-based VOL on [Cori](https://docs.nersc.gov/systems/cori/) at
[NERSC](https://www.nersc.gov).
<p align="center">
<img align="center" src="e3sm_cori_wr.jpg" alt="Performance of log-based VOL on Cori" width="600">
</p>

### Performance Results on Summit at OLCF
Below shows the performance in execution times collected in September 2021 for
log-based VOL on [Summit at OLCF](https://www.olcf.ornl.gov/summit/).
<p align="center">
<img align="center" src="e3sm_summit_wr.jpg" alt="Performance of log-based VOL on Summit" width="600">
</p>
