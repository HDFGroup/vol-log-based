#!/bin/sh
#
# Copyright (C) 2003, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

# Exit immediately if a command exits with a non-zero status.
set -e

if test "x${LOG_VOL_TEST_USE_ENV_VARS}" != "xyes" ; then
   unset HDF5_VOL_CONNECTOR
   unset HDF5_PLUGIN_PATH
fi

for vol_type in "terminal" "passthru"; do
   outfile=`basename $1`

   if test "x${vol_type}" = xterminal ; then
      outfile="${TESTOUTDIR}/$outfile.h5"
      unset H5VL_LOG_PASSTHRU_READ_WRITE
   else
      outfile="${TESTOUTDIR}/$outfile-passthru.h5"
      export H5VL_LOG_PASSTHRU_READ_WRITE=1
   fi

   ${TESTSEQRUN} ./$1 $outfile
   unset H5VL_LOG_PASSTHRU_READ_WRITE

   err=0
   FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k $outfile`
   if test "x${FILE_KIND}" != xHDF5-LogVOL ; then
      echo "Error (as $vol_type VOL): Output file $outfile is not Log VOL, but ${FILE_KIND}"
      err=1
   # else
   #    echo "Success (as $vol_type VOL): Output file $outfile is ${FILE_KIND}"
   fi
done
exit $err
