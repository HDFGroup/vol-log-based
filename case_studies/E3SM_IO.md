## E3SM-I/O case study

The [E3SM I/O benchmark](https://github.com/Parallel-NetCDF/E3SM-IO) reproduces the I/O pattern of the [E3SM simulation framework](https://github.com/E3SM-Project/E3SM) captured by the Scorpio library during the E3SM production runs.

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
* Details on running E3SM I/O can be found in the [E3SM I/O README file](https://github.com/Parallel-NetCDF/E3SM-IO/blob/master/README.md)
* Running with HDF5 native VOL
  ```
    mpiexec -np 16 src/e3sm_io -a hdf5 -x canonical -k -o ${HOME}/e3sm_io_native datasets/f_case_866x72_16p.nc
  ```
* Running with HDF5 log-based VOL
  ```
    mpiexec -np 16 src/e3sm_io -a hdf5_log -x log -k -o ${HOME}/e3sm_io_log datasets/f_case_866x72_16p.nc
  ```

### E3SM I/O evaluation setup
The E3SM I/O benchmark contains the I/O kernel of E3SM's three components - 
the atmospheric component (F case), the oceanic component (G case), and the land component (I case).
The F case and the I case writes two files - H0, and H1.
The G case writes one file.

We evaluate log-based VOL using the I/O pattern recorded in a production run of E3SM high-resolution simulation.
The properties of the output files and their configurations are shown below. 

|     Number of processes                |     21600     |     21600    |     9600     |     1344       |     1344     |
|----------------------------------------|---------------|--------------|--------------|----------------|--------------|
|     Total size of data (GiB)           |     14.09     |     6.68     |     79.69    |     86.11      |     0.36     |
|     Number of fixed sized variables    |     15        |     15       |     11       |     18         |     10       |
|     Number of record variables         |     399       |     36       |     41       |     542        |     542      |
|     Number of records                  |     1         |     25       |     1        |     240        |     1        |
|     Number of partitioned vars         |     25        |     27       |     11       |     14         |     14       |
|     Number of non-partitioned vars     |     389       |     24       |     41       |     546        |     538      |
|     Number of non-contig               |     174953    |     83261    |     20888    |     9248875    |     38650    |
|     Number of attributes               |     1427      |     148      |     858      |     2789       |     2759     |

We compare the performance between log-based VOL, [PnetCDF](https://github.com/Parallel-NetCDF/PnetCDF), and [ADIOS](https://adios2.readthedocs.io/en/latest/#).

PnetCDF stores E3SM variables in a contiguous storage layout.
Each process writes multiple non-contiguous blocks in a variable.
To improve performance, the benchmark use PnetCDF's non-blocking API that aggregates all write requests in a timestep into a single MPI collective write request.

E3SM uses ADIOS through its [SCORPIO](https://github.com/E3SM-Project/scorpio) module.
The SCORPIO module rearranges the I/O pattern in which each process writes a contiguous block of data per variable.
Processes stores the data in ADIOS's local variables.
Local variables are only a collection of data blocks without any metadata describing their logical location.
The logical location of the data as well as other metadata is managed by the SCORPIO module.

### Performance Results on Cori at NERSC
Below shows the performance in execution times collected in September 2021 for
log-based VOL on [Cori](https://docs.nersc.gov/systems/cori/) at
[NERSC](https://www.nersc.gov).
We run 64 processes per node on Cori.
Should the number of processes does not divide the number of processes per node, we allow some nodes to host fewer processes.

We build E3SM I/O, log-based VOL, and HDF5 using the default toolchain on Cori.
We evaluate log-based VOL using Cori's KNL nodes and Lustre file system.
For PnetCDF, the file system is configured to span across 128 OSTs with 16 MiB stripe sizes.
ADIOS and log-based VOL creates one file per node, thus, the file system is configured to store the file on a single OST with a 16 MiB stripe size.

<p align="center">
<img align="center" src="e3sm_cori_wr.jpg" alt="Performance of log-based VOL on Cori" width="600">
</p>

### Performance Results on Summit at OLCF
Below shows the performance in execution times collected in September 2021 for
log-based VOL on [Summit](https://www.olcf.ornl.gov/summit/) at [OLCF](https://www.olcf.ornl.gov/).
We run 84 processes per node on Summit.
Should the number of processes does not divide the number of processes per node, we allow some nodes to host fewer processes.

We build E3SM I/O, log-based VOL, and HDF5 using the default toolchain on Summit.
We evaluate log-based VOL on Summit's Spectrum file system (GPFS).

<p align="center">
<img align="center" src="e3sm_summit_wr.jpg" alt="Performance of log-based VOL on Summit" width="600">
</p>