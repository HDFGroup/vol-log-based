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

for p in ${PAR_TESTS} ; do
    export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}" 
    export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"
    ${MPIRUN} ./${p} ${TESTOUTDIR}/${p}.h5
    
    unset HDF5_VOL_CONNECTOR
    unset HDF5_PLUGIN_PATH
    ${MPIRUN} ./${p} ${TESTOUTDIR}/${p}.h5
done