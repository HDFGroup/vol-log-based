## Design of log-based VOL

Our design principle for the log-based driver is to store I/O requests contiguously in files, like logs appending one after another.
Such strategy avoids the potentially high costs of inter-process communication required if data is to be stored in a canonical order.
The log-based driver includes mechanisms to keep track of the metadata of individual logged requests and manage them for other requests.
The driver will be developed separately from the HDF5 source tree and compiled into a stand-alone library file.
The library can then be linked together with the native HDF5 library when building a user program.
Once the library is linked and registered by the user programs, the HDF5 VOL intercepts a subset of relevant I/O functions and redirects them to the log-based driver.
The design of relevant I/O functions will be described with further details in later sections.

The files created by the log-based driver will be valid HDF5 files.
They will conform with HDF5 file specification and thus can be recognized by existing HDF5 programs.
However, the high-level structures of objects stored in the file may not be the same as traditional HDF5 files.
The log-based driver will be required to reconstruct the metadata into the views understood by the HDF5 conventions.
Auxiliary data objects will be created to support the log layout, which will be stored as regular HDF5 objects, such as datasets and attributes.
These auxiliary data objects will be stored in a special group and accessed through the native HDF5 functions.

In order to increase the performance for write operations, we adjust the semantics of write APIs to be nonblocking and require users to call a flush API explicitly to commit the requests to files.
This adjustment allows the log-based driver to aggregate small requests into large ones for better I/O bandwidths and reduce frequency of inter-process communication cost when flushing data in parallel.

### Internal Representation of Dataset Objects
In the log-based driver, a traditional HDF5 dataset is represented by a scalar dataset with its dimension information stored as the scalar variable’s attributes.
We refer to this scalar dataset as the “anchor dataset”.
The contents of a dataset is stored in a 1D dataset of unsigned byte type, referred as the “log dataset”.
Each time a write request is made to the dataset, the request contents are appended to the end of log dataset as a contiguous block.
The space of log dataset is shared by all datasets and stores data following the same timely sequence of write requests made by the application.
A unique internal ID is assigned to each traditional dataset in order to identify all its requests stored in the log dataset.

Ideally, the log dataset is expandable in size and to be stored contiguously in file for best write performance.
In HDF5, a dataset to be stored in a contiguous space in files must be defined with fixed dimension sizes.
On the other hand, an expandable dataset must use the chunked storage layout which does not guarantee chunks are stored contiguously in files.
Due to this limitation, the log datasets in our design are defined as fixed-size datasets.
A log dataset is created when file flush is called.
Without using data chunking, this allows us to avoid potentially expensive B-tree metadata operations, an indexing data structure employed by HDF5 to keep track of the locations of chunks in files.

A new group is created at the root level to store all auxiliary data objects created by our log-based driver.
The group is referred as “log group”.
The most important members of this group are the log datasets, which are one-dimensional arrays of type H5T_STD_U8LE (unsigned 8-bit type in Little Endian format).
Log datasets store only the write request data. Other new datasets are created in the log group are for storing the log metadata.

### The log data structure
The log is made up of two type of logs: a metadata log, and a data log.
The data log stores the data of dataset write operations while the metadata log stores other information describing the operation.
Separating data and metadata allows the log-based driver to search through metadata entries without the need to do file seeks when handling a read request.
A record in the log is a pair of an entry in the data log and an entry in the metadata log.
It represents a single write operation to a dataset.

A metadata log entry includes: (1) the ID of the dataset involved in the operation; (2) the selection in the dataset data space; (3) the offset of the data in the file; (4) the size of the data in the file; (5) the size of the metadata entry; and (6) flags that indicates the endianceness of the entry and other informations for future extension.
The dataset dataspace selection is stored as a list of hyper-slabs (subarraies).
If the selection has an iregular shape, the log-based VOL decomposes it into disjoint hyper-slabs.
Element selections are treated as hyper-slab selections with unit sized hyper-slabs. 

A data log entry contains the data of the part of the dataset selected in the operation.
If the dataset space selection contains multiple hyper-slabs, the data is ordered according to the order of the hyper-slabs in the corresponding metadata log entry.
That is, the data of the first hyper-slab, followed by the data of the second hyper-slab.
If the memory space selection is not contiguous, the data is packed into a contiguous buffer.
If the memory data type does not match the dataset data type, the log-based VOL converts the data into to the type of the target dataset.
After type conversion, the data went through the filter pipeline associates to the dataset before being written to the data log.

### Writing to datasets

