------------------------------------------------------------------------------
This is essentially a placeholder for the next release note ...
------------------------------------------------------------------------------
(Copy the contents below to RELEASE_NOTE.md when making an official release.)

* New features
  + Flush write requests before handling any read requests.

* New optimization
  + none

* New Limitations
  + none

* Update configure options
  + `--enable-test-qmcpack` to enable tests using QMCPACK [default: disabled].
    This option first downloads and builds QMCPACK. Its test program
    `restart.c` will be tested during `make check`.
  + `--enable-test-hdf5-iotest` to enable tests using hdf5-iotest [default:
    disabled]. This option first downloads and builds hdf5-iotest. Its test
    program `hdf5_iotest.c` will be tested during `make check`.
  + `--enable-test-openpmd` to enable tests using OpenPMD [default: disabled].
    This option first downloads and builds OpenPMD. Its test program
    `hdf5_iotest.c` will be tested during `make check`.
  + `--enable-test-netcdf4[=INC,LIB | =DIR]` to enable tests using NetCDF4
    [default: disabled], and provide the NetCDF installation path(s):
    `--enable-test-netcdf4=INC,LIB` for include and lib paths separated by a
    comma. `--enable-test-netcdf4=DIR` for the path containing include/ and
    lib/ subdirectories.  This option first downloads a few test programs from
    NetCDF-C, which will be tested during `make check`.

* New constants
  + none

* New APIs
  + none

* New native HDF5 APIs
  + none

* API syntax changes
  + none

* API semantics updates
  + none

* New error code precedence
  + none

* Updated error strings
  + none

* New error code
  + none

* New I/O hint
  + none

* New run-time environment variables
  + none

* Build recipes
  + none

* Updated utility program
  + h5ldump and h5lenv check for file signature before parsing files.

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

* New example programs
  + none

* New programs for I/O benchmarks
  + none

* New test program
  + NetCDF4 test programs downloaded from NetCDF git repo at compile time

* Conformity with HDF5 library
  + none

* Discrepancy from HDF5 library
  + none

* Issues related to MPI library vendors:
  + none

* Issues related to Darshan library:
  + none

* Clarifications
  + none