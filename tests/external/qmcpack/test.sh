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

export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}"
export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"

echo "${RUN} ${EXEC} -g \"1 1 1\" > restart.log"
${RUN} ${EXEC} -g "1 1 1" > restart.log

unset HDF5_VOL_CONNECTOR
unset HDF5_PLUGIN_PATH

err=0
for outfile in restart.config.h5 restart.random.h5 ; do
    FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k $outfile`
    if test "x${FILE_KIND}" != xHDF5-LogVOL ; then
       echo "Error: Output file $outfile is not Log VOL, but ${FILE_KIND}"
       err=1
    else
       echo "Success: Output file $outfile is ${FILE_KIND}"
    fi
done
exit $err

