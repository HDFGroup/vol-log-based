#!/bin/sh
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

# Exit immediately if a command exits with a non-zero status.
set -e

outfile=`basename $1`

# Some files are reused by other tests, so we can't switch VOL
# ensure these 2 environment variables are not set
# unset HDF5_VOL_CONNECTOR
# unset HDF5_PLUGIN_PATH

# ${TESTSEQRUN} ./$1 ${TESTOUTDIR}/$outfile.h5

export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}" 
export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"

${TESTSEQRUN} ./$1 ${TESTOUTDIR}/$outfile.h5
