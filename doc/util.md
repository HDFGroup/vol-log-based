### Log-based VOL utilities

* h5lconfig
  + Script for querying configuration and installation options used to build the log-based VOL
  + Usage
    ```
      % ${logvol_install_path}/bin/h5lconfig --help
      h5lconfig is a utility program to display the build and installation
      information of the log-based VOL library.

      Usage: h5lconfig [OPTION]

      Available values for OPTION include:

        --help                      display this help message and exit
        --all                       display all options
        --cc                        C compiler used to build log-based VOL
        --cflags                    C compiler flags used to build log-based VOL
        --cppflags                  C pre-processor flags used to build log-based VOL
        --c++                       C++ compiler used to build log-based VOL
        --cxxflags                  C++ compiler flags used to build log-based VOL
        --ldflags                   Linker flags used to build log-based VOL
        --libs                      Extra libraries used to build log-based VOL
        --profiling                 Whether internal profiling is enabled or not
        --debug                     Whether log-based VOL is built with debug mode
        --prefix                    Installation directory
        --includedir                Installation directory containing header files
        --libdir                    Installation directory containing library files
        --version                   Library version
        --release-date              Date of log-based VOL source was released
        --config-date               Date of log-based VOL library was configured
    ```
* h5ldump
  + Print the content of a log-based VOL output file
  + This utility is sequential
  + Examples
    ```
      % ${logvol_install_path}/bin/h5ldump -h
      Usage: h5ldump [OPTION] FILE
             [-h] Print this help message
             [-v] Verbose mode
             [-H] Dump header metadata only
             [-k] Print the kind of file, one of 'HDF5', 'HDF5-LogVOL', 'NetCDF-4',
                  'NetCDF classic', 'NetCDF 64-bit offset', or 'NetCDF 64-bit data'
             FILE: Input file name

      % ${logvol_install_path}/bin/h5ldump test.h5 
      File: test.h5
          Configuration: metadata encoding, metadata compression, 
          Number of user datasets: 1
          Number of data datasets: 1
          Number of metadata datasets: 2
          Number of metadata datasets: 2
          Metadata dataset 0
              Number of metadata sections: 2
              Metadata section 0: 
                  Metadata entry at 0: 
                      Dataset ID: 0; Entry size: 72
                      Data offset: 2228; Data size: 8
                      Flags: multi-selection, encoded, 
                      Encoding slice size: (2, )
                      Selections: 2 blocks
                          0: ( 0, 0, ) : ( 1, 1, )
                          4: ( 1, 0, ) : ( 1, 1, )
              Metadata section 1: 
                  Metadata entry at 0: 
                      Dataset ID: 0; Entry size: 72
                      Data offset: 2236; Data size: 8
                      Flags: multi-selection, encoded, 
                      Encoding slice size: (2, )
                      Selections: 2 blocks
                          0: ( 0, 1, ) : ( 1, 1, )
                          4: ( 1, 1, ) : ( 1, 1, )
          Metadata dataset 1
              Number of metadata sections: 0
    ```
* h5lreplay
  + Convert log-based VOL output file into traditional HDF5 files
  + This utility support parallel run
  + Usage
    ```
      % mpiexec -np ${N} ${logvol_install_path}/bin/h5lreplay -h
      Usage: h5reply -i <input file path> -o <output file path>

      % mpiexec -np ${N} ${logvol_install_path}/bin/h5lreplay -i test.h5 -o test_out.h5
      Usage: h5reply -i <input file path> -o <output file path>

      % h5dump test_out.h5
      HDF5 "test_out.h5" {
      GROUP "/" {
        DATASET "D" {
            DATATYPE  H5T_STD_I32LE
            DATASPACE  SIMPLE { ( 2, 2 ) / ( 2, 2 ) }
            DATA {
            (0,0): 1, 2,
            (1,0): 2, 3
            }
        }
      }
      }
    ```
* h5lenv
  + Set up environment variables to use log-based VOL as the default VOL
  + Applications that do not specify a VOL will use the VOL specified in the environment variables
  + Usage
    ```
    % source h5lenv.bash
    ```
    ```
    % source h5lenv.csh
    ```
