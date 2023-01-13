## Release Notes of the Log Layout Based HDF5 Virtual Object Layer

### Version 1.4.0 (Jan 13, 2023)
* New features
  + Support fill mode. See commit 8aee024
  + Support NetCDF4. By setting the two VOL environment variables
    `HDF5_VOL_CONNECTOR` and `HDF5_PLUGIN_PATH`, NetCDF4 programs now can write
    data to files in log layout through the Log VOL connector. See PR #15.
  + Support opening and operating an existing regular HDF5 file.
  + Support using the Log VOL connector as a pass-through VOL connector when performing writes.
    * Whe performing writes, users can specify and choose other VOL connectors as the underlying VOL connectors of the Log VOL connector. See PR #33.
    * When the pass-through feature is enabled, it asks the underlying VOLs to use collective MPI I/O. See PR #42.
    * For its usage, refer to the User Guide in doc/usage.md.
  + Support VOL connector interface version 3.
    * HDF5 1.13.3 requires a version 3 connector.
    * The Log VOL connector will expose the version 3 interface when the HDF5 version 
      is 1.13.3 and above.
    * See https://www.hdfgroup.org/2022/10/release-of-hdf5-1-13-3-newsletter-188/
  + Support of stacking the Log VOL connector on top of the Cache and Async VOL connectors.

* New optimization
  + Master file opened by rank 0 only when subfiling is enabled. This
    significantly improves the file close time. See commit 99f813a.

* New Limitations
  + The Log VOL connector currently does not support multiple opens of the same file.

* New APIs
  + H5Pget_passthru. See PR #33.
    + Signature
      ```c
      herr_t H5Pget_passthru(hid_t faplid, hbool_t *enable);
      ```
  + H5Pset_passthru. See PR #33.
    + Signature
      ```c
      herr_t H5Pget_passthru(hid_t faplid, hbool_t *enable);
      ```

* API syntax changes
  + H5Pget/set_subfiling property type changed to int
    + Signature
      ```c
        herr_t H5Pset_subfiling (hid_t fcplid, int  nsubfiles);
        herr_t H5Pget_subfiling (hid_t fcplid, int *nsubfiles);
      ```
      `fcplid` is the file creation preperty list ID.
    + Allows setting the number of subfiles including disabling/enabling
      subfiling. See PR ##32.
      + When nsubfiles is 0: disable subfiling.
      + When nsubfiles is a negative values: one subfile per computer node.
      + When nsubfiles is a positive value: create "nsubfiles" subfiles.
  + Rename APIs H5Pset/get_nonblocking to H5Pset/get_buffered. See PR #23.
  + Deprecate H5S_CONTIG, as it serves the same purpose of H5S_BLOCK, which
    describe a contiguous data space. See PR #22.

* Run-time environment variables
  + Environment variable `H5VL_LOG_NSUBFILES` has been changed to match the
    argument "nsubfiles" in API `H5Pget/set_subfiling` described above.
  + Add environment variable `H5VL_LOG_PASSTHRU`. See PR #33, #51.

* Updated error strings
  + When the same file is opened more than once, the following error string
    will be printed. See PR #38.
    ```
    The same file has been opened. The Log VOL connector currently does not support multiple opens.
    ```

* Updated utility program
  + `h5ldump` and `h5lreplay` now return error code on failure. See PR #16.

* Other updates:
  + Wrap calls to HDF5 VOL lib state APIs around attribute APIs. See PR #41.
  + Add a new use case of E3SM in case_studies/E3SM_IO.md. See commit 1803b11.
  + Add a new use case of WRF in case_studies/WRF.md. See PR #34.
  + Allow user object name starts with '_'. See PR #29.
  + Support checking multiple file opens to the same file. An error will return
    instead of corrupting the file. See PR #28.
  + Error messages are printed only in debug mode. See PR #39.
  + Move the Log VOL connector's data object creation to post open stage.
    + HDF5 1.13.3 and above does not allow creating data objects in the file_create 
      callback.
  + OpenPMD test temporarily disabled in git action script due to a bug in OpenPMD.
    + See https://github.com/openPMD/openPMD-api/issues/1342

