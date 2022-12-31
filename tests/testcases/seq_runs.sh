#!/bin/bash -l
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

RUN_CMD=${TESTSEQRUN}

. $srcdir/../common/wrap_runs.sh

async_vol=no
cache_vol=no
log_vol=no
getenv_vol

exe_base=`basename $1`
if test "x$cache_vol" = xyes && test $exe_base = multi_open ; then
   fmt="####### Skipping %-23s ####################################\n"
   printf "${fmt}" $1
   echo "    Skip 'multi_open' due to a bug in Cache VOL"
   echo "    See https://github.com/hpc-io/vol-cache/pull/17"
   echo ""
   exit 0
fi

log_vol_file_only=1

test_func $1

