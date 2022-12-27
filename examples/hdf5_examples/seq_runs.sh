#!/bin/bash -l
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

RUN_CMD=${TESTSEQRUN}

. $top_srcdir/tests/common/wrap_runs.sh

test_example_func() {
   async_vol=no
   cache_vol=no
   log_vol=no
   getenv_vol
   echo "log_vol=$log_vol cache_vol=$cache_vol async_vol=$async_vol"

   # These examples CANNOT run the second time, because some programs add the
   # same attributes to the files created by other programs.

   # must enable passthru when stacking Log VOL on top of other VOLs
   export H5VL_LOG_PASSTHRU=1

   if test "x$cache_vol" = xyes || test "x$async_vol" = xyes ; then
      if test "x$cache_vol" = xyes ; then
         if test "x$async_vol" = xyes ; then
            echo "---- Run Log + Cache + Async VOLs -------------"
         else
            echo "---- Run Log + Cache VOLs ---------------------"
         fi
      fi
      run_func $1 $log_vol_file_only $2 $3
   elif test "x$log_vol" = xyes ; then
      echo "---- Run Log VOL as a passthrough connector ---------------------"
      run_func $1 $log_vol_file_only $2 $3
   else
      export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}"
      export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"
      log_vol=yes
      echo "---- Run Log VOL as a passthrough connector ---------------------"
      run_func $1 $log_vol_file_only $2 $3
   fi
}

log_vol_file_only=1

if test "$1" = ./h5_crtdat || test "$1" = ./h5_crtatt  || test "$1" = ./h5_rdwt ; then
   outfile=dset.h5
elif test "$1" = ./h5_crtgrp ; then
   outfile=group.h5
elif test "$1" = ./h5_crtgrpar || test "$1" = ./h5_crtgrpd ; then
   outfile=groups.h5
elif test "$1" = ./h5_interm_group ; then
   outfile=interm_group.h5
elif test "$1" = ./h5_write || test "$1" = ./h5_read ; then
   outfile=SDS.h5
elif test "$1" = ./h5_select ; then
   outfile=Select.h5
elif test "$1" = ./h5_subset ; then
   outfile=subset.h5
elif test "$1" = ./h5_attribute ; then
   outfile=Attributes.h5
elif test "$1" = ./ph5example ; then
   export HDF5_PARAPREFIX=$TESTOUTDIR
   outfile=ParaEg0.h5
   outfile2=ParaEg1.h5
fi

test_example_func $1 $outfile $outfile2

