#!/bin/bash
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

# Exit immediately if a command exits with a non-zero status.
set -e

export LD_LIBRARY_PATH=${HDF5_LIB_PATH}:${LOGVOL_LIB_PATH}:${LD_LIBRARY_PATH}

EXEC="build/bin/restart"
if test "x$#" = x0 ; then
    RUN=""
else
    RUN="${TESTMPIRUN}"
fi

echo "${RUN} ${EXEC} -g \"8 8 8\" > restart.log"
${RUN} ${EXEC} -g "8 8 8" > restart.log