#!/bin/bash -l
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

RUN_CMD=`echo ${TESTMPIRUN} | ${SED} -e "s/NP/$1/g"`
# echo "MPIRUN = ${MPIRUN}"
# echo "check_PROGRAMS=${check_PROGRAMS}"

. $srcdir/../common/wrap_runs.sh

log_vol_file_only=1

for p in ${check_PROGRAMS} ; do
   if [ "x$p" != "xsubfile_dread" ]; then
      test_func ./$p
   fi
done

for nproc in {1..12} ; do
   RUN_CMD=`echo ${TESTMPIRUN} | ${SED} -e "s/NP/$nproc/g"`
   test_func ./subfile_dread
done
exit 0
