#!/bin/sh
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

test_func() {
   set +e
   log_vol=no
   cache_vol=no
   async_vol=no
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

   test_case="terminal passthru"
   # make Log VOL a passthrough when either cache or async VOL is set
   if test "x${cache_vol}" = xyes || test "x${async_vol}" = xyes ; then
      test_case="passthru"
   fi
   # echo "test_case=$test_case"

   outfile_kind=HDF5-LogVOL
   if test "x$2" = x || test "x$2" = x0 ; then
      if test "x${log_vol}" = xno ; then
         outfile_kind=HDF5
      fi
   fi

   # Exit immediately if a command exits with a non-zero status.
   set -e

   err=0
   for vol_type in ${test_case} ; do
      # echo "vol_type=$vol_type"

      if test "x${vol_type}" = xterminal ; then
         outfile="${TESTOUTDIR}/$1.h5"
         unset H5VL_LOG_PASSTHRU
      else
         outfile="${TESTOUTDIR}/$1-passthru.h5"
         export H5VL_LOG_PASSTHRU=1
      fi

      ${TESTSEQRUN} ./$1 $outfile

      FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k $outfile`
      if test "x${FILE_KIND}" != x$outfile_kind ; then
         echo "Error (as $vol_type VOL): expecting file kind $outfile_kind but got ${FILE_KIND}"
         exit 1
      fi
   done
}

