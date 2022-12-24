#!/bin/sh
#
# Copyright (C) 2003, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

# Exit immediately if a command exits with a non-zero status.
set -e
set -x

outfile="test"
H5LREPLAY=${top_builddir}/utils/h5lreplay/h5lreplay
H5LGEN=${top_builddir}/utils/h5lreplay/h5lgen

export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}" 
export HDF5_PLUGIN_PATH="../../src/.libs"

${TESTSEQRUN} ${H5LGEN} ${TESTOUTDIR}/${outfile}.h5l

FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k ${TESTOUTDIR}/${outfile}.h5l`
if test "x${FILE_KIND}" != xHDF5-LogVOL ; then
   echo "Error: Output file ${outfile}.h5l is not Log VOL, but ${FILE_KIND}"
   exit 1
else
   echo "Success: Output file ${outfile}.h5l is ${FILE_KIND}"
fi

# ensure these 2 environment variables are not set
unset HDF5_VOL_CONNECTOR
unset HDF5_PLUGIN_PATH

${TESTSEQRUN} ${H5LGEN} ${TESTOUTDIR}/${outfile}.h5

FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k ${TESTOUTDIR}/${outfile}.h5`
if test "x${FILE_KIND}" != xHDF5 ; then
   echo "Error: Output file ${outfile}.h5 is not Log VOL, but ${FILE_KIND}"
   exit 1
else
   echo "Success: Output file ${outfile}.h5 is ${FILE_KIND}"
fi

${TESTSEQRUN} ./${H5LREPLAY} -i ${TESTOUTDIR}/${outfile}.h5l -o ${TESTOUTDIR}/${outfile}.h5r

FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k ${TESTOUTDIR}/${outfile}.h5r`
if test "x${FILE_KIND}" != xHDF5 ; then
   echo "Error: Output file ${outfile}.h5r is not Log VOL, but ${FILE_KIND}"
   exit 1
else
   echo "Success: Output file ${outfile}.h5r is ${FILE_KIND}"
fi

if test "x$H5DIFF" != x ; then
   ${TESTSEQRUN} ${H5DIFF} ${TESTOUTDIR}/${outfile}.h5 ${TESTOUTDIR}/${outfile}.h5r
   if test "x$?" != x0 ; then
      echo "Error: ${outfile}.h5r differs from ${outfile}.h5"
      exit 1
   else
      echo "Success: ${outfile}.h5r and ${outfile}.h5 are the same"
   fi
fi

exit 0
