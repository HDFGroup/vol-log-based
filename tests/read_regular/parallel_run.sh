#!/bin/bash -l
#
# Copyright (C) 2021, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

# Exit immediately if a command exits with a non-zero status.
set -e

MPIRUN=`echo ${TESTMPIRUN} | ${SED} -e "s/NP/1/g"`

# ensure these 2 environment variables are not set
unset HDF5_VOL_CONNECTOR
unset HDF5_PLUGIN_PATH

err=0
for p in ${check_PROGRAMS} ; do
    for vol_type in "terminal" "passthru"; do
        outfile_regular="${TESTOUTDIR}/${p}-${vol_type}-native.h5"
        outfile_log="${TESTOUTDIR}/${p}-${vol_type}-logvol.h5"

        if test "x${vol_type}" = xterminal ; then
	   if test "x${cache_vol}" = xyes || test "x${async_vol}" = xyes ; then
              continue
           fi
           unset H5VL_LOG_PASSTHRU
        else
           export H5VL_LOG_PASSTHRU=1
        fi

        ${MPIRUN} ./${p} ${outfile_regular} ${outfile_log} > ${p}-${vol_type}.stdout.log
        unset H5VL_LOG_PASSTHRU

        FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k ${outfile_log}`
        if test "x${FILE_KIND}" != xHDF5 ; then
            echo "Error: (as $vol_type vol) Output file ${outfile_log} is not HDF5, but ${FILE_KIND}"
            err=1
            continue
        fi
        FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k ${outfile_regular}`
        if test "x${FILE_KIND}" != xHDF5 ; then
            echo "Error: (as $vol_type vol) Output file ${outfile_regular} is not HDF5, but ${FILE_KIND}"
            err=1
            continue
        fi

        if test "x${H5DIFF_PATH}" != x ; then
           h5diff_result=`${H5DIFF_PATH} -q ${outfile_regular} ${outfile_log}`
           if test "x${h5diff_result}" = x ; then
              echo "Success: (as $vol_type vol) ${outfile_regular} and ${outfile_log} are same"
           else
              echo "Success: (as $vol_type vol) ${outfile_regular} and ${outfile_log} are not same"
              err=1
           fi
        fi
    done
done
exit $err

