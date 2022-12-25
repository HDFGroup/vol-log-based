#!/bin/sh
#
# Copyright (C) 2003, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

if test "x${HDF5_VOL_CONNECTOR}" != x ; then
   async_vol=`echo "${HDF5_VOL_CONNECTOR}" | ${EGREP} -- "under_vol=512"`
   if test "x$?" = x0 ; then
      async_vol=yes
   fi
   cache_vol=`echo "${HDF5_VOL_CONNECTOR}" | ${EGREP} -- "under_vol=513"`
   if test "x$?" = x0 ; then
      cache_vol=yes
   fi
   log_vol=`echo "${HDF5_VOL_CONNECTOR}" | ${EGREP} -- "LOG "`
   if test "x$?" = x0 ; then
      log_vol=yes
   fi
fi

# Exit immediately if a command exits with a non-zero status.
set -e

err=0
for vol_type in "terminal" "passthru"; do
   outfile=`basename $1`

   export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}" 
   export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"
   if test "x${vol_type}" = xterminal ; then
      outfile="${TESTOUTDIR}/$outfile.h5"
      unset H5VL_LOG_PASSTHRU
   else
      outfile="${TESTOUTDIR}/$outfile-passthru.h5"
      export H5VL_LOG_PASSTHRU=1
   fi

   ${TESTSEQRUN} ./$1 $outfile

   unset HDF5_VOL_CONNECTOR
   unset HDF5_PLUGIN_PATH
   unset H5VL_LOG_PASSTHRU

   if test "x${log_vol}" != xyes ; then
      continue
   fi

   FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k $outfile`
   if test "x${FILE_KIND}" != xHDF5-LogVOL ; then
      echo "Error: (as $vol_type vol) Output file $outfile is not Log VOL, but ${FILE_KIND}"
      err=1
   else
      echo "Success: (as $vol_type vol) Output file $outfile is ${FILE_KIND}"
   fi
done
exit $err
