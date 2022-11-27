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
for p in ${check_PROGRAMS} ; do
   for vol_type in "terminal" "passthru"; do
      export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}" 
      export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"
      if test "x${vol_type}" = xterminal ; then
         outfile="${TESTOUTDIR}/${p}.h5"
         unset H5VL_LOG_PASSTHRU_READ_WRITE
      else
         outfile="${TESTOUTDIR}/${p}-passthru.h5"
         export H5VL_LOG_PASSTHRU_READ_WRITE=1
      fi

      ${MPIRUN} ./${p} ${outfile}

      unset HDF5_VOL_CONNECTOR
      unset HDF5_PLUGIN_PATH
      unset H5VL_LOG_PASSTHRU_READ_WRITE

      FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k $outfile`
      if test "x${FILE_KIND}" != xHDF5-LogVOL ; then
         echo "Error (as $vol_type vol): Output file $outfile is not Log VOL, but ${FILE_KIND}"
         err=1
      else
         echo "Success (as $vol_type vol): Output file $outfile is ${FILE_KIND}"
      fi
   done
done

if test "x${TCACHEVOL}" = "xyes" ; then
   export HDF5_PLUGIN_PATH="${CACHE_LIBS}:${ASYNC_LIBS}:${top_builddir}/src/.libs"
   export LD_LIBRARY_PATH="${ABT_LIBS}:${HDF5_PLUGIN_PATH}:${LD_LIBRARY_PATH}"
	export HDF5_VOL_CONNECTOR="LOG under_vol=513;under_info={config=cache_1.cfg;under_vol=512;under_info={under_vol=0;under_info={}}}"
   export H5VL_LOG_PASSTHRU_READ_WRITE=1

   ${TESTSEQRUN} ./$1 $outfile

   unset HDF5_VOL_CONNECTOR
   unset HDF5_PLUGIN_PATH
   unset H5VL_LOG_PASSTHRU_READ_WRITE
   
   err=0
   FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k $outfile`
   if test "x${FILE_KIND}" != xHDF5-LogVOL ; then
      echo "Error: (as $vol_type vol) Output file $outfile is not Log VOL, but ${FILE_KIND}"
      err=1
      break
   else
      echo "Success: (as $vol_type vol) Output file $outfile is ${FILE_KIND}"
   fi
fi

exit $err