* Bug fixes
  + Fix a bug for allowing to run ncdump when the two VOL environment variables
    are set. See PR #35.
  + Fix a bug for zero-sized write requests. In HDF5, the correct way to do
    zero-size write is to call select API H5Sselect_none() using the dataset's
    dataspace, instead of H5Screate(H5S_NULL). See detailed comments in PR #21.
  + Fix a bug when reading interleaving regions within a log entry
    + H5VL_log_dataset_readi_gen_rtypes produces an invalid file type due to
      interleaving read regions not being detected
  + Fix a bug in H5VL_log_link_create that uses the calling convention of an
    older VOL interface
  + Fix a bug in H5VL_log_filei_close that fails to update file attributes
    property when subfiling is enabled. See commit 79e91ec.
  + Fix a bug in encoding and decoding of de-duplicated metadata entries. See
    commit 99d3fda.
  + Fix a bug in H5VL_log_file_create that does not set underlying VOL when
    subfiling is enabled. See commit 2532553.
  + Fix a bug in metadata encoding for record writes. See commit 8b68e0f.
  + Deduce internal attributes from attributes count in object info.
  + Fix a memory bug in H5VL_log_str_to_info. See commit b832b3c.

* New test program
  + tests/testcases/multi_open
    + Test opening a second handle to the same file without closing the first
      one.
  + tests/basic/fill
    + Test the fill mode feature on datasets.
  + tests/dynamic/test_env
    + Test changing the VOL environment variable between file create/open.
  + tests/read_regular
    + Test opening and operating an existing regular HDF5 file.
  + tests/passthru
    + Test the Log VOL connector's behavior as a Pass-through VOL connector. It's expected that users
      enable other VOL connectors as the underlying VOL connectors of the Log VOL connector before running/testing
      the programs under this folder.
    + PR #44 provides an example of running the Log VOL connector on top of the Cache VOL and Async VOL connector.
  + testing the Log VOL connector as a Pass-through VOL.
    + tests/testcases, tests/basic, tests/dynamic, and tests/read_regular
      additionaly tests using the Log VOL connector on top of the native VOL. Test programs
      are re-used and no addtional test programs are added. Unlike tests/passthru
      where we expect users to specify the underlying VOLs, all necessary
      envrionment varibales are set in the wrap_runs.sh and parallel_run.sh
      scripts. Running "make check" or "make ptest" is enough perform the test.

### Version 1.3.0 (May 05, 2022)
* New optimization
  + Improve dataspace selection parsing speed when there are a large number of
    hyper-slabs that can be coalesced.

* New APIs
  + To reflect the functionality of the APIs, `H5Pset_nonblocking` and
    `H5Pget_nonblocking` have been renamed to `H5Pset_buffered` and
    `H5Pget_buffered`, respectively. See PR #23
    + New APIs that let users to tell the Log-based VOL whether it should
      buffer the write data. If argument buffered is true (default), then the
      user write buffer can be modified after `H5Dwrite`/`H5Dwrte_n` returns.
      ```
      herr_t H5Pset_buffered (hid_t plist, hbool_t buffered);
      herr_t H5Pget_buffered (hid_t plist, hbool_t *buffered);
      ```
      If not set, the default property is buffered.
    + These two APIs below will be deprecated in the future releases, but kept
      for backward compatibility.
      ```
      herr_t H5Pset_nonblocking (hid_t plist, H5VL_log_req_type_t nonblocking);
      herr_t H5Pget_nonblocking (hid_t plist, H5VL_log_req_type_t *nonblocking);
      ```
* New run-time environment variable
  + `H5VL_LOG_NSUBFILES`
    + Enable subfiling and set the number of subilfes.
    + When the variable is not set, the subfiling is disabled.
    + When the variable is set without a value, i.e. through command
      ```
      export H5VL_LOG_NSUBFILES=
      or
      setenv H5VL_LOG_NSUBFILES
      ```
      then subfiling is enabled and the default is one file per compute node
      allocated.
    + Old environment variables `H5VL_LOG_SUBFILING` and `H5VL_LOG_N_SUBFILE`
      introduced in 1.2.0 are deprecated.

* New utility programs
  + `h5lpcc`
    + A C compiler wrapper to compile applications using log-based VOL. It is
      similar to HDF5 h5pcc.
  + `h5lpcxx`
    + A C++ compiler wrapper to compile applications using log-based VOL.

* Other updates:
  + Change the naming scheme for subfiles to `master_file_name.subfile_ID`.
  + Replace all error codes with C++ exceptions.

* Deprecated constant
  + Constant `H5S_CONTIG` is deprecated in 1.3.0, as it serves exactly the same
    purpose as `H5S_BLOCK` which is first defined in HDF5 1.13.0. See PR #22

