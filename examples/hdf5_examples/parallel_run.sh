#!/bin/bash -l
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

MPIRUN=`echo ${TESTMPIRUN} | ${SED} -e "s/NP/$1/g"`
# echo "MPIRUN = ${MPIRUN}"
# echo "check_PROGRAMS=${check_PROGRAMS}"

. $srcdir/../../tests/common/wrap_runs.sh

log_vol_file_only=1

# These examples CANNOT run the second time, because some programs add the
# same attributes to the files created by other programs.

echo ""
echo "DISABLE passthru as it fails on non-MPI programs"
unset H5VL_LOG_PASSTHRU
echo ""

# must enable passthru when stacking Log on top of other VOLs
# export H5VL_LOG_PASSTHRU=1

if test "x$HDF5_VOL_CONNECTOR" = x ; then
   # If Log VOL is not set in env HDF5_VOL_CONNECTOR, we set and test it.
   export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}"
   export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"
fi

for p in ${check_PROGRAMS} ; do
   if test "$1" = ./ph5example ; then
      export HDF5_PARAPREFIX=$TESTOUTDIR
      outfile=ParaEg0.h5
      outfile2=ParaEg1.h5
   fi

   test_func $1 $log_vol_file_only $outfile $outfile2
done

