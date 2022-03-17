#!/bin/sh
#
# Copyright (C) 2021, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

# Exit immediately if a command exits with a non-zero status.
set -e

MPIRUN=`echo ${TESTMPIRUN} | ${SED} -e "s/NP/$1/g"`
# echo "MPIRUN = ${MPIRUN}"
# echo "check_PROGRAMS=${check_PROGRAMS}"

export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}"
export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"

for p in ${PAR_TESTS} ; do
    # echo ${MPIRUN} ./${p} -c -f ${TESTOUTDIR}
    ${MPIRUN} ./${p} -c -f ${TESTOUTDIR}
done