* Bug fixes
  + Fix a bug when writing a zero-sized request. See commits a67fb43 and 5acb7e3.
  + Fix a hanging bug when a subset of processes write zero-sized requests.
    See commit 3f34656.
  + Fix a bug in `h5lreplay` that ignores attributes.
  + Fix a bug in `h5lreplay` that crashes when there are groups.
  + Fix a bug in `h5lconfig` that reports the wrong library path.

* New example programs
  + A set of demo programs that shows the required modification of traditional
    HDF5 programs in order to use log-based VOL.
    + `examples/demo.cpp` is a simple HDF5 application.
    + `examples/demo_logvol.cpp` modifies `demo.cpp` to use log-based VOL.
    + `examples/demo_sblock.cpp` uses HDF5's `H5S_BLOCK` constant
      to replace memory dataspace argument for the contiguous memory buffer.
    + `examples/demo_buffered.cpp` calls API `H5Pset_buffered` to disable the
      Log-based VOL internal data buffering.
    + `examples/demo_dwrite_n.cpp` calls API `H5Dwrite_n` to write multiple
      block in a dataaset.

* New test programs
  + `tests/testcases/null_space.cpp` tests when only process rank 0 writes a
    zero-sized request using `H5Sselect_none()` and others writes
    non-zero-sized amount.
  + `utils/h5lreplay/h5lgen.cpp` generates a log-based HDF5 file to test
    utility program `h5lreplay`. For correctness test, the converted file is
    compared with the file written using the native VOL.

* Clarifications
  + In HDF5, the correct way to do zero-size write is to call select API
    `H5Sselect_none()` using the dataset's dataspace, instead of
    `H5Screate(H5S_NULL)`. HDF5 reference manual says
    ```
    Any dataspace specified in file_space_id is ignored by the library and the
    dataset's dataspace is always used.
    ```
    The dataset's dataspace here is the dataspace used to create the dataset.
    Once the dataset is created, its dataspace can be set to a selected area
    (e.g. subarray) and passed into `H5Dwrite()` to write to an subbarray space
    of the dataset occupied in the file. Note that the size of argument
    `mem_type_id` must match argument `file_space_id`.

### Version 1.2.0 (March 27, 2022)
* New features
  + Flush write requests before handling any read requests.
* Update configure options
  + `--enable-test-qmcpack` to enable tests using QMCPACK [default: disabled].
    This option first downloads and builds QMCPACK. Its test program
    `restart.c` will be tested during `make check`.
  + `--enable-test-hdf5-iotest` to enable tests using hdf5-iotest [default:
    disabled]. This option first downloads and builds hdf5-iotest. Its test
    program `hdf5_iotest.c` will be tested during `make check`.
  + `--enable-test-openpmd` to enable tests using OpenPMD [default: disabled].
    This option first downloads and builds OpenPMD. Its test program
    `8a_benchmark_write_parallel.c` and `8b_benchmark_read_parallel.c` will be
    tested during `make check`.
  + `--enable-test-netcdf4[=INC,LIB | =DIR]` to enable tests using NetCDF4
    [default: disabled], and provide the NetCDF installation path(s):
    `--enable-test-netcdf4=INC,LIB` for include and lib paths separated by a
    comma. `--enable-test-netcdf4=DIR` for the path containing include/ and
    lib/ subdirectories.  This option first downloads a few test programs from
    NetCDF-C, which will be tested during `make check`.
* Updated utility program
  + h5ldump and h5lenv check for file signature before parsing files.
  + Revise h5ldump command-line options. Option -H is to dump file header
    metadata only. Option -k is to print the file kind only. Option -v is to
    enable verbose mode.
* Other updates:
  + Supports NetCDF4 applications. See PR #15
* Bug fixes
  + `h5ldump` now can use either the native or log-based VOL depending on the
    environment variable HDF5_VOL_CONNECTOR. 
  + `h5lreplay` now queries the dataset create property list `dcplid` before
    extracting filters.
  + `h5lreplay` now recognizes the internal objects of names prefixed with
    double '_'.
  + Fix metadata flush-after-read operations that corrupt the file.
  + Fix write request not flushed properly after the first metadata flush.
  + Fix memory error when writing filtered datasets.
  + Fix a bug in dataspace selection when the least significant dimension of interleaving blocks contains more than one element.
  + h5ldump and h5lenv return -1 if it fails to handle the file.
* New test program
  + NetCDF4 test programs are now downloaded from NetCDF release at make time.
    See PR #15.
  + HDF5 example programs are now downloaded from HDF5 releases at make time.
    See PR #18.

### Version 1.1.0 (Feburary 11, 2022)
* New features
  + Support multiple opened instance of the same dataset.
    + Commit 37eed305581653cbb1e021a3c23389a55043b2e5
