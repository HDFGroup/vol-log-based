## Format of metadata table
The log metadata is defined as an HDF5 dataset of type H5T_STD_U8LE (unsigned byte in Little Endian format).
It is a byte stream containing the information of all write requests logged by the log driver.
Below is the format specification of the metadata table in the form of Backus Normal Form (BNF) grammar notation. 
```
metadata                      = decomp entries
decomp                        = nproc [end_off ...]
nproc                         = INT32
end_off                       = INT64                                      // Ending offset of the index entries written by a process, excluding rank 0
entries                       = [entry ...] 
entry                         = header selection
header                        = entry_size dataset_id flag file_offset file_size
entry_size                    = INT32                                      // Total size of the entry in byte
dataset_id                    = INT32                                      // The ID of the dataset written
flag                          = is_multi_block BIT is_encoded is_compressed is_reference is_record padding
is_multi_block                = BIT
is_encoded                    = BIT
is_compressed                 = BIT
is_reference                  = BIT
is_record                     = BIT
padding                       = [BIT ...]                                  // <0~31 bits to 4-byte boundary>
file_offset                   = INT64                                      // Offset of the data in the file
file_size                     = INT64                                      // Size of the data in the file
selection                     = single_selection | multi_selection | record_selection | ref_selection | record_ref_selection
single_selection              = start count 
start                         = [INT64 ...]                                // starting offsets along all dimensions
count                         = [INT64 ...]                                // access lengths along all dimensions
multi_selection               = nsel selection_list | encoded_selection_list
selection_list                = [single_selection ...]
encoded_selection_list        = encoding_info [encoded_selection ...]
encoding_info                 = [INT64 ...]
encoded_selection             = encorded_start encorded_end
encorded_start                = INT64
encorded_end                  = INT64
record_selection              = record_num multi_selection
record_num                    = INT64
ref_selection                 = INT64                                      // Related file offset to the referenced entry
record_ref_selection          = record_num ref_selection
BIT                           = ON | OFF
ON                            = 1                                          // A 1 bit
OFF                           = 0                                          // A 0 bit
INT32                         = <32-bit signed integer, native representation>
INT64                         = <64-bit signed integer, native representation>
```
