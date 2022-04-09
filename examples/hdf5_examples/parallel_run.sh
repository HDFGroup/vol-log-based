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

err=0
for p in ${PAR_TESTS} ; do
   if test "x$p" = "xph5example" ; then
      outfile=${TESTOUTDIR}/ParaEg0.h5
      outfile2=${TESTOUTDIR}/ParaEg1.h5

      export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}"
      export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"
      echo "*** TESTING $p ***"
      ${MPIRUN} ./${p} -c -f ${TESTOUTDIR}

      unset HDF5_VOL_CONNECTOR
      unset HDF5_PLUGIN_PATH
      FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k ${outfile}`
      if test "x${FILE_KIND}" != xHDF5-LogVOL ; then
         echo "Error: Output file $outfile is not Log VOL, but ${FILE_KIND}"
         err=1
      else
         echo "Success: Output file $outfile is ${FILE_KIND}"
      fi
      FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k ${outfile2}`
      if test "x${FILE_KIND}" != xHDF5-LogVOL ; then
         echo "Error: Output file $outfile2 is not Log VOL, but ${FILE_KIND}"
         err=1
      else
         echo "Success: Output file $outfile is ${FILE_KIND}"
      fi
   fi
done
exit $err