In HDF5, users can specify the way how a dataset is written to the file, by using an HDF5 data transfer property.
For example, predefined constants H5FD_MPIO_COLLECTIVE or H5FD_MPIO_INDEPENDENT can be used to tell HDF5 to use MPI collective or independent I/O functions underneath to transfer data to the file system.
Users can also create their own property class through API H5Pregister2 to define customized properties.

Performance of our log-based driver can be significantly enhanced if the multiple small I/O requests can be aggregated into fewer, large request.
There are two approaches to achieve the effect of request aggregation.
One is to buffer the write requests internally and later the buffered requests are aggregated and flushed together.
This approach allows users to re-use their I/O buffers immediately after the write requests return.
The side effect is the increasing memory footprint.
The other approach is non-blocking I/O.
A new HDF5 data transfer property is defined to introduce a new I/O semantics that limit users from altering the buffer contents before the pending requests are flushed to the file system.
Such non-blocking APIs appear in MPI and PnetCDF.

For each write request, the log driver copies the data into a temporary allocated buffer.
This internal buffer will be freed after the request is flushed to the file.
Once the write request API H5Dwrite returns, users are free to modify the buffer contents.
Users are noted that the write data can only be considered secured (in disk) after the flush API is called.
The space limit of the internal buffers can be set by users through the file access property list.
The space limit is shared by all the datasets defined in the file.
When the limit is reached, the write API returns an error with a message properly defined through HDF5 error handling mechanism.
The error can be used to determine whether to call flush APIs.
It is the user’s responsibility to ensure the set buffer size is large enough for their need.
In our design data flushing is only performed in file flush and close APIs.
File flush and close APIs are collective because log metadata must be synchronized and thus requires the participation of all processes.

The size of the buffer used by the log driver to cache data can be configured by the user in the file access property list.
Space is shared by all datasets in the file.
If there is not enough space to cache incoming data, the write operation will fail.
It is the user’s responsibility to ensure the set buffer size is large enough for their need.
For now, we do not consider flushing the buffer automatically because it will require participation from all processes and involves additional communication overhead.

As for the non-blocking approach, a new data transfer property class is introduced for nonblocking I/O and users can call property set API to indicate a call to H5Dwrite is non-blocking.
When non-blocking transfer property is used, API H5Dwrite acts as an API that posts a pending request.
All pending write requests are later flushed by an explicit call to file flush API.
Users are required to keep the contents of write buffers unchanged until the flush is called. The metadata of a pending write request includes dataset ID, hyperslab selection, pointer to the user buffer, etc.
Metadata of multiple write requests will be used to aggregate the requests during the file flush call.

### Flushing non-blocking requests

Flushing the pending write requests is a collective operation.
Each process first calculates the aggregated size of all locally pending requests.
A call to MPI_Exscan by all processes can obtain the starting offsets to the log dataset for each process.
The starting offsets are used to create a hyperslab selection, which is later used when calling H5Dwrite.
Another call to MPI_Allreduce is also required to obtain the total size of write size across all processes.
The size is used to define the size of the log dataset.

A new log dataset is created under the log group each time file flush is called.
The names of log datasets will include a sequence number representing the timely order of their creation.

Writing the log datasets to file is carried out by calling MPI_File_write at the file offset of the log dataset. All writes are collective.

Writing the metadata tables to file is delayed until a read request is made or when the file is closing.
Log metadata of all flushed requests is kept in the memory, so it can be used to construct the tables right before written to the file. Each entry in the log metadata contains the information about log dataset ID and offset, so individual write requests can be uniquely identified.

The format of the metadata table is specified in [doc/metadata_format.md](./metadata_format.md).

### Reading from datasets in log-based storage layout

Reading in a log storage layout is inherently inefficient because the data is completely unorganized.
In the log storage layout, a dataset is described by a set of log entries scattered through the data log.
Reading the data is like searching the log for related entries and collect pieces of data that need to be read.
The pieces are then stitched together to reconstruct the read data.
The complexity increases as more log entries are appended to the log.

To read a dataset, the log driver searches the metadata tables for any logged requests that intersect with the hyperslab selection of the read request.
For each intersected log, the entire log data is read into a buffer and then the intersected part is copied over to the user buffer.
During the intersection check, the metadata log entries are visited in the order of their creation so that the requests written in the later time will overwrite the previous ones.
This design essentially follows the sequential consistency semantics used in POSIX I/O.
If the file is opened in read/write mode, a call to flush any pending write requests is required prior to the read to ensure data consistency among processes.
The flow chart below depicts the steps the log driver handle read requests.
