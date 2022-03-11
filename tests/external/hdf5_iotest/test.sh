#!/bin/bash
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

# Exit immediately if a command exits with a non-zero status.
set -e

source ${top_builddir}/utils/h5lenv.bash

EXEC="./hdf5_iotest"
if test "x$#" = x0 ; then
    RUN=""
    NP=1
else
    RUN=`echo ${TESTMPIRUN} | ${SED} -e "s/NP/$1/g"`
    NP=$1
fi

${M4} -D NP=${NP} hdf5_iotest.m4 > hdf5_iotest.ini

echo "${RUN} ${EXEC} > hdf5_iotest.log"
${RUN} ${EXEC} > hdf5_iotest.log

${top_builddir}/utils/h5ldump/h5ldump hdf5_iotest.h5