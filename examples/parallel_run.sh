#!/bin/bash -l
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

RUN_CMD=`echo ${TESTMPIRUN} | ${SED} -e "s/NP/$1/g"`
# echo "MPIRUN = ${MPIRUN}"
# echo "check_PROGRAMS=${check_PROGRAMS}"

. $srcdir/../tests/common/wrap_runs.sh

async_vol=no
cache_vol=no
log_vol=no
getenv_vol
if test "x$async_vol" = xyes ; then
   # Skip Async I/O VOL as it requires to call MPI_Init_thread()
   return 0
fi

log_vol_file_only=1

for p in ${check_PROGRAMS} ; do
   test_func $p
done

