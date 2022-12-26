# The Log VOL connector User Guide

[HDF5](https://www.hdfgroup.org/solutions/hdf5/) is a file format and an I/O
library designed to handle large and complex data collections. HDF5 provides a
flexible data model and a wide range of functions that allow applications to
organize complex data along with a wide variety of metadata in a hierarchical
way. Due to its convenience, HDF5 is widely adopted in scientific applications
within the HPC community. However, for large number of non-contiguous I/O
requests, achieving a scalable performance can be a challenging task for HDF5.
Despite many of the recent optimization techniques, HDF5 usually struggles to
keep up with the performance of I/O libraries explicitly designed for parallel
I/O, such as [PnetCDF](https://parallel-netcdf.github.io/) and
[ADIOS](https://github.com/ornladios/ADIOS2).

One of the contributing factors to the performance issue is the way HDF5 stores
its datasets in the file.  An HDF5 dataset can be viewed as a multi-dimensional
array of a specific type in an HDF5 file. HDF5 stores the multi-dimensional
arrays in the canonical order, i.e. the row-major order. The implementation
relies on MPI-IO to perform inter-process communication to reorder the data
from multiple MPI processes into the canonical order. As the scale of
supercomputers grows, such communication cost increases as the number of
compute nodes, and can becomes a performance bottleneck for parallel HDF5
applications. 

## Design of the Log VOL connector

The [the Log VOL connector](https://github.com/DataLib-ECP/vol-log-based) is an HDF5
Virtual Object Layer (VOL) plug-in that stores HDF5 datasets in a log-based
storage layout, in contrast to the canonical order layout. The
[Log-based storage layout](https://link.springer.com/chapter/10.1007/978-3-540-75416-9_34)
has been known to be efficient to write, and less susceptible to complex I/O
patterns. Instead of arranging the data according to its logical location, it
simply stores the data as-is. Metadata describing the logical location is also
stored so that the data can be gathered when we need to read the data. In this
design, the expensive overhead of rearranging the data among processes is
deferred to the reading stage. For applications that produce checkpoints for
restarting purposes, the overhead can be totally avoided unless the program is
actually interrupted. Log-based storage layout is an effective way for
applications that care less about reading performance to trade read performance
for write performance. Our design principle of the Log VOL connector is described in
[doc/design.md](design.md).

## Documents
* [doc/INSTALL.md](INSTALL.md) contains the compile and run instructions.
* [doc/api.md](api.md) describes the new APIs introduced in this VOL.
* [doc/usage.md](usage.md) shows examples of how to enable the Log VOL connector
  through the two VOL environment variables without changing the applications.
  It also describes a way to explicitly use the Log VOL connector by editing the
  application source codes.

### Current limitations
  + Read operations:
    + Reading is implemented by searching through log records to find
      the log blocks intersecting with the read request.
    + The searching requires to read the entire log metadata into the memory.
  + Async I/O (a new feature of HDF5 in the future release) is not yet supported.
  + Virtual Datasets (VDS) feature is not supported.
  + Multiple opened instances of the same file is not supported.
    + the Log VOL connector caches some metadata of an opened file.
      The cached metadata is not synced among opened instances.
    + The file can be corrupted if the application open and operate multiple handles to the same file.
  + The names of (links to) objects cannot start with '_' character.
    + Names starting with '_' are reserved for the Log VOL connector for its internal data and metadata.
  + The Log VOL connector does not support all the HDF5 APIs.
    See [doc/compatibility.md](compatibility.md) for a full list of supported and unsupported APIs.
  + All opened objects of a file must be closed before the file is closed.
  + The Log VOL connector does not recognize files written by the native VOL.
    + The native VOL connector can read the Log VOL connector output files, but not vice-versa.
  + In H5Dwrite, the dataspace indicated by file_space_id must have the same dimensions as the dataset's dataspace.
    + file_space_id can be a selection, e.g., a subarray of the dataset's dataspace.
    + If a zero-sized request is indicated by a null dataspace, users must call H5Sselect_none to set a zero-sized selection of the dataspace.
  + Object must be located by (link's) name directly.
    + Logvol does not support accessing objects by index or token.
      + All *by_idx and *by_token APIs are not supported.
  + Datasets with a variable-length datatype are not supported.
    + Logvol will return an error if users create a dataset using variable-length datatype.
  
### HDF5 VOL Connector ID
* This the Log VOL connector has been registered with the HDF group with
  [Connector Identifier 514](https://portal.hdfgroup.org/display/support/Registered+VOL+Connectors).

## References
* [HDF5 VOL application developer manual (PDF)](https://github.com/HDFGroup/hdf5doc/raw/vol_docs/RFCs/HDF5/VOL/user_guide/vol_user_guide.pdf)
* [HDF5 VOL plug-in developer manual (PDF)](https://github.com/HDFGroup/hdf5doc/raw/vol_docs/RFCs/HDF5/VOL/connector_author_guide/vol_connector_author_guide.pdf)
* [HDF5 VOL RFC (PDF)](https://github.com/HDFGroup/hdf5doc/raw/vol_docs/RFCs/HDF5/VOL/RFC/RFC_VOL.pdf)

## Project funding supports:
Ongoing development and maintenance of the Log VOL connector are supported by the
Exascale Computing Project (17-SC-20-SC), a joint project of the U.S.
Department of Energy's Office of Science and National Nuclear Security
Administration, responsible for delivering a capable exascale ecosystem,
including software, applications, and hardware technology, to support the
nation's exascale computing imperative.

