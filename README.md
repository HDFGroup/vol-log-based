## Log-based VOL - an HDF5 VOL Plugin that stores HDF5 datasets in a log-based storage layout

This software repository contains source codes implementing an [HDF5](https://www.hdfgroup.org) Virtual Object Layer ([VOL](https://portal.hdfgroup.org/display/HDF5/Virtual+Object+Layer)) plugin that stores HDF5 datasets in a log-based storage layout. In the log-based layout, data of multiple write requests made by an MPI process are appended one after another in the file. Such I/O strategy can avoid the expensive inter-process communication and I/O serialization due to file lock contentions when storing data in the canonical order. Files created by this VOL conform with the HDF5 file format, but require this VOL to read them back. Through the log-based VOL, exist HDF5 programs can achieve a better parallel write performance with minimal changes to their codes.

### HDF5 VOL Connector ID
* This log-based VOL has been registered with the HDF group with [Connector Identifier 514](https://portal.hdfgroup.org/display/support/Registered+VOL+Connectors).
 
### Build Instructions
* See [doc/INSTALL.md](doc/INSTALL.md)

### Current limitations
  + It does not support NetCDF4 programs that write in the HDF5 file format.
  + Read operations:
    + Reading is implemented by naively searching through log records to find
      the log blocks intersecting with the read request.
    + The searching requires to read the entire metadata of the file into the memory.
  + The subfiling feature is under development.
  + Async I/O (a new feature of HDF5 in the future release) is not yet supported.
  + Virtual Datasets (VDS) feature is not supported.
  + Multiple opened instances of the same file is not supported.
    + The log-based VOL caches some metadata of an opened file.
      The cached metadata is not synced among opened instances.
    + The file can be corrupted if the application open and operate multiple handles to the same file.
  + The names of (links to) objects cannot start with '_' character.
    + Names starting with '_' are reserved for the log-based VOL for its internal data and metadata.
  + The log-based VOL does not support all the HDF5 APIs.
    See [doc/compatibility.md](doc/compatibility.md) for a full list of supported and unsupported APIs.
  + All opened objects of a file must be closed before the file is closed.
  

### References
* [HDF5 VOL application developer manual](https://github.com/HDFGroup/hdf5doc/raw/vol_docs/RFCs/HDF5/VOL/user_guide/vol_user_guide.pdf)
* [HDF5 VOL plug-in developer manual](https://github.com/HDFGroup/hdf5doc/raw/vol_docs/RFCs/HDF5/VOL/connector_author_guide/vol_connector_author_guide.pdf)
* [HDF5 VOL RFC](https://github.com/HDFGroup/hdf5doc/raw/vol_docs/RFCs/HDF5/VOL/RFC/RFC_VOL.pdf)

### Project funding supports:
Ongoing development and maintenance of Log-based VOL are supported by the Exascale Computing Project (17-SC-20-SC), a joint project of the U.S. Department of Energy's Office of Science and National Nuclear Security Administration, responsible for delivering a capable exascale ecosystem, including software, applications, and hardware technology, to support the nation's exascale computing imperative.
