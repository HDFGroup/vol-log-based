## Using the Log-based VOL

### Enable log-based VOL in HDF5 applications
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
  + The log-based VOL can be enabled at the run time by setting the
    environment variables below. No source code changes to existing user
    programs are required. (Note this is an HDF5 feature, applicable to all VOL
    plugins.)
    + Append log-based VOL library directory to the shared object search path,
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
  + The log-based VOL comes with utility script h5lenv.bash and h5lenv.csh to set the environment variables automatically for bash and tcsh shells.
     ```
     % source ${HOME}/Log_IO_VOL/bin/h5lenv.bash
     ```

### Enable log-based VOL subfiling feature
Subfiling is a feature to mitigate the file lock conflict in a parallel write
operation to a shared file. It divides MPI processes into groups disjointedly
and creates one file (named subfile) per group. Each subfile is accessible only
to the processes in the same group and contains data written by those processes
only. By reducing the number of MPI processes sharing a file, subfiling reduces
the file access contentions, and hence effectively improves the degree of I/O
parallelism.

* Enabling subfiling through an environment variable
  + Set the environment variable `H5VL_LOG_NSUBFILES` to the number of
    subfiles, for example
    ```
    % export H5VL_LOG_NSUBFILES=2
    ```
  + If the environment variable is set without a value or with a non-positive
    value, then the number of subfiles will be set to equal to the number of
    compute nodes allocated.
    ```
    % export H5VL_LOG_NSUBFILES=
    ```
  + If the environment variable is not set or set to 0, then the subfiling is
    disabled.
* Output file structure and naming scheme.
  * A master file (whose file name is supplied by the user to `H5Fcreate`)
    contains:
    + User attributes.
    + User datasets as scalars. The contents of datasets are stored in the
      subfiles.
  * A new directory containing all the subfiles.
    + The directory is named `<master_file_name>.subfiles`.
  * Subfiles
    + Each subfile is named `<master_file_name>.id` where `id` is the ID
      starting from 0 to the number of subfiles minus one.
    + Each subfile stores the log data of all partitioned datasets.

### Differences from the native VOL
  * Buffered and non-buffered modes
    + H5Dwrite can be called in either buffered or non-buffered mode.
    + The mode can be set in the dataset transfer property list through API
      `H5Pset_buffered()`
    + The mode can be queried through API `H5Pget_buffered()`.
    + When in the non-buffered mode, the user buffer should not be modified
      between the call to `H5Dwrite()` and `H5Fflush()`, as it will be used to
      flush the write data at `H5Fflush()` or `H5Fclose()`.
    + On the other hand, when in buffered mode, the write data will be buffered
      in an internal buffer. The user buffer can be modified after the call to
      `H5Dwrite()` returns.
  * Write operations are always non-blocking
    + `H5Dwrite()` only caches the write request data internally.
    + Users must call `H5Fflush()` explicitly to flush the data to the file.
    + All cached write data is flushed at `H5Fclose()`.
  * Read operations are non-blocking
    + Even after the call to `H5Dread()` returns, the data is not read into the
      user buffer until the call to `H5Fflush()` returns.

### Current limitations
  * Blocking read operations must be collective.
    + H5Dread must be called collectively.
    + Calling H5Dread automatically triggers a flush.
  * Read operations may be slow:
    + Reading is implemented by naively searching through log records to find
      the log blocks intersecting with the read request.
    + The searching requires reading the entire metadata of the file into the memory.
  * Async I/O (a new feature of HDF5 in the future release) is not yet supported.
  * Virtual Datasets (VDS) feature is not supported.
  * Multiple opened instances of the same file are not supported.
    + The log-based VOL caches some metadata of an opened file.
      The cached metadata is not synced among opened instances.
    + The file can be corrupted if the application opens and operates multiple handles to the same file.
  * The names of (links to) objects cannot start with the prefix double underscore "__".
    + Names starting with double underscore "__" are reserved for the log-based VOL for its internal data and metadata.
  * The log-based VOL does not support all the HDF5 APIs.
    See [doc/compatibility.md](./compatibility.md) for a full list of supported and unsupported APIs.
  * Lob-based VOL will crash if there are any objects left open when the program exit.
    + All opened HDF5 handles must be properly closed.

### Log-based VOL-specific APIs
* For a list of APIs introduced in the log-based VOL, see [doc/api.md](./api.md)

### Log-based VOL utilities
* For instructions related to log-based VOL utility programs, see [doc/util.md](./util.md)
