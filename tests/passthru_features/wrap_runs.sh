#!/bin/sh
#
# Copyright (C) 2003, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

# Exit immediately if a command exits with a non-zero status.
set -e

outfile=`basename $1`
outfile_regular="${TESTOUTDIR}/${outfile}_regular.h5"
outfile_log="${TESTOUTDIR}/${outfile}_log.h5"

# export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}" 
# export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"

# ensure these 2 environment variables are not set
unset HDF5_VOL_CONNECTOR
unset HDF5_PLUGIN_PATH

${TESTSEQRUN} ./$1 ${outfile_regular} ${outfile_log}

exit 0
# err=0
# FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k $outfile_regular`
# if test "x${FILE_KIND}" != xHDF5-LogVOL ; then
#    echo "Error: Output file $outfile_regular is not Log VOL, but ${FILE_KIND}"
#    err=1
# else
#    echo "Success: Output file $outfile_regular is ${FILE_KIND}"
# fi

# FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k $outfile_log`
# if test "x${FILE_KIND}" != xHDF5-LogVOL ; then
#    echo "Error: Output file $outfile_log is not Log VOL, but ${FILE_KIND}"
#    err=1
# else
#    echo "Success: Output file $outfile_log is ${FILE_KIND}"
# fi

# if $outfile_regular != "read_from_regular_write_to_log_regular.h5"; then
    # h5diff $outfile_log $outfile_regular
# fi

# exit $err

