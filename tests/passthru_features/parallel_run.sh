#!/bin/sh
#
# Copyright (C) 2021, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

# Exit immediately if a command exits with a non-zero status.
set -e

MPIRUN=`echo ${TESTMPIRUN} | ${SED} -e "s/NP/1/g"`

# echo "check_PROGRAMS=${check_PROGRAMS}"

# export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}" 
# export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"

# ensure these 2 environment variables are not set
unset HDF5_VOL_CONNECTOR
unset HDF5_PLUGIN_PATH

for p in ${check_PROGRAMS} ; do
    outfile="${TESTOUTDIR}/${p}.h5"
    outfile_regular="${TESTOUTDIR}/${p}_regular.h5"
    outfile_log="${TESTOUTDIR}/${p}_log.h5"
    ${MPIRUN} ./${p} ${outfile_regular} ${outfile_log}
    # if [ "${p}" == "read_from_regular_write_to_log_regular" ]; then
    #     h5diff $outfile_log $outfile_regular
    # fi
done
exit 0

