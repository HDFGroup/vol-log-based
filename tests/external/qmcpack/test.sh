#!/bin/bash
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

# Exit immediately if a command exits with a non-zero status.
set -e

source ${top_builddir}/utils/h5lenv.bash

EXEC="./restart"
if test "x$#" = x0 ; then
    RUN=""
else
    RUN=`echo ${TESTMPIRUN} | ${SED} -e "s/NP/$1/g"`
fi

echo "${RUN} ${EXEC} -g \"1 1 1\" > restart.log"
${RUN} ${EXEC} -g "1 1 1" > restart.log
