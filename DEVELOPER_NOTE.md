Log file strategy

# Note for Developers

### Table of contents
- [Logvol file format proposals](#characteristics-and-structure-of-neutrino-experimental-data)

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