------------------------------------------------------------------------------
This is essentially a placeholder for the next release note ...
------------------------------------------------------------------------------
(Copy the contents below to RELEASE_NOTE.md when making an official release.)

* New features
  + none

* New optimization
  + none

* New Limitations
  + none

* Update configure options
  + none

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
  + none

* Other updates:
  + none

* Bug fixes
  + Fix a bug when writing a zero-sized request. See a67fb43 and 5acb7e3.
  + Fix a hanging bug when a subset of processes write zero-sized requests.
    See 3f34656.

* New example programs
  + none

* New programs for I/O benchmarks
  + none

* New test program
  + `tests/testcases/null_space.cpp` tests when only rank 0 process writes
    zero-sized request using H5Sselect_none() and others writes non-zero-sized
    amount.

* Conformity with HDF5 library
  + none

* Discrepancy from HDF5 library
  + none

* Issues related to MPI library vendors:
  + none

* Issues related to Darshan library:
  + none

* Clarifications
  + In HDF5, the correct way to do zero-size write is to call select API
    H5Sselect_none() using the dataset's dataspace, instead of
    H5Screate(H5S_NULL). HDF5 reference manual says "Any dataspace specified in
    file_space_id is ignored by the library and the dataset's dataspace is
    always used." The dataset's dataspace is the dataspace used when creating
    the dataset. Once the dataset is created, its dataspace can be set to a
    selected area (e.g. subarray) and passed into H5Dwrite() to write to an
    subbarray space of the dataset occupied in the file. Note that the size of
    mem_type_id must match file_space_id.

