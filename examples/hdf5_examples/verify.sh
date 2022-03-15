#!/bin/sh
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

# Exit immediately if a command exits with a non-zero status.
set -e

unset HDF5_VOL_CONNECTOR
unset HDF5_PLUGIN_PATH

for f in ./*.h5; do
    ${top_builddir}/utils/h5ldump/h5ldump $f
done