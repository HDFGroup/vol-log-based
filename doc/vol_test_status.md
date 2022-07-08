## Log-based VOL API compatibility

This file contains the current status of log-based VOL on the official hdf5 vol test suit.

The test ran on Ubuntu 20.04.1 LTS with MPICH 3.3.2 and HDF5 1.13.0.
gcc 9.3.0 was used to compile HDF5, log-layout based VOL, and the vol-tests suit.

### Status
* Pass
  + vol-test suit reports no error in the section
* Fail
  + vol-test suit reports at least one error in the section
* Skip
  + Test section being skipped manually due to features not supported by log-based VOL
* Not tested
  + Sections not yet tested

### h5_test_testhdf5
* LOW-LEVEL FILE I/O

| Test case                                                         | Status | Note                                                        |
|-------------------------------------------------------------------|--------|-------------------------------------------------------------|
| Test file creation(also creation templates)                       | Pass   |                                                             |
| Test file opening                                                 | Pass   |                                                             |
| Test file reopening                                               | Pass   |                                                             |
| Test file close behavior                                          | Pass   |                                                             |
| Test H5Iget_file_id                                               | Pass   |                                                             |
| Test H5Fget_obj_ids for Jira Issue 8528                           | Pass   |                                                             |
| Test file access permissions                                      | Fail   | Log-based VOL allows only one opened file handle at a time. |
| Test file access permission again                                 | Pass   |                                                             |
| Test detecting HDF5 files correctly                               | Pass   |                                                             |
| Test H5Fdelete                                                    | Pass   |                                                             |
| Test opening objects with "." for a name                          | Pass   |                                                             |
| Test opening files in an overlapping manner                       | Fail   | Log-based VOL allows only one opened file handle at a time. |
| Test basic H5Fget_name() functionality                            | Pass   |                                                             |
| Test opening root group from two files works properly             | Fail   | Log-based VOL allows only one opened file handle at a time. |
| Test opening same group from two files works properly             | Fail   | Log-based VOL allows only one opened file handle at a time. |
| Test opening same dataset from two files works properly           | Fail   | Log-based VOL allows only one opened file handle at a time. |
| Test opening same named datatype from two files works properly    | Fail   | Log-based VOL allows only one opened file handle at a time. |

* GENERIC OBJECT FUNCTIONS

| Test cases                                         | Status | Note                                                        |
|----------------------------------------------------|--------|-------------------------------------------------------------|
| Test generic open function                         | Pass   |                                                             |
| Test opening objects by token                      | Pass   |                                                             |
| Test generic close function                        | Pass   |                                                             |
| Test incrementing and decrementing reference count | Pass   |                                                             |
| Test object creation properties                    | Pass   |                                                             |
| Test object link routine                           | Pass   |                                                             |
| Test info for objects in the same file             | Fail   | Log-based VOL allows only one opened file handle at a time. |

* DATASPACES

| Test cases                                          | Status | Note                                                                                       |
|-----------------------------------------------------|--------|--------------------------------------------------------------------------------------------|
| Test basic H5S code                                 | Fail   | Logvol accepts dataspace without extend in H5Dwrite/read. It is treated as a NULL request. |
| Test Null dataspace H5S code                        | Fail   | Logvol does not handle the case of datasets created with NULL space.                       |
| Test dataspace with zero dimension size             | Pass   |                                                                                            |
| Test encoding and decoding                          | Pass   |                                                                                            |
| Test encoding regular hyperslabs                    | Pass   |                                                                                            |
| Test encoding irregular hyperslabs                  | Pass   |                                                                                            |
| Test encoding points                                | Pass   |                                                                                            |
| Test version 2 hyperslab encoding length is correct | Pass   |                                                                                            |
| Test operations with old API routine (H5Sencode1)   | Pass   |                                                                                            |
| Test scalar H5S writing code                        | Pass   |                                                                                            |
| Test scalar H5S reading code                        | Pass   |                                                                                            |
| Test compound datatype scalar H5S writing code      | Pass   |                                                                                            |
| Test compound datatype scalar H5S reading code      | Pass   |                                                                                            |
| Exercise bug fix for chunked I/O                    | Pass   |                                                                                            |
| Test extent comparison code                         | Pass   |                                                                                            |
| Test extent copy code                               | Pass   |                                                                                            |
| Test bug in offset initialization                   | Pass   |                                                                                            |
| Test version bounds with dataspace                  | Pass   |                                                                                            |

* DATASPACE COORDINATES
  + All tests passed.

* ATTRIBUTES

