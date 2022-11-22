1. test0: processing two files (one regular file and one log based file) at the same time. "processing" here means opening/creating the file and the write data to it.
2. test1: Use native vol to create a file with data and then close the file. Reopen the file, read the data, and write to a log based file. Read from the log based file to ensure correctness. 
3. test2: test attribute. modified from `hdf5/examples/h5_attribute.c`.
4. test3: test group. modified from `hdf5/examples/h5_group.c`.
