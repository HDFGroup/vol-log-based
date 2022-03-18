#!/bin/sh
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

# Exit immediately if a command exits with a non-zero status.
set -e

# Some files are reused by other tests
outfile=`basename $1`

if test "x$outfile" = xh5_crtdat ; then
   outfile=${TESTOUTDIR}/dset.h5
elif test "x$outfile" = xh5_crtatt ; then
   outfile=
elif test "x$outfile" = xh5_crtgrp ; then
   outfile=${TESTOUTDIR}/group.h5
elif test "x$outfile" = xh5_crtgrpar ; then
   outfile=${TESTOUTDIR}/groups.h5
elif test "x$outfile" = xh5_crtgrpd ; then
   outfile=
elif test "x$outfile" = xh5_interm_group ; then
   outfile=${TESTOUTDIR}/interm_group.h5
elif test "x$outfile" = xh5_rdwt ; then
   outfile=
elif test "x$outfile" = xh5_write ; then
   outfile=${TESTOUTDIR}/SDS.h5
elif test "x$outfile" = xh5_read ; then
   outfile=
elif test "x$outfile" = xh5_select ; then
   outfile=${TESTOUTDIR}/Select.h5
elif test "x$outfile" = xh5_subset ; then
   outfile=${TESTOUTDIR}/subset.h5
elif test "x$outfile" = xh5_attribute ; then
   outfile=${TESTOUTDIR}/Attributes.h5
elif test "x$outfile" = xph5example ; then
   outfile=${TESTOUTDIR}/ParaEg0.h5
   outfile2=${TESTOUTDIR}/ParaEg1.h5
fi

err=0
if test "x$1" = "x./ph5example" ; then
   export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}"
   export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"
   ${TESTSEQRUN} ./$1 -c -f ${TESTOUTDIR}

   unset HDF5_VOL_CONNECTOR
   unset HDF5_PLUGIN_PATH
   FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k ${outfile}`
   if test "x${FILE_KIND}" != xHDF5-LogVOL ; then
      echo "Error: Output file $outfile is not Log VOL, but ${FILE_KIND}"
      err=1
   else
      echo "Success: Output file $outfile is ${FILE_KIND}"
   fi
   FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k ${outfile2}`
   if test "x${FILE_KIND}" != xHDF5-LogVOL ; then
      echo "Error: Output file $outfile2 is not Log VOL, but ${FILE_KIND}"
      err=1
   else
      echo "Success: Output file $outfile is ${FILE_KIND}"
   fi
elif test "x$outfile" != x ; then
   export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}"
   export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"
   ${TESTSEQRUN} ./$1

   unset HDF5_VOL_CONNECTOR
   unset HDF5_PLUGIN_PATH
   FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k ${outfile}`
   if test "x${FILE_KIND}" != xHDF5-LogVOL ; then
      echo "Error: Output file $outfile is not Log VOL, but ${FILE_KIND}"
      err=1
   else
      echo "Success: Output file $outfile is ${FILE_KIND}"
   fi
fi
exit $err

