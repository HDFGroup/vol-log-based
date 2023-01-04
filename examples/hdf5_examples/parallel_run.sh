#!/bin/bash -l
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

RUN_CMD=`echo ${TESTMPIRUN} | ${SED} -e "s/NP/$1/g"`
# echo "MPIRUN = ${MPIRUN}"

. $srcdir/../../tests/common/wrap_runs.sh

log_vol_file_only=1

# These example programs downloaded from HDF5 repo CANNOT run the second time,
# because some programs add the same attributes to the files created by other
# programs run previous.

async_vol=no
cache_vol=no
log_vol=no
getenv_vol
# echo "log_vol=$log_vol cache_vol=$cache_vol async_vol=$async_vol"

for p in ${PAR_TESTS} ; do
   if test "$p" = ph5example ; then
      export HDF5_PARAPREFIX=$TESTOUTDIR
      outfile=ParaEg0.h5
      outfile2=ParaEg1.h5
   fi
   if test "x$cache_vol" = xyes || test "x$async_vol" = xyes ; then
      if test "x$cache_vol" = xyes ; then
         if test "x$async_vol" = xyes ; then
            echo "---- Run Log + Cache + Async VOLs -------------"
         else
            echo "---- Run Log + Cache VOLs ---------------------"
         fi
      fi
      # must enable passthru when stacking Log on top of other VOLs
      export H5VL_LOG_PASSTHRU=1
      run_func ./$p $log_vol_file_only $outfile $outfile2
   elif test "x$log_vol" = xyes ; then
      echo "---- Run Log VOL as a passthrough connector ---------------------"
      export H5VL_LOG_PASSTHRU=1
      run_func ./$p $log_vol_file_only $outfile $outfile2
      echo "---- Run Log VOL as a terminal connector ---------------------"
      unset H5VL_LOG_PASSTHRU
      run_func ./$p $log_vol_file_only $outfile $outfile2
   else
      export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}"
      export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"
      log_vol=yes
      echo "---- Run Log VOL as a passthrough connector ---------------------"
      export H5VL_LOG_PASSTHRU=1
      run_func ./$p $log_vol_file_only $outfile $outfile2
      echo "---- Run Log VOL as a terminal connector ---------------------"
      unset H5VL_LOG_PASSTHRU
      run_func ./$p $log_vol_file_only $outfile $outfile2
   fi
done

