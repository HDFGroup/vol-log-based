# The goal of Log VOL
As part of the ECP, we are exploring ways to improve parallel I/O performance of HDF5.
In this work, we explore the idea of storing HDF5 datasets in log-based storage layout 
We plan to implement our idea as an HDF5 Virtual Object Layer (VOL).
We hope to improve HDF5's parallel write performance by always writing to contiguous blocks.

---

# The purpsoe of this design document
We create this document to promote collaboration between members.
In this document, we organize the ideas we come up with related to the design and implementation of log-based VOL.
We organize them based on the part of the VOL functions they are targeting.
We include the lesson we learned by studying other documentations and running some preliminary experimental results.
Everyone is encouraged to discuss and make suggestions on this document. 


# Supporting parallel read on datasets in log-based storage layout
  Here we discuss two potential approach to support efficient read of datasets in log-based storage layout.

## Implement read from scratch 
  In this approach, we implement our mechanism to support reading.
* Constructing the metadata table
  To allow efficient searching for write requests in log datasets, we construct a metadata table of the log entries.
  Each entry of the metadata table stores the metadata of a write request. 
  The metadata includes the location of data in the file and where the data belongs to (which dataset, which part).
  Entries are sorted based on increasing order of dataset IDs of the dataset involved.
  + Storing the metadata table as an unlimited 1-dimensional byte dataset
    There is only one metadata table dataset.
    The dataset must be expandable to accommodate future writes.
    HDF5 requires all datasets with unlimited size to be chunked.
  + Accelerate table lookup with a lookup table 
    We create a lookup table containing the offset of the first metadata entry of every dataset.
    It allows us to skip through irrelevant entries.
* Reading from the log entries 
  The VOL traverse the metadata table and look for entries that intersect the selection.
  For every intersection, the VOL retrieve the intersecting part of the data and place it into the correct position in the application buffer.
  + HDF5 selection always assumes canonical order
    Regardless of the order selection is made, HDF5 always reads selected data according to its position in canonical order.
    For example, we cannot create a selection that reads the rows of a 2-D dataset in reverse order.
    The same behavior applies to memory space selections.
    For this reason, we can not have HDF5 read the data directly into the application buffer by manipulating the selection.
    Instead, we read all data into a temporary buffer and copy them into the application buffer using MPI_Pack.
  + Bypassing HDF5 with MPI-IO
    An alternate solution to read data is to use MPI-IO directly.
    HDF5 selection can be easily translated to MPI derived datatype using subarray type (MPI_Type_create_subarray).
    We can adjust the order in memory datatype to read directly into the application buffer, assuming the application does not read the same place twice.

## Create Virtual Dataset for reading
  In this approach, we utilize a HDF5 feature called The Virtual Dataset (VDS) to support reading.
* What are Virtual Datasets
  Virtual Datasets (VDS) are a special type of dataset that do not have their own space for data in the HDF5 file.
  A VDS contains multiple links that map part of its data space to the space of other datasets, similar to shortcuts.
  The application defines a VDS by setting the mappings in the dataset creation property list.
  The mapping can be defined on a portion of the data space using selection.
  The target dataset does not need to sit in the same file as the virtual dataset.
  The selection of virtual data set and target dataset does not need to be the same as long as the size matches.
  Applications define mappings using the dataset creation property list.
  For more information about the VDS, please refer to https://support.hdfgroup.org/HDF5/docNewFeatures/NewFeaturesVirtualDatasetDocs.html
* Reading dataset in log-based storage layout via VDS mappings
  We can use VDS to support reading to data stored in the log dataset.
  For each dataset created by the application, we create a virtual dataset.
  After writing the data to the log dataset, the VOL defines a mapping in the corresponding virtual dataset that maps the selected space to the data in the log dataset.
  Read operations on a dataset are redirected to the corresponding virtual dataset.
