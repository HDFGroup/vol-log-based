#!/bin/bash -l
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

# Exit immediately if a command exits with a non-zero status.
set -e

. ${top_builddir}/utils/h5lenv.bash
export LD_LIBRARY_PATH="${builddir}/build/lib:${LD_LIBRARY_PATH}"

if test "x$#" = x0 ; then
    RUN=""
else
    RUN=`echo ${TESTMPIRUN} | ${SED} -e "s/NP/$1/g"`
fi

export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}"
export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"

echo "${RUN} ./8a_benchmark_write_parallel > 8a_benchmark_write_parallel.log"
${RUN} ./8a_benchmark_write_parallel > 8a_benchmark_write_parallel.log

# echo "${RUN} ./8b_benchmark_read_parallel ../samples/8a_parallel_3Db_0000001.h5 sy > 8b_benchmark_read_parallel.log"
# ${RUN} ./8b_benchmark_read_parallel ../samples/8a_parallel_3Db_0000001.h5 sy > 8b_benchmark_read_parallel.log

outfile=../samples/8a_parallel_3Db_0000001.h5
err=0
unset HDF5_VOL_CONNECTOR
unset HDF5_PLUGIN_PATH
FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k $outfile`
if test "x${FILE_KIND}" != xHDF5-LogVOL ; then
   echo "Error: Output file $outfile is not Log VOL, but ${FILE_KIND}"
   err=1
else
   echo "Success: Output file $outfile is ${FILE_KIND}"
fi
exit $err

