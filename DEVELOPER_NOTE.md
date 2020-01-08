Log file strategy

# Note for Developers

### Table of contents
- [Logvol file format proposals](#characteristics-and-structure-of-neutrino-experimental-data)

---

## The goal of the VOL

HDF5 is a file format and an I/O library designed to handle large and complex data collections.
HDF5 probvides a wide range of functions that allows applications to organize complex data along with a wide variety of metadata in a hierachical way.
Due to its convenience and flexibility, HDF5 is widely adopted in scientific applications within the HPC communicty.
However, HDF5 was not originally designed for parallel I/O performance.
Despite all the optimizations introduced later on, HDF5 struggles to keep up with I/O libraries designed specifically for parallel I/O performance, especially on complex I/O patterns.

The cause of the performance difference lies within HDF5's flexible file format to support a wide range of use cases. % A better phase of "use cases"?
One of them is the storage layout.
HDF5 represents data as dataset objects.
An HDF5 dataset can be viewed as a multi-dimensional array of a certain type in an HDF5 file.
HDF5 arranges the data of datasets in the file according to their logical position within the dataset.
As a result, a single write operation may need to write to multiple positions in the file.
Writing to discontiguous positions is less efficient compared to writing to contiguous space on most modern file systems.
It also increases the overhead of parallel I/O by introducing more metadata to synchronize among processes.

Log-based storage layout is shown to be efficient for writing even on complex I/O patterns.
Instead of arranging the data according to its logical location, it simply stores the data as-is.
Metadata describing the logical location is also stored so that the data can be gathered when we need to read the data.
In this way, the overhead of arranging the data is delayed to the reading time.
For applications that produce checkpoints for restarting purposes, the file written will not be read unless there are anomalies.
Trading read performance for write performance becomes an effective strategy to boost overall I/O performance.

In this work, we explore several options to support log-based storage layout for HDF5 dataset.
We develope a HDF5 Virtual Object Layer (VOL) that enable log-based storage layout for HDF5 dataset.
We designed several formats to implement log-styled layout using HDF5 data objects.
We study several strategies to perform parallel I/O on datasets in log-based layout.
We compare those options under a variety of I/O patterns.


  * Match the write performance of the ADIOS library
    + ADIOS achieve it by using log-based storage layout
      + Log-based storage layout for efficient write operations 
      + data caching to reduce I/O and communication
    + We want to catch ADIOS's write performance using a similar technique
  * Enable log-based storage layout in HDF5
    + Efficient write I/O pattern
      + Contiguous in file space
    + Delay organization of data until we need to read the dataset
    + Trade read performance for write performance
      + Not all files written are read later
        + Checkpoint files for restart purpose is discarded
  * Mitigate some performance issue of HDF5 
    + Slow parallel write performance compared to ADIOS
      + The overhead
    + Heavy metadata operation
      + Every data object needs to be located by walking the b-tree
      + Expensive dataset creation
    + No non-blocking operation
---

## Background

Unlike  traditional  storage  layouts,  a  log-baseed  storage
layout  do  not  store  the  data  of  a  dataset  directly.  Instead,  it
records  all  I/O  operations  that  modifies  the  dataset  in  a  log-
based format. In this work, we refer to the data structure used
to store records the ”log”. In the log, records (entries) append
one after another. Each record in the log contains the data as
well  as  the  metadata  that  describes  an  I/O  operation.  Some
design  store  metadata  and  data  together  while  others  store
them separately.
A  major  advantage  of  using  log-baseed  storage  layout  is
write  performance.  Writing  to  a  dataset  in  log-based  storage
layout involves merely appending a record to the log. In this
way,  we  always  write  to  a  contiguous  region  regardless  of
the  I/O  pattern  at  application  level.  Also  there  is  no  need  to
seek before each write. This type of I/O pattern is known be
efficient on most of the file systems.
Contrary to writing, reading a dataset in log-based storage
layout is not that straight forward. The I/O library must search
through  the  records  for  pieces  of  data  that  intersect  with  the
selection.  The  pieces  is  then  stiches  together  to  reconstruct
the  data  within  the  selection.  Some  design  incorporate  an
index of records to speed up the search. Reading data in log-
based  storage  layout  seems  to  be  inefficient  since  a  single
selection  can  intersects  many  records.  However,  Polte,  Milo,
et  al  showed  that  such  senario  seldom  happens  [3].  In  many
cases,  the  read  pattern  is  the  same  as  write,  resulting  in
I/O  patterns  more  efficient  than  that  reading  from  dataset  in
contiguous storage layout.
Due too advantages mentioned above, log-based torage lay-
out has been emploied by many I/O libraries and middlewares.
Kimpe et al. introduced an I/O middleware called LogFS that
records  low-level  I/O  operation  in  a  temporary  log  and  then
reconstruct the data when the process closes the file [2].

## VOL design 

We implemented a HDF5 VOL called log VOL that sotre HDF5 datasets in log-based storage layout.
The log VOL intercept operation regarding dataset creation, querying, writing, and reading.
For other operations, it is passed through to the native VOL (the VOL for native HDF5 format).
We construct data structure for log-base layout using HDF5 datasets and attributes. % How to say it better
The file generated by the log VOL is a valid HDF5 file except that the content is only understandable by the log VOL.

The VOL represents a dataset using a special type of HDF5 dataset called the anchor dataset.
The anchor dataset have the same name as the dataset it represents.
Operations involving the dataset that do not need special tratement form the log VOL is perofrmed on the anchor datasset byt he native VOL.
The data of the dataset is store elsewhere in log-based data structure.
To conserve space, the anchor datasets is declared as scalar datasets.
The original dimensionality and shape is stored as attributes of the anchor dataset for querying.
Datasets are assigned an unique ID to identify them in the log-based structure.

We implement the log using HDF5 datasets.
For best write peroformance, the data structure of the log should be both contiguous in file space and expandable in size.
Unfortunately, non of HDF5 datasets satisfy all the property..
In HDF5, datasets in contiguous storage layout must have a fixed size.
Expandable datasets must be stored in a chunked storage layout that is not contiguous in the file.
For several reasons, we use fixed sized datasets to store records.
First, writing to contiguous dataset is more efficient than having to look up and seek to the positions each chunks.
Also, creating a contiguous dataset involves less metadata overhead  than creating a chunked datasets dataset.
Moreover, both chunks and data objects are indexed by a b-tree in HDF5.
As a resut. expanding chunked dataset (adding chunks) may not be much cheaper than creating datasets.
Finally, a contiguous dataset make it easier to implement optimizations outside HDF5 as discussed in later subsections.

The log is made up of multiple special fixed-size datasets refered to as log datasets.
They are 1-dimensional dataset of type byte (H5T\_STD\_U8LE) we create when we need space to record I/O operations.
The log dataset always locate in the root group with special prefix in the name to distinguish from other datasets.
% This behavior can be improved, we can try to create larger dataset for future writes
There is no header in the log, records in log datasets simply appends one after another.
Within each record are metadata describing one write operation followed by the data to write.
The metadata includes the ID of the dataset to write, the location to write (selection), the location of data, and the endianess of the daata.
The selection can be a hyper-slab (subarray) denoted by start position and length along each dimension or a point list (list of points) denoted by the position of each cell selected.

To allow more efficient searching of records, we place a copy of all metadata into a single dataset.
The dataset storing all metadata is a 1-dimensional fixed-sized dataset of type BYTE (H5T\_STD\_U8LE) refered to as index dataset.
Similar to a log dataset, it is located in the root group and has special prefix in the name to distinguish it form other datasets.
We only create the index dataset and write the metadata once when the file is closing for performance consideration.
The metadata in the log serves as a backup inc ase the program is interrupted before we can write all metadata to the index dataset.
Later on, we can replace the metadata with more sophisticate indexing data structures.

To write records to the log, processes first synchornize on the size of the records from each process.
It is done by performing MPI\_All reduce on the size of records on each process.
With such information, each process can calculate the offset to write their records to without overwriting ecord from other processes.
Records are written to the log in the order of rank of the processes.
If the total size of records exceeds the remaining space in the log dataset, we need to create a new log dataset.
When creating new log dataset, we estimate the space required based on the size required to store records for current existing write oeprations ans the percentage of data space covered so far.
After securing the file space to write, processes select the corresponding region in the log dataset and collectively write their records using the native VOL.

### File create
We relies on the native VOL to access the HDF5 file.
When the application creates a HDF5 file using the log VOL, the log VOL creates the file using the native VOL.
We put hidden data objects used by the log VOL in a special group called LOG group under the root group.
The log VOL creates the LOG group right after the file is created.
It also creates attributes under the LOG group to store metadata used by the log VOL.
Those metadata includes the number of datasets that stores log entries, and the name of the dataset that stores the index table.
The file object returned by the log VOL includes all metadata cached in the memory, the index table, and the object returned by the native VOL.
### File open
To open a file, the log VOL opens the file with the native VOL.
The log VOL tries to opens the LOG group.
If the LOG group exists, the file is considered valid by the log VOL.
They log VOL reads all attributes under the LOG group and store them in memory for fast access.
Finally, the log VOL reads the entire index table into the memory to enable fast lookup of log entries.
If the dataset containing the index table does not exist, the file is considered corrupted. 
The file object returned by the log VOL includes all metadata cached in the memory, the index table, and the object returned by the native VOL.
### File close
### Dataset create
### Dataset open
### Dataset dataspace query
### Dataset close

  * Turn every dataset into scalar datasets
    + Original shape is stored as attributes under the scalar dataset
    + The scalar dataset represents the original dataset
    + Does not occupy (large) space in the HDF file
    + the VOL store all dataset metadata as attributes in the scalar dataset
  * Data of datasets are stored in special datasets along with metadata describing them
    + Log-based data structure
      + Optimize for write performance
    + They are 1-dimensional datasets of type byte
    + Log datasets need to have indefinitely large space
      + Declare as a chunked dataset with unlimited size or reallocate when it is full
    + They act as empty space in the file
  * The VOL pass the operation unrelated to log I/O to the native VOL (default HDF5 VOL) (passthrough)
    + Operations regarding non-dataset data objects are passed to the native VOL
    + Operations on datasets that do not involve read, write, and dataspace query are performed on the scalar dataset by the native VOL
      + If application adds an attribute to a dataset, it will be stored under the scalar dataset representing that dataset
    + Dataset dataspace query must return the original shape instead of scalar
      + The VOL reconstruct dataspace using the shape stored in the attribute

### Accessing the log dataset

    There are 3 approaches to access the log datasets. The most straight forward approach is to use existing high-level HDF5 API (native VOL).
    We simply call the dataset reading/writing function of the native VOL. We reply on HDF5 to perform all caching and optimizations.
    The second approach is to use low-level HDF5 functions that give more control to the VOL. The third is not using HDF5 functions at all.
    We access the dataset directly using MPI-IO.
    We discuss the pros and cons of each approach below.

  * Use high-level HDF5 APIs
    + Read/write log datasets using H5VLdataset_read and H5VLdataset_write with native VOL
    + Enable raw chunk data caching
      + **GOOD**: Serve as a form of aggregation
      + **GOOD**: Reduce the number of I/O operation
      + **GOOD**: Off the shelf, no additional work required
      + **BAD**: We have no control on eviction policy
        + There can be eviction even when there is enough space if the hash table collided
    + Declare log datasets as chunked dataset of unlimited size
      + Datalog has large chunk size
      + Metadata log have medium chunk size
      + Need to balance between allocation overhead and file size
        + Small chunk size need a frequent allocation of new chunks
        + Large chunk size may wastem space if there is not that much data
        + Chunk size of data log can be estimate by total size of datasets.
        + Hard to estimate chunk size for metadata log
  * Use low-level HDF5 APIs
    + H5VLdataset_read and H5VLdataset_write with native VOL
    + Declare log datasets as chunked dataset of unlimited size
    + We need to implement our own caching
      + Can support it through non-blocking callback
      + Aggregate request in memory
      + **GOOD**: We have full control on cache behavior
      + **BAD**: More work on the development
  * Use MPI-IO
    + Create the dataset using HDF5 API
    + Most difficult to implement compared to other approaches
      + **BAD**: Many works to do on development
    + Perform I/O directly using MPI-IO
      + **GOOD**: No additional overhead of HDF5 API and sanity check
    + **BAD**: Need to maintain file consistency
      + If HDF5 has internal status about the file, this will not work
      + HDF5 profiling and accounting will be bypassed
    + Need to store the dataset in contiguous layout
      + Or the offset is hard to calculate
      + Can't declare as unlimited size
      + **BAD**: Must reallocate when it is full

### Dataset to log mapping

    There are three ways to organize logs. In all cases, we record write across all datasets in the same log datasets.
    One approach is to have all processes sharing a log dataset.
    Another is to have separate log datasets for each process.
    The other one is to use sub-filing so that each process writes to one file.
    We discuss the property of each approach below.

1. **Strategy 1** Shared log
   * Create only 2 special datasets per file
     + A metadata log that store all metadata of write operations
     + A data log that store all data written to datasets
     + **GOOD**: Low initialization cost, only two additional datasets created
   * Log is shared by all processes
     + **BAD**: Need communication to prevent overwriting each other
       + MPI_Scan on the offset of metadata and data
     + **BAD**: Write operation needs to be collective
   * Write operations across all datasets are mixed in the same log file
     + **BAD**: Need additional field to record the dataset involved
     + **BAD**: Datasets need to be assigned a fixed ID (IDs in HDF5 can change across runtime).
   * Require the use of MPI VFL driver
     + Required to access shared log datasets
     + **BAD**: No chunk caching
     + **GOOD**: Collective I/O reduces lock contention
   * Only 2 datasets will be written
     + **GOOD**: Higher (system) cache efficiency
     + **GOOD**: Contiguous I/O pattern
   
2. 
   **Strategy 2** Log per process
   * Create 2 spefcial dataset per processes
     + A metadata log for each process that store metadata of write operations from that process
     + A data log for each process that store all data written by that process
     + **BAD**: Initialization is not scalable
       + The number of special datasets created is proportional to the number of processes
       + Each dataset creation involves communication
     + **BAD**: Higher metadata operation
       + Increase in the number of dataset increase the size of B-Tree
         + Need to walk more nodes in the file to reach the data object
   * Every process has its own log
     + **GOOD**: Does not need communication to write new records
     + **GOOD**: Can do independent I/O
   * Write operations across all datasets are mixed in the same log file
     + **BAD**: Need additional field to record the dataset involved
     + **BAD**: Datasets needs to be assigned a fixed ID (IDs in HDF5 can change across runtime)
   * Possible to use POSIX driver
     + **GOOD**: Chunk caching can be used
       + Reduce the number of I/O operation
   * Access pattern can be fragmented
     + Datasets scattered in the file
     + We don't know how they map to the OST
     + **BAD**: Lock contention can happen since all datasets are in the same file.

3. 
   **Strategy 3** Log per process with sub-filing
   * Every process/node has its own file
     + **GOOD**: Does not need communication to write new records
     + **GOOD**: Can do independent I/O
     + **GOOD**: Alleviate initialization cost and metadata operation
       + Each file contains only one additional dataset.
       + **GOOD**: No (or less) communication to create dataset 
       + **GOOD**: Size of B-Tree reduced
     + **GOOD**: Reduce metadta operation 
       + Smaller B-tree, fewer nodes to traverse
     + **GOOD**: No lock contention
     + **BAD**: Need to create many files
       + Metadata server can bceome bottleneck
   * Create 2 spefcial dataset per processes
     + A metadata log for each process that store metadata of write operations from that process
     + A data log for each process that store all data written by that process
   * Map each subfile to a single OST
     + Reduce the number of request on the file server
     + Subfiles should be evenly distributed across OSTs
        + Workload balancing
---

## Log format proposal 

We designed a file format for doing log-based I/O operation on HDF5 datasets.
We implemented it using native HDF5 data objects, so the file is a valid HDF5 file except that the content is only understandable by the log VOL.
The role and function of all types of data object except datasets are the same as a traditional HDF5 file.
In our design, there are three types of HDF5 datasets - the data log, the metadata log, and anchor datasets.
The data log and the metadata log together are referred to as the log.

The data log is a 1-dimensional chunked dataset with unlimited size (H5S\_UNLIMITED).
We store all data written to datasets in the datalog.
There is only 1 data log in the file to store data of all datasets.
The dataset contains a signature (magic) followed by data entries.
Each entry store the raw data of each recorded write operations.

The metadata log is a 1-dimensional chunked dataset with unlimited size (H5S\_UNLIMITED).
We store metadata of each I/O operation into the metadata log.
There is only one metadata log in the file for I/O operations across all datasets.
The metadata log contains a header followed by metadata entries.
Each entry records the metadata of a single write operation.
The metadata records the user dataset involved, the selection, and the location of data in the data log. 
The selection can be a hyper-slab (subarray) denoted by start position and length along each dimension or a point list (list of points) denoted by the position of each cell selected.
For fast traversal, we also record in an entry the position of the next entry and the position of the next entry of the same dataset.


Anchor datasets are scalar datasets that serve as a proxy of HDF5 datasets.
For every dataset created by the application, the VOL creates an anchor dataset of the same name.
We perform every operation form the application other than reading and writing the data, such as adding attributes on the anchor dataset.
We assign each anchor dataset a unique ID to identify them in the log.
We store the metadata of the dataset as attributes under the anchor dataset.
The metadata includes the size of the user dataset it represents, the location of the first record in the metadata log, and the dataset ID.

### Log data structure
```
metadata_log	= metadata_header [entry ...]	// two sections: header and entry
    metadata_header		= metadata_magic format num_entries
        metadata_magic		= "logvol_metadata" VERSION	// case sensitive 8-byte string
            VERSION		= '0'|'1'|'2'|'3'|'4'|'5'|'6'|'7'|'8'|'9'|'.'
        format		= LOGVOL_CLASSIC // The format of the log file inc ase we have different option
            LOGVOL_CLASSIC = ONE
        num_entries	= INT64			// number of MPI processes
    entry = big_endian dsetid selection filter dataptr nextptr
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
        filter = NONE | ZLIB
            NONE = ZERO
            ZLIB = ONE
        dataptr = INT64 // Where are the dtat in data log
        nextptr = INT64 // Where is the next entry of the same dataset
data log	= data_header [data ...]	// two sections: header and entry
    data_header		= data_magic format
        data_magic		= "logvol_data" VERSION	// case sensitive 8-byte string
            VERSION		= '0'|'1'|'2'|'3'|'4'|'5'|'6'|'7'|'8'|'9'|'.'
        format		= LOGVOL_CLASSIC // The format of the log file inc ase we have different option
            LOGVOL_CLASSIC = ONE
    data = [BYTE]
    
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

## Log format design choices  
### Log per dataset vs shared log
* Shared log for all datasets *
  * Benefits non-blocking operation (aggregation)
    * Write one contiguous chunk per metadata and data log
    * Read onc contiguous chunk for metadata
    * Only one communication on offset and size of metadata and data
  * Cache efficient, always writes to offset right after previous one
  * Fewer dataset to create, faster initialization
  * Slight disadvantage on blocking read
    * Need to traverse more metadata
    * Non-contiguous traversal 
* Separate log for each dataset
  * Better blocking read performance, read only one contiguous chunk
  * Slight disadvantage over cache efficiency
  * Slower nonblocking write
    * Need to write discontinuously
  * No significant advantage on nonblocking read
    * Even need to read discontinuous chunks
    * At most some optimization on processing the metadata of the same dataset adjancetly 
  * Slower initialization, need to create more datasets
* Hybrid
    * Make it a user option (future work)
    * Log per group
      * assuming nonblocking I/O won't cross group.

### Separate vs shared data and metadata log
* Separate data log and metadata log *
  * Slightly slower write compared to shared log
  * Allow separate metadata operation
  * Can cache metadata in memory
* Shared log for data and metadata
  * Cut number of contiguous chunk to write in half
  * Unacceptable read performance
    * Non-contiguous metadata traversal
    * Hard to manage metadata cache

### Fixed size vs variable size log
* Chunked dataset (variable size)
  * Need to fully understand HDF5 format or rely on HDF5 API
    * Metadata operation may not be cheap
  * Chunks should  be lighter than dataset to add
  * Chunk size limit to 4 Gib (need confirmation) , may not be enough since its shared across processes
* Fixed sized dataset
  * The effect and overhead may be similar to using chunked dataset
  * Need to reallocate when dataset is full
  * Have more control on our side
---

## Algorithm for writing

Write operation in log VOL is fast because it avoids as much work as possible.
Writing to a dataset is simply appending a record in the log to describe the write operation.
When the application writes to a dataset, the VOL appends the data to the data log.
Then, it adds a record in the metadata log describing the physical location of the data in the data log as well as the logical location of the data in an HDF5 dataset.
Finally, it updates the previous metadata entry and last metadata entries of the same dataset to point to it.
If it is the first entry of a dataset, the attribute also needs to be updated.

Since the log is shared by all processes, synchronization is required so that processes don't overwrite each other.
As a result, blocking write operations has to be collective.
The data form each process in a collective write operation is recorded as separate I/O requests in the log.
Records are written to the log in the order of rank of the processes.
The process synchronizes on the position (offset) their requests should be written to in both the metadata log and the data log.
It is done by performing MPI\_Scan on the size of metadata and the size of data on each process.
After that, processes select the corresponding region in the metadata log and the data log and collectively write their records using the native VOL.
In case the file is open or read as well, the metadata needs to be known to all processes.
Instead of having all processes read back for the metadata log, we perform MPI\_Allgather to share the added metadata to all processes.

Each metadata entry contains information that links to the next entry of the same dataset.
Processes need to know the position of the first entry of every dataset in the records of later processes to calculate such information.
In read/write mode, since we already have records from all processes, it can be calculated locally.
In write-only mode, we perform a scan in reverse order on an array that records the location of the first metadata entry of every dataset.
The first processes are responsible for updating the link in previous entries.
We keep track of the last entry of every dataset in memory so those entries can be located to update the link.

---

## Algorithm for reading

Reading in a log storage layout is inherently inefficient because the data is completely unorganized.
In the log storage layout, a dataset is described by a set of log entries scattered through the data log.
Reading the data is like searching the log for related entries and collect pieces of data that need to be read.
The pieces are then stitched together to reconstruct the read data.
The complexity increases as more log entries are appended to the log.

The log VOL handle read requests in 2 steps.
The first step is to collect all pieces of data from the data log into a buffer.
The log VOL calculates the intersection of selection recorded in each metadata log entry and the selection in the read request.
It uses the link in each metadata entry to skip records of other datasets.
For each intersection, the corresponding region in the data log is selected.
The log VOL then reads the data into a temporary buffer using the native VOL.
In the second step, data in the temporary buffer is copied to the user buffer.
The log VOL creates an MPI derived datatype to describe the location of data in the user buffer and use MPI\_Pack to copy the data to the user buffer.
For efficiency, we keep a copy of the metadata log in the memory all the time.

---

## HDF5 Limitations
### Parallel I/O for Datasets with Compression Enabled
* Parallel I/O must be collective when the dataset compression feature is
  enabled.
* Parallel compression in HDF5 is not mature yet. Quoted from Jordan Henderson
  [4979](https://forum.hdfgroup.org/t/compressed-parallel-writing-problem/4979/10)
  ```
  for a while now the feature has been essentially experimental and the
  collective requirement for H5Dwrite did'nt really make it into any official
  documentation; it was only mentioned in the HDF5 1.10.2 release blog post that
  @inchinp linked at the start of this topic. That being said, we are hoping to
  make some improvements to the feature in the near future and part of that would
  entail better documentation and examples for users.
  ```

### Chunk Ownership
* When compression of a dataset is enabled, a chunk can only be compressed
  and decompressed by a single process. The ownership of chunks becomes the
  parallel I/O (data partitioning) pattern for a dataset. The internal design
  of chunk ownership in HDF5 is based on which process accesses the biggest
  portion of a chunk. An MPI collective communication is called to calculate
  the ownership which needs to be consistent among all processes. All I/O
  requests to a chunk are shipped to the chunk owner through MPI communication
  calls (isend/irecv).
* **TODO** -- In order to balance the workload of I/O, compression, and
  decompression, an application-level chunk redistribution among processes (and
  align them with chunk boundaries) is necessary. This idea leads to Strategy
  3 (see below). However, HDF5 internally ships the write data to the chunk
  owners, which is equivalent to Strategy 3 for the write case.

### Metadata Operation Modes
* :question:
  HDF5 allows metadata operations to be in either collective or independent
  mode, settable by users through the two APIs below. Collective mode requires
  most all arguments be the same among the calling processes. However, MPI
  collective communication may be called, which can make creating and opening
  a large number of datasets very expensive.
  ```
  H5Pset_coll_metadata_write()
  H5Pset_all_coll_metadata_ops()
  ```
* **TODO**
  1. Check what MPI communication and I/O functions are actually called for
     each `H5Dopen` and `H5Dcreate`. Preliminary study reveals that HDF5 uses a
     metadata cache of default size 2 MiB to temporally store the metadata and
     evict the metadata if a hash collision is encountered. Increasing the
     metadata cache size may reduce the eviction frequency.
  2. Count the number of MPI communication and I/O calls for both collective
     and independent modes.
  3. Evaluate the performance impact of these 2 modes for creating/opening
     a large number of datasets, e.g. > 16K.

### Slow Parallel Dataset Creation
* :x:
  From an experiment of concatenating 128 files using Haswell nodes on Cori,
  creating all 16K datasets took about 25~30 seconds, regardless of the number
  of MPI processes (2 per node). In discussion
  [3619](https://forum.hdfgroup.org/t/improving-performance-of-collective-object-definitions-in-phdf5/3619/2)
  from the HDF Forum, the trick below is suggested.
  ```
  fapl_id = H5Pcreate (H5P_FILE_ACCESS);
  H5Pset_libver_bounds (fapl_id, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
  file_id = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, fapl_d);
  ```
* :x:
  On Bigdata, this trick does not show much difference. When concatenating 2
  files by 2 processes on a single node, the creation time for 16K datasets
  without this trick was 3.01 seconds vs. 3.67 seconds with it. 
* Note in that discussion the poor creation performance occurs for the case
  when creating more than 50K datasets in a single group
* **TODO**
  1. Increase metadata cache size to reduce metadata eviction frequency.

### Adjustable Metadata Cache Size
* :question:
  For dataset creations, the default metadata cache size set by HDF5 is 2 MiB
  which is automatically adjusted by HDF5. When setting the minimum and maximum
  cache size to 128 MiB and manually turning off the automatic adjustment (so
  the maximum size is always used), we observed the creation time was
  **reduced**.
* Creating output file on NFS using 2 processes (on 2 Bigdata compute nodes)
  takes about 13 seconds with the default setting.  After turning off the
  automatic adjustment and fixing the size to 128 MB, the creation time is
  reduced to 7.8 seconds.
* The metadata cache size can be adjusted using the APIs [H5Fget_mdc_config](https://support.hdfgroup.org/HDF5/doc1.8/RM/RM_H5F.html#File-GetMdcConfig)
  and [H5Fset_mdc_config](https://support.hdfgroup.org/HDF5/doc1.8/RM/RM_H5F.html#File-SetMdcConfig).
  Note that, based on [HDF5 manual](https://support.hdfgroup.org/HDF5/doc/Advanced/MetadataCache/index.html),
  the version should always be set to `H5AC__CURR_CACHE_CONFIG_VERSION`.
  A short code snippet for adjusting the metadata cache size is as follows.
  ```
  H5AC_cache_config_t config;
  config.version = H5AC__CURR_CACHE_CONFIG_VERSION;
  H5Fget_mdc_config(output_file_id, &config);
  config.min_size = 1048576;
  config.decr_mode = H5C_decr__off;
  H5Fset_mdc_config(output_file_id, &config);
  ```
* :question:
  On 64 Cori Haswell nodes, 128 processes take 33.23 sec to create 16K datasets
  with the default metadata cache setting (initial: 2 MiB, maximum: 32 MiB).
  After increasing it to 128 MiB and turning off the automatic adjustment, the
  creation time is reduced to 30.89 sec.
* :x:
  Increasing the metadata cache size has no effect on the **read** stage of
  input files. Our evaluation shows no significant improvement. In-memory I/O
  for read stage is still the best option. This is because opening every group
  or dataset still result in a read from the file. There is no aggregated read
  in HDF5. This is versus using in-memory I/O, which reads the file a big chunk
  at a time.
* **TODO**
  1. In order to know whether the metadata cache size has a significant impact
     hacking to HDF5 to count the number of MPI communication and I/O calls is
     required. Those numbers reflect the timing costs. Gathering the counts of
     MPI calls on local machines is to study the HDF5 internal mechanism.
  
### Raw Data Chunk Caching
* HDF5 allows users to adjust setting for
  [raw data chunk caching](https://support.hdfgroup.org/HDF5/doc/H5.user/Caching.html)
  through API
  [H5Pset_cache()](https://support.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetCache)
  ```
  herr_t H5Pset_cache(hid_t plist_id, int mdc_nelmts, size_t rdcc_nslots, size_t rdcc_nbytes, double rdcc_w0)
  ```
  + `mdc_nelmts` - the number of objects (no longer used)
  + `rdcc_nslots` - hash table size and the number of possible hash values 
  + `rdcc_nbytes` - total cache size
  + `rdcc_w0` - chunk preemption policy
* :x:
  However, raw dataset chunk caching is **not** currently supported when using
  the MPI I/O and MPI POSIX file drivers in read/write mode. When using one of
  these file drivers, all calls to `H5Dread` and `H5Dwrite` will access the
  disk directly, and `H5Pset_cache` will have no effect on performance.
* The documentation of HDF5 claims that chunk caching is when MPI-IO drivers are used in
  read-only mode. However, we found it not the case. We read the same dataset in a file 
  stored in NFS twice, HDF5 issues the same amount of I/O request to MPI-IO every time. 

### Reading Partial Chunk
* For datasets whose compression feature are **disabled**, reading a chunk
  partially results in reading only the requested part of the chunk. Collective
  reads are implemented using `MPI_Type_create_hindexed`.
* For datasets whose compression feature are **enabled**, reading a chunk
  partially results in reading the whole chunk, decompressing it, and copying
  the requested part of chunk over to the user buffer.

### Reading and Writing the Whole Dataset
* HDF5 always performs the intersection check of a request to all chunks, even
  when reading or writing the entire dataset, i.e. using H5S_ALL in dataspace.
* Such intersection check can be expensive when the number of chunks is large.
* **TODO**: One possible improvement is to skip the checking.

### Impact of Chunk Size on Compression/Decompression time
In general, a large chunk size causes an expensive compression/decompression
time while it provides cheaper metadata operations.  When the data is highly
compressible, the compression/decompression cost can be dominant over the I/O
time due to the high compression ratio. A large chunk size also means a smaller
number of chunks which lowers the degree of parallelism. Therefore, the chunk
size should be tuned to a sufficiently small value which makes a good trade-off
between the compression cost and the metadata operation cost.

* Reading compressed chunks
  + In our experiments, we found that the chunk sizes smaller than 8 MB did not
    show a large difference of decompression time.  The decompression time
    starts to increase from 16 MB.  The metadata operation time is linearly
    reduced as the chunk size increases.  The chart below shows the timing
    breakdown for concatenating 8 files that contains `slicemap` dataset only.
    Even though the metadata operation is expensive, because the decompression
    time is much longer than the metadata and I/O time, A small chunk size
    between 1 MB ~ 8 MB can be considered as a good chunk size for this case.

    ![picture](docs/read_decomp.jpg)
    
* Writing compressed chunks
  + We see the similar result for write. The larger chunk size gives a higher
    compression time.  The metadata operation cost for collective writes was
    not much affected by the chunk size.  Considering the degree of
    parallelism, therefore, 1 MB is a practical choice.
  
    ![picture](docs/write_comp.jpg)

Based on the analysis, we use 1 MB as the default chunk size.

---