| Test cases                                                                          | Status | Note                                                        |
|-------------------------------------------------------------------------------------|--------|-------------------------------------------------------------|
| Test basic H5A writing code                                                         | Pass   |                                                             |
| Test basic H5A reading code                                                         | Pass   |                                                             |
| Test H5A I/O in the presence of H5Fflush calls                                      | Pass   |                                                             |
| Test attribute property lists                                                       | Pass   |                                                             |
| Test complex datatype H5A writing code                                              | Pass   |                                                             |
| Test complex datatype H5A reading code                                              | Pass   |                                                             |
| Test scalar dataspace H5A writing code                                              | Pass   |                                                             |
| Test scalar dataspace H5A reading code                                              | Pass   |                                                             |
| Test H5A writing code for multiple attributes                                       | Pass   |                                                             |
| Test H5A reading code for multiple attributes                                       | Pass   |                                                             |
| Test H5A iterator code                                                              | Pass   |                                                             |
| Test H5A code for deleting attributes                                               | Pass   |                                                             |
| Test using shared dataypes in attributes                                            | Pass   |                                                             |
| Test storing big attribute                                                          | Pass   |                                                             |
| Test storing attribute with NULL dataspace                                          | Pass   |                                                             |
| Test deprecated API routines                                                        | Pass   |                                                             |
| Test storing lots of attributes                                                     | Pass   |                                                             |
| Test passing a NULL attribute info pointer to H5Aget_info(_by_name/_by_idx)         | Pass   |                                                             |
| Test passing a NULL or empty attribute name to H5Arename(_by_name)                  | Pass   |                                                             |
| Test passing NULL buffer to H5Aget_name(_by_idx)                                    | Pass   |                                                             |
| Test querying attribute info by index                                               | Skip   | Logvol does not support locating objects by index           |
| Test deleting attribute by index                                                    | Skip   | Logvol does not support locating objects by index           |
| Test iterating over attributes by index                                             | Skip   | Logvol does not support locating objects by index           |
| Test opening attributes by index                                                    | Skip   | Logvol does not support locating objects by index           |
| Test opening attributes by name                                                     | Fail   | Logvol does not maintain object creation order              |
| Test creating attributes by name                                                    | Fail   | Logvol does not maintain object creation order              |
| Test odd allocation operations                                                      | Pass   |                                                             |
| Test many deleted attributes                                                        | Pass   |                                                             |
| Test "self referential" attributes                                                  | Pass   |                                                             |
| Test attributes on named datatypes                                                  | Pass   |                                                             |
| Test opening/closing attributes through different file handles                      | Fail   | Log-based VOL allows only one opened file handle at a time. |
| Test reading empty attribute                                                        | Pass   |                                                             |
| Test attribute expanding object header with undecoded messages                      | Pass   |                                                             |
| Test large attributes converting to dense storage                                   | Pass   |                                                             |
| Test dense attribute storage creation                                               | Pass   |                                                             |
| Test opening attributes in dense storage                                            | Pass   |                                                             |
| Test deleting attributes in dense storage                                           | Pass   |                                                             |
| Test renaming attributes in dense storage                                           | Pass   |                                                             |
| Test unlinking object with attributes in dense storage                              | Pass   |                                                             |
| Test dense attribute storage limits                                                 | Pass   |                                                             |
| Test duplicated IDs for dense attribute storage                                     | Fail   | Logvol does not support locating objects by index           |
| Test creating an object w/attribute creation order info                             | Pass   |                                                             |
| Test compact attribute storage on an object w/attribute creation order info         | Pass   |                                                             |
| Test dense attribute storage on an object w/attribute creation order info           | Pass   |                                                             |
| Test creating attributes w/reopening file from using new format to using old format | Pass   |                                                             |
| Test attribute storage transitions on an object w/attribute creation order info     | Pass   |                                                             |
| Test deleting object using dense storage w/attribute creation order info            | Pass   |                                                             |
| Test writing to shared attributes in compact & dense storage                        | Pass   |                                                             |
| Test renaming shared attributes in compact & dense storage                          | Pass   |                                                             |
| Test deleting shared attributes in compact & dense storage                          | Pass   |                                                             |
| Test unlinking object with shared attributes in compact & dense storage             | Pass    
|                                                             |

* SELECTIONS
  + Not tested
* TIME DATATYPES 
  + Not tested
* REFERENCES
  + Not tested
* VARIABLE-LENGTH DATATYPES
  + Not tested
* VARIABLE-LENGTH STRINGS
  + Not tested
* GROUP & ATTRIBUTE ITERATION
  + Not tested
* ARRAY DATATYPES
  + Not tested
* GENERIC PROPERTIES
  + Not tested
* UTF-8 ENCODING
  + Not tested
* USER-CREATED IDENTIFIERS
  + Not tested
* MISCELLANEOUS
  + Not tested

