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

export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}"
export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"

for p in ${check_PROGRAMS} ; do
   if test "$1" = ./ph5example ; then
      export HDF5_PARAPREFIX=$TESTOUTDIR
      outfile=ParaEg0.h5
      outfile2=ParaEg1.h5
   fi

   test_func $1 $log_vol_file_only $outfile $outfile2
done

