#!/bin/sh
#
# Copyright (C) 2003, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

# Exit immediately if a command exits with a non-zero status.
set -e

for vol_type in "terminal" "passthru"; do
   echo "Testing as a ${vol_type} VOL"
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
done
exit $err
