[DEFAULT]
version = 0
steps = 10 
arrays = 10 
rows = 32
columns = 32
#process-rows = 1764
process-rows = NP
#process-rows = 882
process-columns = 1
# [weak, strong]
scaling = weak
# align along increment [bytes] boundaries
alignment-increment = 16777216 
# minimum object size [bytes] to force alignment (0=all objects)
alignment-threshold = 0
# minimum metadata block allocation size [bytes]
meta-block-size = 2048
# [posix, core, mpi-io-uni]
single-process = mpi-io-uni
[CUSTOM]
#one-case = 18
#one-case = 119 
#gzip = 6
#szip = H5_SZIP_NN_OPTION_MASK, 8
#async = 1
#delay = 0s
hdf5-file = hdf5_iotest.h5
csv-file = hdf5_iotest.csv
#split = 1
#restart = 1
