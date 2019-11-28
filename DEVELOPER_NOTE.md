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
  