* Low-performance dues to frequent file open and close
  The VDS allows linking to datasets across multiple files. The mappings are stored internally as metadata.
  On each H5Dread, the selection is compared with the mappings stored in the VDS.
  For each mapping that intersects the selection, HDF5 opens the source file, reads the data, and immediately close the source file.
  In a dataset with many mappings, opening and closing operations hinder the performance significantly.
  + Example program (example/vds.cpp)
    We create a 1024 x 1024 int32 dataset in contiguous layout.
    We create a virtual dataset of the same size under the same HDF5 file.
    The n-th column of the virtual dataset maps to the n-th row of the contiguous dataset.
    We compared column-wise access to the contiguous and the virtual dataset.
    We expect the virtual dataset to perform better than the contiguous dataset (close to row-wise access) since the access pattern is contiguous in the file space.
    The performance of the virtual dataset turns out to be slow.
    We studied the behavior and found that it is caused by repeated open and close operations.
    + Result running on a desktop
      ```
      Create virtual dataset: 80 ms

      Row-wise write on the contiguous dataset: 20 ms
      Column-wise write on the contiguous dataset: 3496 ms
      Column-wise write on the virtual dataset: 22764 ms

      Rowise read on contiguous dataset: 20 ms
      Column-wise read on the contiguous dataset: 1774 ms
      Column-wise read on the virtual dataset: 22143 ms
      ```
    + Potential solution
      There is no workaround from outside the HDF5.
      The VDS implementation can be improved with a file cache.
      Instead of closing the file after reading, we can keep it open for future access.
  + Lack of MPI-IO support
    Currently, the VDS is not available under the MPI VFL driver.
    Since our VOL is designed for parallel I/O, we need to use the MPI VFL driver for performance consideration.
    + Delay virtual dataset creation
      A workaround is to delay the creation of virtual datasets until the file is closed.
      The metadata is kept in memory during and session and gathered to the master rank (rank 0).
      The master rank reopens the file using the POSIX VFL driver and creates all virtual datasets.
    + Open the file in read-only mode
      The easiest way to support reading is to have individual processes open the file using the POSIX VFL driver.
      It requires the file to be opened in read-only mode, so HDF5 will not try to lock the file.
      As a result, read and write modes are not supported.

# Other lessons we learned that is related to the design
## Metadata operation cost
  * Expensive B-tree operations
    Everything in HDF5 is indexed using B-tree.
    There is a B-tree under each group to track data objects within that group.
    There is a B-tree for every chunked dataset to track its chunks.
    Locating data objects requires traveling the B-tree form the current location (e.g., the loc_id passed to H5Dopn).
    Since the location of a child node of a B-tree is not known before reading its parent node, traversing a B-tree may involve multiple writes.
    + We should avoid creating many hidden data object (datasets, groups)
      Metadata operations are relatively expensive compared to NetCDF.
      They may involve multiple read operations.

# Reducing metadata overhead

## Updated metadata format
The log metadata table is defined as an HDF5 dataset of type H5T_STD_U8LE
(unsigned byte in Little Endian format). It is a byte stream containing the
information of all write requests logged by the log driver. Below is the format
specification of the metadata table in the form of Backus Normal Form (BNF)
grammar notation.
```
metadata_table		= signature endianness entry_list

signature		= 'L' 'O' 'G' VERSION
VERSION			= \x01
endianness		= ZERO |	// little Endian
			  ONE		// big Endian

entry_list		= nelems [entry_off ...] [entry_len ...] index_table
entry_off		= INT64		// starting file offset of an entry
entry_len		= INT64		// byte size of an entry
					// entry_off and entry_len are pairwise

index_table		= [entry ...]	// log index table
entry			= dsetid flag location
					// A log entry contains metadata of
					// write requests to a dataset. One
					// dataset may have more than one
					// entry.

dsetid			= INT64		// dataset ID as ordered in var_list

flag		= is_varn_loc is_merged_loc is_offset_loc is_zip_loc
is_varn_loc = FALSE1 | TRUE1
is_merged_loc = FALSE1 | TRUE1
is_offset_loc = FALSE1 | TRUE1
is_zip_loc = FALSE1 | TRUE1

location = zip_location | raw_location
zip_location = [BYTE...]
raw_location = loc_encode_into location_data
loc_encode_into = [INT64 ...] // Information to translate start and count to flattened offsets

location_data = varn_loc | merged_loc | simple_loc

varn_loc = file_loc dset_loc dset_loc [dset_loc ...]

file_loc		= file_off file_size 
file_off = INT64		// starting file offset storing the logged dataset contents
file_size = INT64		// size of the data

dset_loc = encoded_dset_loc | raw_dset_loc
encoded_dset_loc = start_off end_off
start_off = INT64
end_off = INT64
raw_dset_loc = start_cord count
start_cord = [INT64 ...]
count = [INT64 ...]
simple_loc= file_loc dset_loc
merged_loc = [simple_loc ...]

FALSE1    = 0	// 1-bit integer in native representation
TRUE1     = 1	// 1-bit integer in native representation
NONE			= ZERO
ZLIB			= ONE
TRUE			= ONE
FALSE			= ZERO
BYTE			= <8-bit byte>
CHAR			= BYTE
INT32			= <32-bit signed integer, native representation>
INT64			= <64-bit signed integer, native representation>
ZERO			= 0	// 4-byte integer in native representation
ONE			= 1	// 4-byte integer in native representation
TWO			= 2	// 4-byte integer in native representation
THREE			= 3	// 4-byte integer in native representation
```

