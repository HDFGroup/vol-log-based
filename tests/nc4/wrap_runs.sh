#!/bin/bash -l
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

if test "x$cache_vol" = xyes || test "x$async_vol" = xyes ; then
   # tests in this folder is not ready for stacking Log VOL on top of others
   exit 0
fi

# Exit immediately if a command exits with a non-zero status.
set -e

err=0

H5LREPLAY=${top_builddir}/utils/h5lreplay/h5lreplay

# overwrite srcdir, as some netcdf4 test program use it
export srcdir=.

outfile=`basename $1`
outfile=${outfile}.nc

export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}" 
export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"

${TESTSEQRUN} $1 $outfile

${NCDUMP} ${outfile} > ${outfile}.dump

err=0
unset HDF5_VOL_CONNECTOR
unset HDF5_PLUGIN_PATH
if test "x${log_vol}" = xyes ; then
   FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k $outfile`
   if test "x${FILE_KIND}" != xHDF5-LogVOL ; then
      echo "Error: Output file $outfile is not Log VOL, but ${FILE_KIND}"
      err=1
   else
      ${H5LREPLAY} -i ${outfile} -o ${outfile}_replay.h5
      echo "Success: Output file $outfile is ${FILE_KIND}"
   fi
fi

exit $err
