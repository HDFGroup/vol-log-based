# Log-based VOL User Guide

## Prolog

HDF5 is a file format and an I/O library designed to handle large and complex data collections \cite{folk1999hdf5}.
HDF5 provides a flexible data model and a wide range of functions that allow applications to organize complex data along with a wide variety of metadata in a hierarchical way.
Due to its convenience, HDF5 is widely adopted in scientific applications within the HPC community.
However, the performance of HDF5 can suffer when the I/O pattern is complex.
Despite the implementation of many optimization techniques, HDF5 usually struggles to keep up with the performance of I/O libraries explicitly designed for parallel I/O, such as PnetCDF and ADIOS.

One of the contributing factors to the performance issue is the way HDF5 stores its datasets in the file.
An HDF5 dataset can be viewed as a multi-dimensional array of a specific type in an HDF5 file.
By default, HDF5 stores the data in a contiguous layout in which the data are flattened into the file space according to canonical order.
It requires MPI-IO, the I/O middleware used by HDF5 for parallel file access, to perform inter-process communication to reorder the data into canonical order. 
As the scale of supercomputers grows, the communication cost is likely to increase and eventually becomes a performance bottleneck for parallel HDF5 applications. 

## What is log-based VOL ?

The log-based VOL is a HDF5 Virtual Object Layer (VOL) plug-in that stores HDF5 datasets in a log-based storage layout
Data in a log-based storage layout is known to be efficient to write, and less susceptible to complex I/O patterns \cite{kimpe2007transparent}.
Instead of arranging the data according to its logical location, it simply stores the data as-is.
Metadata describing the logical location is also stored so that the data can be gathered when we need to read the data.
In this way, the expensive overhead of rearranging the data is deferred to the reading stage.
For applications that produce checkpoints for restarting purposes, the overhead can be totally avoided unless the program is actually interrupted.
Log-based storage layout is an effective way for applications that care less about reading performance to trade read performance for write performance.

## How does it work ?
* For a detailed description of the design of log-based VOL, see [doc/design.md](doc/design.md)

## Installing log-base VOL
* See [doc/INSTALL.md](doc/INSTALL.md)

## Using the log-base VOL
* See [doc/usage.md](doc/usage.md)

## Misc
### HDF5 VOL Connector ID
* This log-based VOL has been registered with the HDF group with [Connector Identifier 514](https://portal.hdfgroup.org/display/support/Registered+VOL+Connectors).

## References
* [HDF5 VOL application developer manual](https://github.com/HDFGroup/hdf5doc/raw/vol_docs/RFCs/HDF5/VOL/user_guide/vol_user_guide.pdf)
* [HDF5 VOL plug-in developer manual](https://github.com/HDFGroup/hdf5doc/raw/vol_docs/RFCs/HDF5/VOL/connector_author_guide/vol_connector_author_guide.pdf)
* [HDF5 VOL RFC](https://github.com/HDFGroup/hdf5doc/raw/vol_docs/RFCs/HDF5/VOL/RFC/RFC_VOL.pdf)

## Project funding supports:
Ongoing development and maintenance of Log-based VOL are supported by the Exascale Computing Project (17-SC-20-SC), a joint project of the U.S. Department of Energy's Office of Science and National Nuclear Security Administration, responsible for delivering a capable exascale ecosystem, including software, applications, and hardware technology, to support the nation's exascale computing imperative.
