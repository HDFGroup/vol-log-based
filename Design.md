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

In this report, we present a design of HDF5 Virtual Object Layer (VOL) to enable log-based storage layout for HDF5 dataset.
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
The log VOL intercept operation regarding dataset creation, querying, writing, and reading.
For other operations, it is passed through to the native VOL (the VOL for native HDF5 format).
We construct data structure for log-base layout using HDF5 datasets and attributes.
We place all hidden HDF5 data objects used by the VOL is a group under the root group called the LOG group.
The file generated by the log VOL is a valid HDF5 file except that the content is only understandable by the log VOL.

The VOL represents a dataset using a special type of HDF5 dataset called the anchor dataset.
The anchor dataset has the same name as the dataset it represents.
Operations involving the dataset that do not need special treatment form the log VOL is performed on the anchor dataset by the native VOL.
The data of the dataset is stored elsewhere in a log-based data structure.
To conserve space, anchor datasets are declared as scalar datasets.
The original dimensionality and shape are stored as attributes of the anchor dataset for querying.
Datasets are assigned a unique ID to identify them in the log-based structure.

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
The metadata includes dataset ID, selection, the location of the data, the endianness of the data, and a time stamp.
The selection can be a hyper-slab (subarray) denoted by start position and length along each dimension or a point list (list of points) denoted by the position of each cell selected.

To allow more efficient searching of records, we construct an index table of all records and save it into a dataset.
The dataset storing all metadata is a 1-dimensional fixed-sized dataset of type BYTE (H5T\_STD\_U8LE) referred to as the index dataset.
Similar to a log dataset, it is located in the LOG group and has a special prefix in the name to distinguish it from other datasets.
We only create the index dataset and write the metadata once when the file is closing for performance consideration.
The metadata in the record serves as a backup in case the program is interrupted before we can write build the index table.
Currently, our index table is merely a list of metadata entries sorted according to the target dataset ID and the time stamp.

Processes must synchronize on the size of the records from each process in order to write records to the log dataset.
It is done by performing MPI\_Allreduce on the size of records on each process.
With such information, each process can calculate the offset to write their records without overwriting records from other processes.
Records are written to the log in the order of rank of the processes.
If the total size of records exceeds the remaining space in the log dataset, we need to create a new log dataset.
When creating a new log dataset, we estimate the space required based on the size required to store records for current existing write operations and the percentage of data space covered so far.
After securing the file space to write, processes select the corresponding region in the log dataset and collectively write their records using the native VOL.

## VOL behavior

In this section, we present the behavior of our VOL in each operation in detail.

