Log file strategy

# Note for Developers

### Table of contents
- [Logvol file format proposals](#characteristics-and-structure-of-neutrino-experimental-data)

---
## Log format proposal

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