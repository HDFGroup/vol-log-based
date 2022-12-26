#!/bin/bash -l
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

. $top_srcdir/tests/common/wrap_runs.sh

test_example_func() {
   async_vol=no
   cache_vol=no
   log_vol=no
   getenv_vol
   echo "log_vol=$log_vol cache_vol=$cache_vol async_vol=$async_vol"

# FAIL when passthru running non-MPI program
unset H5VL_LOG_PASSTHRU

   # must enable passthru
   # export H5VL_LOG_PASSTHRU=1

   if test "x$cache_vol" = xyes || test "x$async_vol" = xyes ; then
return
      run_func $1 $log_vol_file_only $2 $3
   elif test "x$log_vol" = xyes ; then
      run_func $1 $log_vol_file_only $2 $3
   else
      export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}"
      export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"
      log_vol=yes
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
echo "----------- 1=$1 outfile=$outfile"

test_example_func $1 $outfile $outfile2
exit 0

log_vol=no
cache_vol=no
async_vol=no
if test "x$HDF5_VOL_CONNECTOR" = x ; then
   export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}"
   log_vol=yes
else
   set +e
   if test "x${HDF5_VOL_CONNECTOR}" != x ; then
      err=`echo "${HDF5_VOL_CONNECTOR}" | ${EGREP} -- "under_vol=512"`
      if test "x$?" = x0 ; then
         async_vol=yes
      fi
      err=`echo "${HDF5_VOL_CONNECTOR}" | ${EGREP} -- "under_vol=513"`
      if test "x$?" = x0 ; then
         cache_vol=yes
      fi
      err=`echo "${HDF5_VOL_CONNECTOR}" | ${EGREP} -- "LOG "`
      if test "x$?" = x0 ; then
         log_vol=yes
      fi
   fi
   # echo "log_vol=$log_vol cache_vol=$cache_vol async_vol=$async_vol"
fi
if test "x$HDF5_PLUGIN_PATH" = x ; then
   export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"
fi

if test "x$cache_vol" = xye || test "x$async_vol" = xyes ; then
   export H5VL_LOG_PASSTHRU=1
   test_func $1 $outfile $outfile2
else
   unset H5VL_LOG_PASSTHRU
   test_func $1 $outfile $outfile2
   # CANNOT run second time, because the same attribute will be added again
   # export H5VL_LOG_PASSTHRU=1
   # test_func $1 $outfile $outfile2
fi