### File create
We rely on the native VOL to access the HDF5 file.
When the application creates an HDF5 file using the log VOL, the log VOL creates the file using the native VOL.
We put hidden data objects used by the log VOL in a special group called the LOG group under the root group.
The log VOL creates the LOG group right after the file is created.
It also creates attributes under the LOG group to store metadata used by the log VOL.
That metadata includes the number of datasets that stores log entries, the name of the dataset that stores the index table, and the number of datasets in the file.
The file object of the log VOL includes all metadata cached in the memory, the index table, the object returned by the native VOL.
It also includes the object of the LOG group, the index dataset, and all log datasets.
### File open
To open a file, the log VOL opens the file with the native VOL.
The log VOL  reads all the attributes and the index table into the memory for fast access.
If any data object does not exist, the file is considered corrupted. 
The file object returned by the log VOL includes all metadata cached in the memory, the index table, and the object returned by the native VOL.
### File close
If non-blocking I/O is enabled. The log VOL flushes any request staged in memory to the log dataset when the file is closed.
All attributes cached in the memory are written back to the attributes.
After that, the VOL uses the metadata recorded in the memory to construct an index table.
It creates a dataset (called index dataset) and writes the index table in a way similar to flushing log entries.
Finally, the log vol closes the HDF5 with the native VOL.
### Dataset create
The VOL creates a dataset in the HDF5 (anchor dataset) to represent the data the application creates.
The anchor dataset allows operation not related to data read/write, such as adding dataset attributes, to be performed directly by the native VOL.
Since we store the data of the dataset elsewhere in the log, the space allocated for the dataset will remain unused.
To reduce file size, the log VOL declares the anchor dataset as a scalar dataset in the compact storage layout.
The VOL saves the shape of the actual data space of the dataset as an attribute under the anchor dataset for future queries.
The VOL assigns an ID to the dataset that is unique in the file.
We store the ID as an attribute under the anchor dataset.
The ID is set to the current number of datasets in the file.
After assigned the ID, the number of files is increased, and the attribute representing it under the LOG group is updated.
The data structure returned by the log VOL contains the dataset returned by the native VOL and all metadata, such as its shape and ID.
### Dataset open
Opening a dataset only involves opening the anchor dataset with the native VOL and reading all attributes into the memory.
The data structure returned by the log VOL contains the dataset returned by the native VOL and all metadata, such as its shape and ID.
### Dataset dataspace query
The shape (saved as an attribute) is cached in the data structure of the dataset.
The log VOL returns a new dataspace created using HDF5 API with the shape information cached.
The VOL does not handle dataspace creation, so we use HDF5 API directly instead of the native VOL. 
### Dataset close
Closing a dataset does not require any other operations other than closing the dataset with the native VOL.
### Dataset write
The behavior of writing depends on whether we are doing aggregation.
If not, dataset writing acts like flushing with only one entry staged.
If aggregation is enabled, we temporary store a record in the memory.
A record contains the dataset ID, the selection (start and count), and the data.
For efficiency, we do not actually copy the data but only keep the pointer to the application buffer.
The application should not modify the data buffer until the request is flushed.
Each record is given a timestep so ensure newer data read in case records overlap each other.
### Dataset (file) flush
We are writing records collectively to a shared dataset in which records form different processes appends one after another in the order of process rank.
The VOL calculates the offset of the records from a process using an MPI_Scan operation.
It synchronizes the size of all records across processes using an MPI_Allreduce operation.
If the dataset for log entries (log dataset) has not been created or the remaining space in the log dataset is not enough, we create a new log dataset to store the records.
In such a case, the attribute in the LOG group is updated to reflect the newly created log dataset.
The log dataset is named according to the order they are created (the first one is called "log_1" followed by "log_2" ... etc.) so we can iterate through them for the entries.
The size of the log dataset is calculated based on the size of existing records and the percentage they cover the dataspace.
For example, if existing records occupy 10 MiB while covering 10% of the space in all datasets, we will allocate a log dataset with a size of 100 MiB.
The log VOL calls dataset write of native VOL to write all records to the log dataset.
After flushing the records, we keep a copy of the metadata of each record in memory.
The log VOL adds an entry to the index table for each record written to the log dataset.
The index entry contains the same metadata as the records plus the location of the record in the file.
The location is represented by the ID of the log dataset they are stored and the offset (index) within the log dataset.
Index entries must be synchronized (all scatter) across processes so new data can be seen by all processes.
We also synchronize the time stamp across the processes after flushing the data. 
### Dataset read
To read a dataset, the vol search through the index table for any record that intersects the selection.
That is, entries with the same dataset ID and a selection intersecting the selection to read.
For every match, the log VOL reads the data from the log dataset and copies it to the corresponding place in the application buffer.
The entries are visited in the order they are recorded so that data form later records overwrites former one as if overwriting a dataset in contiguous storage layout.
The metadata cached in memory is also searched for newly written records since we only construct the index table when the file is closed.
### Construct the index
For now, we use a simple array-based index table.
The index table is an array of index entries.
Each entry represents a record in log datasets.
Entries are sorted by dataset ID, followed by the time stamp.
We also construct a skip list for entries that belongs to one dataset so we can skip unrelated entries.
The skip list is stored before the index array in the index dataset.

## Reference
[1]  M.  Folk,  A.  Cheng,  and  K.  Yates,  “Hdf5:  A  file  format  and  i/o  library for high performance computing applications,” in Proceedings of super-computing, vol. 99, 1999, pp. 5–33.
[2]  D.  Kimpe,  R.  Ross,  S.  Vandewalle,  and  S.  Poedts,  “Transparent  log-based data storage in mpi-io applications,” in
European Parallel Virtual Machine/Message Passing Interface Users’ Group Meeting. Springer, 2007, pp. 233–241.
[3]  M.   Polte,   J.   Lofstead,   J.   Bent,   G.   Gibson,   S.   A.   Klasky,   Q.   Liu, M.  Parashar,  N.  Podhorszki,  K.  Schwan,  M.  Wingate et al. ,  “...  and eat it too: High read performance in write-optimized hpc i/o middleware file formats,” in Proceedings of the 4th Annual Workshop on Petascale Data Storage. ACM, 2009, pp. 21–25.

## Appendix

### Log data format

Here we present the format of our log in logical view.
We assume the log is stored as a contiguous datablock for demonstration purpose.
In reality, it is stored across multiple HDF5 datasets.

```
log	= [entry ...]	
    entry = dsetid selection big_endian filter timestamp data
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
        timestamp = INT64 // An increasing sequence so we know which record should overwrite the other when there is an overlap
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
### Index format

Here we present the format of the index table.
The index table is stored in a dataset in contiguous layout.

```
index = skip_list [entry ...]	
    skip_list = [skip_list_entry ...]
        skip_list_entry = dsetoff dsetcnt
            dsetoff = INT64  // The starting offset of index entries of the dataset ID corresponding to the index of the entry int he skip_list
            dsetcnt = INT64  // The number of index entries of the dataset ID corresponding to the index of the entry int he skip_list
    entry = dsetid selection big_endian filter timestamp data_loc
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
        timestamp = INT64 // An increasing sequence so we know which record should overwrite the other when there is an overlap
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
