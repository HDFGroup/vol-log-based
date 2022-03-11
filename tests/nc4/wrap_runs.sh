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

# ensure these 2 environment variables are not set
unset HDF5_VOL_CONNECTOR
unset HDF5_PLUGIN_PATH

${TESTSEQRUN} ./$1 ${TESTOUTDIR}/$outfile.h5

export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}" 
export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"

${TESTSEQRUN} ./$1 ${TESTOUTDIR}/$outfile.h5

${top_builddir}/utils/h5ldump/h5ldump $outfile.h5 