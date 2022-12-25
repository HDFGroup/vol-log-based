#!/bin/sh
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

source $srcdir/../common/wrap_runs.sh

log_vol_only=0

export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}"
export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"
test_func $1 $log_vol_only

unset HDF5_VOL_CONNECTOR
unset HDF5_PLUGIN_PATH
test_func $1 $log_vol_only