* New optimization
  + Coalesce hyper-slab and points selections to reduce metadata size and index searching time on reading.
    + Commit 181abe3094e0e961e7e04bb76bab6e6ac45cbfd1
  + Improve indexing memory consumption on dataset read with a redesigned compact index data structure.
    + Commit 3b08ff37f43dae46de74408ef15871ccf1f23b02
* New native HDF5 APIs
  + H5Ovisit, H5Literate, and H5Lvisit.
* Bug fixes
  + Fix missing starts and counts assignments in example programs "examples/dwrite_n.cpp and "examples/dread_n.cpp".
    + Commit bab2f2f03c1a6c9ed179ef18062366874ef72e1b
  + Fix bug on metadata flush when some processes have no data to flush.
    + Avoid creating empty compound datatypes, as some MPI implementation does not allow it.
    + COmmit ea081e8cf479336244337e4d9411b380c6fccdb3
  + Fix H5Oopen on dataset crashing issue.
    + Commit 36ff7f323b8264d24a7876f882dd39ffdb03acb9
  + Fix mistyped variable data type for file info.
  + Fix bug on metadata encoding not following the file configure flag.
    + Commit c8003451ab540b32ed6392646d0ebb671020ce36
  + Fix bug on generating MPI datatype for reading.
    + Check subarray interleaving only when the target falls into the same block of data.
    + File offset must include selection block offset.
  + Fix bug for accessing scalar and 1-D datasets.
    + Commit bc3b94d77a00be583e8d6b668d12fdc9b8e6c2da
  + Fix referenced metadata decoding bug.
    + Commit e806234498cf605fad9f4d861396aca4caf8af89
  + Fix using the wrong MPI communicator on metadata flush barrier.
    + Commit 4df75f826a2f81ef1329250f1e02fcd5a33484a9
  + Fix metadata flag bug after metadata compression fails.
    + Commit 34609d90a752b9b0bac574ed76d4ad602bd3540a
  + Fix a bug that incorrectly erases metadata index.
    + Commit 5859e6014aa04700f1fe7aed5665f9aa5f0382e3
  + Fix dataset I/O bug when using "H5S_ALL".
    + Commit b5302d004d80473d6aa93ed5efb2cbae3f57e827
  + Fix calculation for determining whether the file space is interleaving.
    + Commit 8959e59798066ec166b51f3696b93efe594f0c1d
  + Fix a bug when memory space and file space have different number of dimensions.
  + Fix a reference count bug when VOL info is created from a string.
    + Commit 646a0c640d68a1bc15ddf24a16d9bfbc7e6263fa
* New test program
  + Added tests of parallel runs. Command "make ptest" will run test programs
    in parallel using 4 MPI processes.
  + A new test program 'tests/testcases/scalar.cpp'  to read/write test scalar
    datasets.
  + An external test program qmcpack restart 
    + https://github.com/QMCPACK/qmcpack.git
  + An external test program hdf5-iotest   
    + https://github.com/HDFGroup/hdf5-iotest
  + An external test program openPMD-api benhmark
    + https://github.com/openPMD/openPMD-api
* Updated utility program
  + Add command-line option "-d" to utility program "h5ldump" for dumping data
    of each selection block.
  + h5dump can now read a log-based VOL output file using log-based VOL.
  + Add utility script to set HDF5 environment to use log-based VOL.
    + h5lenv.bash, h5lenv.csh
* Other updates:
  + Add utility scripts ("utils/h5lenv.bash.in" and "utils/h5lenv.csh.in") to
    set HDF5 environment to use log-based VOL.
  + This log-layout based VOL has been registered with HDF5 group with
    Connector Identifier 514.
    https://portal.hdfgroup.org/display/support/Registered+VOL+Connectors
  + Change the required HDF5 library from the develop branch to official
    release of 1.13.0.
  + Support build on MacOS.
  + Solve compatibility issues with OpenMPI.
  + Use "MPI_COMM_SELF" if the communicator is not set to allow single process I/O.

### Version 1.0.0 (December 2, 2021)
* First public release of log-based VOL.
* Features implemented.
  + Support major HDF5 APIs.
  + Documentation on log-based VOL APIs.
  + Utility programs.
    + h5ldump - Dump the log metadata in a log-based VOL outptu file.
    + h5lreplay - Convert log-based VOL output file to traditional HDF5 file.
  + Utility script for querying configuration and installation options.
    + h5lconfig 
  + Example programs to demonstrate the use of log-based VOL.
