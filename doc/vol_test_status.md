## The Log VOL connector API compatibility

This file contains the current status of the Log VOL connector on the HDF5 [vol-tests](https://github.com/HDFGroup/vol-tests.git).

Library versions:
- HDF5 1.14.2 release
- Log VOL 1.4.0 release
- vol-tests git commit: [996dd87](https://github.com/HDFGroup/vol-tests/tree/996dd87212b2547f1ce638d29b64c8ca436d859c)
- MPICH 4.1.2
- gcc version 8.5.0 20210514
- x86_64-redhat-linux

Date of tests: Oct/9/2023

### Status

|  | Test Suite | PASS | FAIL | SKIP | TOTAL | Pass Rate | Note (Something to check further) |
|---|---|---|---|---|---|---|---|
| 1 | h5vl_test_attribute | 215 | 17 | 2 | 226 | 95.13% | In H5VL_log_attr_specific at H5VL_log_att.cpp:457: function returns -1 |
| 2 | h5vl_test_dataset | 11 | 46 | 13 | 70 | 15.71% | The same file has been opened. The Log VOL connector currently does not support multiple opens. |
| 3 | h5vl_test_datatype | 31 | 6 | 4 | 41 | 75.61% | Couldn't Open File |
| 4 | h5vl_test_file | 0 | 0 | 1 | 1 | 0.00% | Read: received incorrect file intent for read-only file open |
| 5 | h5vl_test_group | 55 | 0 | 0 | 55 | 100.00% | - |
| 6 | h5vl_test_link | 53 | 32 | 6 | 91 | 58.24% | The same file has been opened. The Log VOL connector currently does not support multiple opens. |
| 7 | h5vl_test_misc | 19 | 0 | 2 | 21 | 90.48% | - |
| 8 | h5vl_test_object | 116 | 13 | 10 | 139 | 83.45% | Failure when copy object |
| 9 | h5_test_testhdf5 | - | - | - | - | - | mpi-io driver not enabled |
| 10 | h5vl_test_parallel | - | - | - | - | - | Write: point sel takes long time |
| 11 | h5_partest_t_bigio | - | - | - | - | - | Read: point sel takes long time |
| 12 | h5_partest_t_pshutdown | 1 | 0 | 0 | 1 | 100.00% | - |
| 13 | h5_partest_t_shapesame | 0 | 0 | 1 | 1 | 0.00% | Assertion Failure (Incorrectness) |
| 14 | h5_partest_testphdf5 | 0 | 0 | 1 | 1 | 0.00% | Assertion Failure (Incorrectness) |
|  | SUM | 501 | 114 | 40 | 647 | 77.43% |  |