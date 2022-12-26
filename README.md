## Log-layout based VOL connector - an HDF5 VOL Plugin for storing datasets in a log-based layout

This software repository contains source codes implementing Log-layout based VOL connector, an
[HDF5](https://www.hdfgroup.org) Virtual Object Layer
([VOL](https://portal.hdfgroup.org/display/HDF5/Virtual+Object+Layer)) plugin
that stores HDF5 datasets in a log-based storage layout in files. When using
this VOL connector (later referred to as the Log VOL connector), multiple write
requests from an MPI process are appended one after
another (as logs) and stored in a contiguous block in the file. The contiguous
blocks of multiple processes are appended one another following the processes'
MPI rank order. Such log-based I/O strategy avoids the expensive inter-process
communication which is required when storing data in the canonical order.
Thus, exist HDF5 programs, using the Log VOL connector, can achieve a better
parallel write performance with no changes to their codes. Files created by
this VOL conform with the HDF5 file format, but require this VOL to read them
back.

* Current build status:
  [![Ubuntu_mpich](https://github.com/DataLib-ECP/vol-log-based/actions/workflows/ubuntu_mpich.yml/badge.svg)](https://github.com/DataLib-ECP/vol-log-based/actions/workflows/ubuntu_mpich.yml)

### HDF5 VOL Connector ID
* This Log VOL connector has been registered with the HDF group with
  [Connector Identifier 514](https://portal.hdfgroup.org/display/support/Registered+VOL+Connectors).
 
### Documents
* [doc/userguide.md](doc/userguide.md) contains the compile and run instructions.
* [doc/design.md](doc/design.md) outlines the design of Log VOL connector.
* [doc/api.md](doc/api.md) describes the new APIs introduced in this VOL.

### Applicaton Case Studies and Experimental Results
* [E3SM I/O case study](case_studies/E3SM_IO.md) - [Energy Exascale Earth System Model (E3SM)](https://github.com/wrf-model/WRF).
* [WRF I/O case study](case_studies/WRF.md) - [Weather Research and Forecasting Model (WRF)](https://github.com/wrf-model/WRF).

### Developers
* Wei-keng Liao <<wkliao@northwestern.edu>>
* Kai-yuan Hou <<kai-yuanhou2020@u.northwestern.edu>>
* Zanhua Huang <<zanhua@u.northwestern.edu>>

Copyright (C) 2022, Northwestern University.
See [COPYRIGHT](COPYRIGHT) notice in top-level directory.

### Project funding supports:
Ongoing development and maintenance of the Log VOL connector are supported by the
Exascale Computing Project (17-SC-20-SC), a joint project of the U.S.
Department of Energy's Office of Science and National Nuclear Security
Administration, responsible for delivering a capable exascale ecosystem,
including software, applications, and hardware technology, to support the
nation's exascale computing imperative.

