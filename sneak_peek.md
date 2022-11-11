------------------------------------------------------------------------------
This is essentially a placeholder for the next release note ...
------------------------------------------------------------------------------
(Copy the contents below to RELEASE_NOTE.md when making an official release.)

* New features
  + Support fill mode. See 8aee024
  + Support NetCDF4. NetCDF4 users now can set the two VOL environment
    variables `HDF5_VOL_CONNECTOR` and `HDF5_PLUGIN_PATH` to write data to
    files in log layout. See PR #15.

* New optimization
  + Master file opened by rank 0 only when subfiling is enabled
    + Improves native VOL file close time
    + See 99f813a.

* New Limitations
  + Log VOL currently does not support multiple opens of the same file.

* Update configure options
  + none

* New constants
  + none

* New APIs
  + none

* New native HDF5 APIs
  + none

* API syntax changes
  + H5Pget/set_subfiling property type changed to int
    + Signature
      ```
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

* Build recipes
  + none

* Updated utility program
  + `h5ldump` and `h5lreplay` now return error code on failure. See PR #16.

* Other updates:
  + Add a new use case of E3SM in case_studies/E3SM_IO.md. See 1803b11.
  + Add a new use case of WRF in case_studies/WRF.md. See PR #34.
  + Allow user object name starts with '_'. See PR #29.
  + Support multiple file opens to the same file. See PR #28.

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
  + Fix a bug in H5VL_log_filei_close that fails to update file attributes property when subfiling is enabled. See 79e91ec.
  + Fix a bug in encoding and decoding of deduplicated metadata entries. See 99d3fda.
  + Fix a bug in H5VL_log_file_create that does not set underlying VOL when subfiling is enabled. See 2532553.
  + Fix a bug in metadata encoding for record writes. See 8b68e0f.
  + Deduce internal attributes from attributes count in object info

* New example programs
  + none

* New programs for I/O benchmarks
  + none

* New test program
  + tests/testcases/multi_open
    + Test opening a second handle to the same file without closing the first one.
  + tests/basic/fill
    + Test the fill mode feature on datasets.
  + tests/dynamic/test_env
    + Test changing the VOL environment variable between file create/open.

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
