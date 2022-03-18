#!/bin/sh
#
# Copyright (C) 2003, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

# Exit immediately if a command exits with a non-zero status.
set -e

# overwrite srcdir, as some netcdf4 test program use it
export srcdir=.

outfile=`basename $1`
outfile=${outfile}.nc

export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}" 
export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"

${TESTSEQRUN} $1 $outfile

err=0
unset HDF5_VOL_CONNECTOR
unset HDF5_PLUGIN_PATH
FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k $outfile`
if test "x${FILE_KIND}" != xHDF5-LogVOL ; then
   echo "Error: Output file $outfile is not Log VOL, but ${FILE_KIND}"
   err=1
else
   echo "Success: Output file $outfile is ${FILE_KIND}"
fi
exit $err
