#!/bin/bash -l
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

RUN_CMD=`echo ${TESTMPIRUN} | ${SED} -e "s/NP/$1/g"`
# echo "MPIRUN = ${MPIRUN}"
# echo "check_PROGRAMS=${check_PROGRAMS}"

. $srcdir/../common/wrap_runs.sh

async_vol=no
cache_vol=no
log_vol=no
getenv_vol

log_vol_file_only=1

for p in ${check_PROGRAMS} ; do
   exe_base=`basename $p`
   if test "x$cache_vol" = xyes && test $exe_base = multi_open ; then
      fmt="####### Skipping %-23s ####################################\n"
      printf "${fmt}" $p
      echo "    Skip multi_open due to a bug in Cache VOL"
      echo "    See https://github.com/hpc-io/vol-cache/pull/17"
      echo ""
      continue
   fi
   test_func ./$p
done

