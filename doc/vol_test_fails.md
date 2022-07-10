## Current Status of Testing with HDF5 VOL Tests

This file summarize the result running [vol-tests](https://github.com/HDFGroup/vol-tests) using the current version of log-layout based VOL.
The test ran on commit 3eb0a434fcede20673a53b0b1cd845f0a30e104b of vol-tests.

The test ran on Ubuntu 20.04.1 LTS with MPICH 3.3.2 and HDF5 1.13.0.
gcc 9.3.0 was used to compile HDF5, log-layout based VOL, and the vol-tests suit.


| Number of cases tested | Passed | Failed |
|------------------------|--------|--------|
| 96                     | 83     | 13     |

### Filed tests

The table below lists all failed tests by the current version of log-layout based VOL.

| Test program     | Category                 | Operation                                                      | Cause                                                                                       | Solution                                              |
|------------------|--------------------------|----------------------------------------------------------------|---------------------------------------------------------------------------------------------|-------------------------------------------------------|
| h5_test_testhdf5 | LOW-LEVEL FILE I/O       | Test file access permissions                                   | Log-based VOL allows only one opened file handle at a time.                                 | Implement support for multiple open to the same file. |
| h5_test_testhdf5 | LOW-LEVEL FILE I/O       | Test opening files in an overlapping manner                    | Log-based VOL allows only one opened file handle at a time.                                 | Implement support for multiple open to the same file. |
| h5_test_testhdf5 | GENERIC OBJECT FUNCTIONS | Test opening root group from two files works properly          | Log-based VOL allows only one opened file handle at a time.                                 | Implement support for multiple open to the same file. |
| h5_test_testhdf5 | GENERIC OBJECT FUNCTIONS | Test opening same group from two files works properly          | Log-based VOL allows only one opened file handle at a time.                                 | Implement support for multiple open to the same file. |
| h5_test_testhdf5 | GENERIC OBJECT FUNCTIONS | Test opening same dataset from two files works properly        | Log-based VOL allows only one opened file handle at a time.                                 | Implement support for multiple open to the same file. |
| h5_test_testhdf5 | GENERIC OBJECT FUNCTIONS | Test opening same named datatype from two files works properly | Log-based VOL allows only one opened file handle at a time.                                 | Implement support for multiple open to the same file. |
| h5_test_testhdf5 | GENERIC OBJECT FUNCTIONS | Test info for objects in the same file                         | Log-based VOL allows only one opened file handle at a time.                                 | Implement support for multiple open to the same file. |
| h5_test_testhdf5 | DATASPACES               | Test basic H5S code                                            | Logvol accepts dataspace without extent  in H5Dwrite/read. It is treated as a NULL request. | Return error if dataspace without extent is detected. |
| h5_test_testhdf5 | DATASPACES               | Test Null dataspace H5S code                                   | Logvol does not handle the case of datasets created with NULL space.                        | Extend internal attribute to represent NULL space.    |
| h5_test_testhdf5 | ATTRIBUTES               | Test opening attributes by name                                | Logvol does not maintain object creation order.                                             | Support *_by_idx APIs.                                |
| h5_test_testhdf5 | ATTRIBUTES               | Test creating attributes by name                               | Logvol does not maintain object creation order.                                             | Support *_by_idx APIs.                                |
| h5_test_testhdf5 | ATTRIBUTES               | Test opening/closing attributes through different file handles | Log-based VOL allows only one opened file handle at a time.                                 | Implement support for multiple open to the same file. |
| h5_test_testhdf5 | ATTRIBUTES               | Test duplicated IDs for dense attribute storage                | Logvol does not support locating objects by index                                           | Support *_by_idx APIs.                                |
|                  |                          |                                                                |                                                                                             |                                                       |

