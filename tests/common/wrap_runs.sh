#!/bin/sh
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

getenv_vol() {
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
}

# $1 -- executable
# $2 -- whether all output file kinds are Log files
# $3 -- output file name
# $4 -- 2nd output file name if there is any
run_func() {
   outfile_kind=HDF5-LogVOL
   if test "x$2" = x || test "x$2" = x0 ; then
      if test "x${log_vol}" = xno ; then
         outfile_kind=HDF5
      fi
   fi

   # Exit immediately if a command exits with a non-zero status.
   set -e

   if test "x$3" = x ; then
      outfile=`basename $1`
      outfile="${TESTOUTDIR}/$outfile.h5"
   else
      outfile="${TESTOUTDIR}/$3"
   fi
   if test "x$4" != x ; then
      outfile2="${TESTOUTDIR}/$4"
   fi

   echo "----------------- ${TESTSEQRUN} ./$1 $outfile"
   ${TESTSEQRUN} ./$1 $outfile

   for f in ${outfile} ${outfile2} ; do
      FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k $f`
      echo "----------------- h5ldump -k $f = $FILE_KIND"
      if test "x${FILE_KIND}" != x$outfile_kind ; then
         echo "Error (as $vol_type VOL): expecting file kind $outfile_kind but got ${FILE_KIND}"
         exit 1
      fi
   done
}

# $1 -- executable
# $2 -- output file name
# $3 -- 2nd output file name if there is any
test_func() {
   async_vol=no
   cache_vol=no
   log_vol=no
   getenv_vol
   echo "log_vol=$log_vol cache_vol=$cache_vol async_vol=$async_vol"

   if test "x$cache_vol" = xyes || test "x$async_vol" = xyes ; then
      # must enable passthru
      export H5VL_LOG_PASSTHRU=1
      run_func $1 $log_vol_file_only $2 $3
   elif test "x$log_vol" = xyes ; then
      # enable passthru
      unset H5VL_LOG_PASSTHRU
      run_func $1 $log_vol_file_only $2 $3
      # enable terminal
      export H5VL_LOG_PASSTHRU=1
      run_func $1 $log_vol_file_only $2 $3
   else
      export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}"
      export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"
      log_vol=yes

      # enable passthru
      unset H5VL_LOG_PASSTHRU
      run_func $1 $log_vol_file_only $2 $3
      # enable terminal
      export H5VL_LOG_PASSTHRU=1
      run_func $1 $log_vol_file_only $2 $3

      unset HDF5_VOL_CONNECTOR
      unset HDF5_PLUGIN_PATH
      log_vol=no

      # enable passthru
      unset H5VL_LOG_PASSTHRU
      run_func $1 $log_vol_file_only $2 $3
      # enable terminal
      export H5VL_LOG_PASSTHRU=1
      run_func $1 $log_vol_file_only $2 $3
   fi
}

