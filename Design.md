Design of log VOL using HDF5 based file format

### Table of contents
- [Introduction](#Introduction)
- [Background](#Background)
    + [Log-based storage layout](#Log-based-storage-layout)
    + [I/O library that support log-based storage layout](#I/O-library-that-support-log-based-storage-layout)
- [VOL design](#VOL-design)
- [VOL behavior](#VOL-behavior)
    + [File create](#File-create)
    + [File open](#File-open)
    + [File close](#File-close)
    + [Dataset create](#Dataset-create)
    + [Dataset open](#Dataset-open)
    + [Dataset dataspace query](#Dataset-dataspace-query)
    + [Dataset close](#Dataset-close)
    + [Dataset write](#Dataset-write)
    + [Dataset (file) flush](#Dataset-(file)-flush)
    + [Dataset read](#Dataset-read)
    + [Construct the index](#Construct-the-index)
- [Reference](#Reference)
- [Appendix](#Appendix)
- [Log data format](#Log-data-format)
- [Index format](#Index-format)
---

## Introduction

HDF5 is a file format and an I/O library designed to handle large and complex data collections [1].
HDF5 provides a wide range of functions that allows applications to organize complex data along with a wide variety of metadata in a hierarchical way.
Due to its convenience and flexibility, HDF5 is widely adopted in scientific applications within the HPC community.
However, slow parallel I/O performance is being observed through many applications.
Despite many optimizations being introduced, HDF5 struggles to keep up with I/O libraries designed specifically for parallel I/O performance on complex I/O patterns.

The cause of the performance difference lies within HDF5's flexible file format to support a wide range of use cases. % A better phase of "use cases"?
One of them is the storage layout.
HDF5 represents data as dataset objects.
An HDF5 dataset can be viewed as a multi-dimensional array of a certain type in an HDF5 file.
HDF5 arranges the data of datasets in the file according to their canonical order within the dataset.
As a result, a single write operation may need to write to multiple locations in the file.
Writing to discontiguous locations is less efficient compared to writing to contiguous space on most modern file systems.
It also increases the overhead of parallel I/O by introducing more metadata to synchronize among processes.

Log-based storage layout is shown to be efficient for writing even on complex I/O patterns [2].
Instead of arranging the data according to its logical location, it simply stores the data as-is.
Metadata describing the logical location is also stored so that the data can be gathered when we need to read the data.
In this way, the overhead of arranging the data is delayed to the reading time.
For applications that produce checkpoints for restarting purposes, the file written will not be read unless there are anomalies.
Trading read performance for write performance becomes an effective strategy to boost overall I/O performance.

In this report, we present a design of HDF5 Virtual Object Layer (VOL) called log VOL to enable log-based storage layout for HDF5 dataset.
We present a file format that implement log-styled layout using HDF5 data objects.
We present our method to perform parallel I/O on datasets in log-based layout.
We will discuss design choices we made from several aspect and the reason behind.
---

## Background

### Log-based storage layout

Unlike traditional storage layouts, a log-baseed storage layout do not store the data of a dataset directly.
Instead, it records all I/O operations that modifies the dataset in a log-based format.
In this work, we refer to the data structure used to store records the "log".
In the log, records (entries) append one after another.
Each record in the log contains the data as well as the metadata that describes an I/O operation.
Some design store metadata and data together while others store them separately.

A major advantage of using log-baseed storage layout is write performance.
Writing to a dataset in log-based storage layout involves merely appending a record to the log.
In this way, we always write to a contiguous region regardless of the I/O pattern at application level.
Also there is no need to seek before each write.
This type of I/O pattern is known be efficient on most of the file systems.

Contrary to writing, reading a dataset in log-based storage layout is not that straight forward.
The I/O library must search through the records for pieces of data that intersect with the selection.
The pieces is then stiches together to reconstruct the data within the selection.
Some design incorporate an index of records to speed up the search.
Reading data in log-based storage layout seems to be inefficient since a single selection can intersects many records.
However, Polte, Milo, et al showed that such senario seldom happens [3].
In many cases, the read pattern is the same as write, resulting in I/O patterns more efficient than that reading from dataset in contiguous storage layout.

### I/O library that support log-based storage layout

Due too advantages mentioned above, log-based torage layout has been emploied by many I/O libraries and middlewares.
Kimpe et al. introduced an I/O middleware called LogFS that records low-level I/O operation in a temporary log and then reconstruct the data when the process closes the file [2].

## VOL design

We designed an HDF5 VOL called log VOL that stores HDF5 datasets in a log-based storage layout.
The log VOL is implemented in a separate library that exposes an HDF5 VOL instant that the application can use to create HDF5 files that store datasets in a log-based storage layout.
The log VOL is built on top of the native VOL (the VOL for native HDF5 format).
It intercepts operations that require special treatment to support the log-based storage layout on datasets while passing other operations directly to the native VOL.
We use HDF5 data objects to implement all data structures used by the log VOL.
We use the native VOL to access those data structures.
We place those special HDF5 data objects used by the VOL in a special group.
The log VOL hides those data objects from the user application.
The file generated by the log VOL is a valid HDF5 file except that the content is only understandable by the log VOL.
To support data aggregation, we changed the semantics of the dataset write function slightly.
Calling H5Dwrite does not immediately write the data to the file; instead, it only stages the I/O request within the log VOL.
We perform actual write when the file is closed or when requested by the application.
The application must make sure the data buffer is not modified before requests are flushed to the file.

## VOL architecture 

### Dataset in log VOL

The VOL represents a dataset using a special type of HDF5 dataset called the anchor dataset.
The anchor dataset has the same name as the dataset it represents.
Operations involving the dataset that do not need special treatment form the log VOL is performed on the anchor dataset by the native VOL.
The data of the dataset is stored elsewhere in a log-based data structure.
To conserve space, anchor datasets are declared as scalar datasets.
The original dimensionality and shape are stored as attributes of the anchor dataset for querying.
Datasets are assigned a unique ID to identify them in the log-based structure.

### Special datasets for log entries

We implement the log using HDF5 datasets.
For best write performance, the data structure of the log should be both contiguous in file space and expandable in size.
Unfortunately, none of the HDF5 datasets satisfy all the property.
In HDF5, datasets in contiguous storage layout must have a fixed size.
Expandable datasets must be stored in a chunked storage layout that is not contiguous in the file.
For several reasons, we use fixed-sized datasets to store records.
First, writing to a contiguous dataset is more efficient than having to look up and to seek to the position of each chunk.
Also, creating a contiguous dataset involves less metadata overhead than creating a chunked datasets dataset.
Moreover, both chunks and data objects are indexed by a b-tree in HDF5.
As a result, expanding chunked datasets (adding chunks) may not be much cheaper than creating datasets.
Finally, a contiguous dataset makes it easier to implement optimizations outside HDF5, as discussed in later subsections.

The log is made up of multiple special fixed-size datasets referred to as log datasets.
They are 1-dimensional datasets of type byte (H5T\_STD\_U8LE) we create when we need space to record I/O operations.
The log dataset always locates in the LOG group with special prefix in the name to distinguish from other datasets.
We estimate the space required based on the space we used so far and the portion of datasets being written.
There is no header in the log, records in log datasets simply append one after another.
Within each record are metadata describing one write operation followed by the data to write.
The metadata includes dataset ID, selection, the location of the data, the endianness of the data, and filters (compression ... etc.) applied to the data.
The selection can be a hyper-slab (subarray) denoted by start position and length along each dimension or a point list (list of points) denoted by the position of each cell selected.

### The index table and the skip list

To allow more efficient searching of records, we construct an index table of all records and save it into a dataset.
The dataset storing the index table is a 1-dimensional dataset of type BYTE (H5T\_STD\_U8LE) referred to as the index dataset.
The index dataset is in chunked storage layout with an unlimited dimension to allow expansion of the index table when more records are added.
There is only one index dataset in the LOG group.
We name it "index_table."
The index entries are sorted by the dataset ID associated with the record.
We store a skip list of the starting position of the index entries of each user dataset so we can skip unrelated entries when reading a dataset.
The skip list is stored in a 1-dimensional dataset of type INT64 (H5T\_STD\_U64LE) in chunked storage layout with an unlimited dimension called skip list dataset.
There is only one skip list dataset in the LOG group. 
We name it "skip_list."
We only create the index dataset and skip list dataset once when the file is closing for performance consideration.
The metadata enclosed in every log entry serves as a backup in case the program is interrupted before we can complete writing the index table.
We will provide a utility program that rebuilds the index table from metadata within log entries.

---

## VOL functionality

In this section, we present the operation performed inside each VOL function.
Any function not mentioned here are not intercepted by the log VOL and are passed directly to the native VOL because they do not need special treatments.

### Log VOL file object

When the file creation or opening function is called, the VOL returns an object representing the file created or opened to the VOL dispatcher.
The object is used to refer to the file on other VOL operations.

The data object returned by the log VOL includes the following fields:

(1) The file object returned by the native VOL

(2) Data structure (array or linked list) for staging I/O requests

(3) A cache of all file metadata (attributes of the LOG group)

(4) A cache of the index table (empty in write-only mode)

(5) A cache of the index skip list (empty in write-only mode)

### File create
We rely on the native VOL to access the HDF5 file.
First, the log VOL creates the file using the native VOL.
Then, it creates a special group called the LOG group under the root group to house additional HDF5 data objects used by the log VOL.
The LOG group, as well as all objects inside, is hidden from the user application.
After that, the log VOL creates two attributes for the LOG group to store metadata of the file.
One is a scalar integer attribute indicating the number of user datasets (dataset counter).
Another one is a scalar integer attribute indicating the number of log datasets (log dataset counter).
We delay the creation of the index dataset, the skip list dataset, and log datasets to the time they are first written.
At that time, we can calculate the proper size of those datasets more accurately.

### File open
To open a file, the log VOL opens the file with the native VOL.
The log VOL reads all the attributes and the index table into the memory for fast access.
If any data object does not exist, the file is considered corrupted. 

### Log VOL dataset object

When the dataset creation or opening function is called, the VOL returns an object representing the dataset created or opened to the VOL dispatcher.
The object is used to refer to the dataset on other VOL operations.

The data object returned by the log VOL includes the following fields:

(1) The dataset object of the anchor dataset returned by the native VOL

(2) A cache of all dataset metadata (attributes of the anchor dataset), includes the unique dataset ID assigned by the log VOL and the shape of the dataset

(3) A pointer to the file object where the dataset belongs to so that requests can be staged in the file object when writing the dataset

### Dataset create
The VOL creates a dataset in the HDF5 (anchor dataset) to represent the data the application creates.
The anchor dataset allows operation not related to data read/write, such as adding dataset attributes, to be performed directly by the native VOL.
Since we store the data of the dataset elsewhere in the log, the space allocated for the dataset will remain unused.
To reduce file size, the log VOL declares the anchor dataset as a scalar dataset in the compact storage layout.
The VOL saves the shape of the actual data space of the dataset as an attribute under the anchor dataset for future queries.
The VOL assigns an ID to the dataset that is unique in the file.
We store the ID as an attribute under the anchor dataset.
The ID is set to the current number of datasets in the file.
After assigned the ID, the number of datasets is increased so that the next dataset created will get a different ID.

### Dataset open
Opening a dataset only involves opening the anchor dataset with the native VOL and reading all attributes into the memory.

### Dataset dataspace query
The shape (saved as an attribute) is cached in the data structure of the dataset.
The log VOL returns a new dataspace created using HDF5 API with the shape information cached.
The VOL does not handle dataspace creation, so we use HDF5 API directly instead of the native VOL. 

### Dataset write
The log VOL performs request aggregation for all write operations.
When the application writes to a dataset, we temporarily store the request in the memory.
A request contains the dataset ID, the selection (start and count), and a pointer to the application data buffer.
For efficiency, we do not keep a copy of the data for requests.
The application should not modify the data buffer until the request is flushed.

### Dataset read
To read a dataset, the vol search through the index table for any record that intersects the selection.
That is, entries with the same dataset ID and a selection intersecting the selection to read.
For every match, the log VOL reads the data from the log dataset and copies it to the corresponding place in the application buffer.
The entries are visited in the order they are recorded so that data form later records overwrites former one as if overwriting a dataset in contiguous storage layout.
The metadata cached in memory is also searched for newly written records since we only construct the index table when the file is closed.

### Dataset close
Closing a VOL dataset is simply closing the anchor dataset with the native VOL.

### File flush

* Calculate the offset to write and size of all records
We are writing records collectively to a shared dataset in which records form different processes appends one after another in the order of process rank.
First, each process calculates the total size of all log entries by counting the size of metadata and data in each staged write request.
The VOL calculates the offset of the records from a process using an MPI_Scan operation on the total size per process.
It calculates the total size of all records among all processes using an MPI_Allreduce operation on the total size per process.

* Calculate the offset to write and size of all records
Knowing the size needed for all log entries, the log VOL prepares the log dataset to store all entries.
If the dataset for log entries (log dataset) has not been created or the remaining space in the log dataset is not enough, we create a new log dataset to store the records.
In such a case, the attribute in the LOG group is updated to reflect the newly created log dataset.
The log dataset is named according to the order they are created (the first one is called "log_1" followed by "log_2" ... etc.) so we can iterate through them for the entries.
The size of the log dataset is calculated based on the size of existing records and the percentage they cover the dataspace.
For example, if existing records occupy 10 MiB while covering 10% of the space in all datasets, we will allocate a log dataset with a size of 100 MiB.

* Writing log entries
The final step is to write log entries to the log dataset.
The log VOL constructs an MPI derived datatype to pack metadata and data of all log entries into a contiguous buffer.
Then, each process selects part of the dataset to write according to the offset calculated previously.
Finally, the log VOL calls dataset writes of the native VOL to write all records to the log dataset.

* Keep metadata in memory
We delay the update of the index table until we need to read datasets or when closing the file.
We keep metadata of all flushed requests in memory even after flushing the data so we can construct the index table later.
The location of the data, including log dataset ID and the offset, is added to each metadata entry in the memory.

### Construct the index

* Sorting the entries
The first step is to sort the local index entries on each process by dataset ID.
Then, we count the total size of entries per dataset.

* Calculate the offset to write and size of all entries for each dataset
In order to construct the index table, processes must insert entries associated with each dataset to specific offset so that the combined table is the correct order.
The VOL calculates the offset of index entries associated with each dataset from a process using an MPI_Scan operation on an array indicating size per dataset.
It calculates the total size of all index entries associated with each dataset among all processes using an MPI_Allreduce operation on the array indicating size per dataset.
After that, the offset for entries associated with each dataset is calculated by summing up the total size of entries form all datasets with smaller ID.
The result is added to the offset calculated by MPI_Allreduce previously to get the actual offset for entries associated with each dataset.

* Create the index dataset and skip list dataset
We need to create the index dataset if it is not created.
If it is created but does not have enough space for all index entries, we need to expand it by calling the set_extend function of the native VOL.
We estimate the size required for the index table in a similar way we used to estimate the size of log datasets.
We do the same to make sure the skip list dataset is created and has enough space.

* Writing to the index dataset
With the offset information, we can now write index entries to the index dataset to form the index table.
Processes select parts of the dataset to write according to the offset calculated previously.
We make one selection per user dataset as entries associated with different datasets will be written to a different location to skip through entries form other processes.
The log VOL calls dataset writes of the native VOL to write all index entries to the index dataset.
Finally, the offset and size of entries form each dataset is written to the skip list dataset by the root process.

### File close
If non-blocking I/O is enabled. The log VOL flushes any request staged in memory to the log dataset when the file is closed.
All attributes cached in the memory are written back to the attributes.
After that, the VOL uses the metadata recorded in the memory to construct an index table.
It creates a dataset (called index dataset) and writes the index table in a way similar to flushing log entries.
Finally, the log vol closes the HDF5 with the native VOL.

## Reference
[1]  M.  Folk,  A.  Cheng,  and  K.  Yates,  “Hdf5:  A  file  format  and  i/o  library for high performance computing applications,” in Proceedings of super-computing, vol. 99, 1999, pp. 5–33.
[2]  D.  Kimpe,  R.  Ross,  S.  Vandewalle,  and  S.  Poedts,  “Transparent  log-based data storage in mpi-io applications,” in
European Parallel Virtual Machine/Message Passing Interface Users’ Group Meeting. Springer, 2007, pp. 233–241.
[3]  M.   Polte,   J.   Lofstead,   J.   Bent,   G.   Gibson,   S.   A.   Klasky,   Q.   Liu, M.  Parashar,  N.  Podhorszki,  K.  Schwan,  M.  Wingate et al. ,  “...  and eat it too: High read performance in write-optimized hpc i/o middleware file formats,” in Proceedings of the 4th Annual Workshop on Petascale Data Storage. ACM, 2009, pp. 21–25.

## Appendix

### Format for log entries

Here we present the format of our log in logical view.
We assume the log is stored as a contiguous datablock for demonstration purpose.
In reality, it is stored across multiple HDF5 datasets.

```
log	= [entry ...]	
entry = dsetid selection big_endian filter data
big_endian	= TRUE | FALSE		// Endianness of values stored in this metadata entry. TRUE means big Endian
dsetid = INT64  // What dataset we are writing to
selection = selection_type subarray_selection | point_selection // Which part of the dataset
selection_type = SUBARRAY | POINT
SUBARRAY_SELECTION = ZERO
POINT_SELECTION = ONE
subarray_selection = start count 
point_selection = num_point start [start]
start		= [INT64 ...]			// number of elements == dimension of the dataset
count		= [INT64 ...]			// number of elements == dimension of the dataset
filter = NONE | ZLIB  // So far, our design does not include compression capability, we leave this entry for future extension
NONE = ZERO
ZLIB = ONE
data = [BYTE] // Raw data received from the application buffer on each write operation
TRUE		= ONE
FALSE		= ZERO
BYTE		= <8-bit byte>
CHAR        = BYTE
INT32		= <32-bit signed integer, native representation>
INT64		= <64-bit signed integer, native representation>
ZERO		= 0		// 4-byte integer in native representation
ONE		= 1		// 4-byte integer in native representation
TWO		= 2		// 4-byte integer in native representation
THREE		= 3		// 4-byte integer in native representation
```
### Format for the index table and skip list

Here we present the format of the index table and the skip list for fast traversal to entrie of specific dataasset.

```
skip_list = [skip_list_entry ...]
skip_list_entry = dsetoff dsetcnt
dsetoff = INT64  // The starting offset of index entries of the dataset ID corresponding to the index of the entry int he skip_list
dsetcnt = INT64  // The number of index entries of the dataset ID corresponding to the index of the entry int he skip_list
index_table = [entry ...]	
entry = dsetid selection big_endian filter data_loc
big_endian	= TRUE | FALSE		// Endianness of values stored in this metadata entry. TRUE means big Endian
dsetid = INT64  // What dataset it is
selection = selection_type subarray_selection | point_selection
selection_type = SUBARRAY | POINT
SUBARRAY_SELECTION = ZERO
POINT_SELECTION = ONE
subarray_selection = start count 
point_selection = num_point start [start]
start		= [INT64 ...]			// number of elements == dimension of the dataset
count		= [INT64 ...]			// number of elements == dimension of the dataset
filter = NONE | ZLIB  // So far, our design does not include compression capability, we leave this entry for future extension
NONE = ZERO
ZLIB = ONE
data_loc = logdsetid off // Where is the data?
logdsetid = INT64  // Which log dataset is the data located?
off = INT64  // What is the offsest of the data in that log datasset?

TRUE		= ONE
FALSE		= ZERO
BYTE		= <8-bit byte>
CHAR        = BYTE
INT32		= <32-bit signed integer, native representation>
INT64		= <64-bit signed integer, native representation>
ZERO		= 0		// 4-byte integer in native representation
ONE		= 1		// 4-byte integer in native representation
TWO		= 2		// 4-byte integer in native representation
THREE		= 3		// 4-byte integer in native representation
```
---
