#!/bin/sh
#
# Copyright (C) 2003, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

# Exit immediately if a command exits with a non-zero status.
set -e

outfile=`basename $1`

# export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}" 
# export HDF5_PLUGIN_PATH="../../src/.libs"

${TESTSEQRUN} ./$1 ${TESTOUTDIR}/$outfile.h5