#!/bin/bash -l
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

# check if Log, Cache, Async VOLs set in the env HDF5_VOL_CONNECTOR
getenv_vol() {
   set +e
   if test "x${HDF5_VOL_CONNECTOR}" != x ; then
      err=`echo "${HDF5_VOL_CONNECTOR}" | ${EGREP} -- "under_vol=512"`
      if test "x$?" = x0 ; then
         async_vol=yes
      fi
      err=`echo "${HDF5_VOL_CONNECTOR}" | ${EGREP} -- "async "`
      if test "x$?" = x0 ; then
         async_vol=yes
      fi
      err=`echo "${HDF5_VOL_CONNECTOR}" | ${EGREP} -- "under_vol=513"`
      if test "x$?" = x0 ; then
         cache_vol=yes
      fi
      err=`echo "${HDF5_VOL_CONNECTOR}" | ${EGREP} -- "cache_ext "`
      if test "x$?" = x0 ; then
         cache_vol=yes
      fi
      err=`echo "${HDF5_VOL_CONNECTOR}" | ${EGREP} -- "under_vol=514 "`
      if test "x$?" = x0 ; then
         log_vol=yes
      fi
      err=`echo "${HDF5_VOL_CONNECTOR}" | ${EGREP} -- "LOG "`
      if test "x$?" = x0 ; then
         log_vol=yes
      fi
   fi
   # echo "log_vol=$log_vol cache_vol=$cache_vol async_vol=$async_vol"
}

# $1 -- executable
# $2 -- whether all output file kinds are Log files
# $3 -- output file name
# $4 -- 2nd output file name if there is any
run_func() {
   outfile_kind=HDF5-LogVOL
   if test "x$2" = x || test "x$2" = x0 ; then
      if test "x${log_vol}" = xno ; then
         outfile_kind=HDF5
      fi
   fi
   # set TEST_NATIVE_VOL_ONLY to 1 to completely disable Log VOL
   if test "x$TEST_NATIVE_VOL_ONLY" = x1 ; then
      outfile_kind=HDF5
   fi

   # Exit immediately if a command exits with a non-zero status.
   set -e

   if test "x$3" = x ; then
      _outfile=`basename $1`
      _outfile="${TESTOUTDIR}/$_outfile.h5"
   else
      _outfile="${TESTOUTDIR}/$3"
   fi
   if test "x$4" != x ; then
      _outfile2="${TESTOUTDIR}/$4"
   fi

   EXE_CMD=$RUN_CMD
   if test "x$EXE_CMD" != x ; then
      tokens=( $EXE_CMD )
      tokens[0]=`basename ${tokens[0]}`
      echo "    ${tokens[*]} $1 $_outfile"
   else
      echo "    $1 $_outfile"
   fi
   $RUN_CMD $1 $_outfile

   # disable all VOLs to avoid debug message messing with the output of h5ldump
   saved_HDF5_VOL_CONNECTOR=
   if test "x$HDF5_VOL_CONNECTOR" != x ; then
      saved_HDF5_VOL_CONNECTOR=$HDF5_VOL_CONNECTOR
      unset HDF5_VOL_CONNECTOR
   fi
   for f in ${_outfile} ${_outfile2} ; do
      FILE_KIND=`${TESTSEQRUN} ${top_builddir}/utils/h5ldump/h5ldump -k $f`
      if test "x${FILE_KIND}" != x$outfile_kind ; then
         echo "Error: expecting file kind $outfile_kind but got ${FILE_KIND}"
         exit 1
      else
         fname=`basename $f`
         echo "    Success: output file '$fname' is expected kind ${FILE_KIND}"
      fi
   done
   # restore HDF5_VOL_CONNECTOR for next run
   if test "x$saved_HDF5_VOL_CONNECTOR" != x ; then
      export HDF5_VOL_CONNECTOR=$saved_HDF5_VOL_CONNECTOR
   fi
}

# Test all combinations of Log, Cache, Async, Passthru
# $1 -- executable
# $2 -- output file name
# $3 -- 2nd output file name if there is any
test_func() {
   async_vol=no
   cache_vol=no
   log_vol=no
   getenv_vol
   # echo "log_vol=$log_vol cache_vol=$cache_vol async_vol=$async_vol"

   fmt="####### Running %-24s ####################################\n"
   printf "${fmt}" $1

   if test "x$cache_vol" = xyes || test "x$async_vol" = xyes ; then
      if test "x$cache_vol" = xyes ; then
         if test "x$async_vol" = xyes ; then
            echo "  - Run Log + Cache + Async VOLs ---------------------------------------"
         else
            echo "--- Run Log + Cache VOLs -----------------------------------------------"
         fi
      fi
      # When stacking Log on top of other VOLs, we can only run Log as passthru
      export H5VL_LOG_PASSTHRU=1
      run_func $1 $log_vol_file_only $2 $3
   elif test "x$log_vol" = xyes ; then
      # test when Log is passthru
      echo "  - Run Log VOL as a passthrough connector -----------------------------"
      export H5VL_LOG_PASSTHRU=1
      run_func $1 $log_vol_file_only $2 $3
      # test when Log is terminal
      echo "  - Run Log VOL as a terminal connector --------------------------------"
      unset H5VL_LOG_PASSTHRU
      run_func $1 $log_vol_file_only $2 $3
   else
      # No env is set to use Log VOL

      if test "x$TEST_NATIVE_VOL_ONLY" != x1 ; then
         # First, set the env to test Log VOL
         echo " -- Set HDF5_VOL_CONNECTOR to \"LOG under_vol=0;under_info={}\" ----------"
         export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}"
         export HDF5_PLUGIN_PATH="${top_builddir}/src/.libs"
         log_vol=yes

         # test when Log is passthru
         echo "  - Run Log VOL as a passthrough connector -----------------------------"
         export H5VL_LOG_PASSTHRU=1
         run_func $1 $log_vol_file_only $2 $3
         # test when Log is terminal
         echo "  - Run Log VOL as a terminal connector --------------------------------"
         unset H5VL_LOG_PASSTHRU
         run_func $1 $log_vol_file_only $2 $3
      fi

      # Unset all env to test Log VOL (i.e. test programs will call H5Pset_vol)
      echo " -- Unset HDF5_VOL_CONNECTOR -------------------------------------------"
      unset HDF5_VOL_CONNECTOR
      unset HDF5_PLUGIN_PATH
      log_vol=no

      # test when Log is passthru
      echo "  - Run Log VOL as a passthrough connector -----------------------------"
      export H5VL_LOG_PASSTHRU=1
      run_func $1 $log_vol_file_only $2 $3
      # test when Log is terminal
      echo "  - Run Log VOL as a terminal connector --------------------------------"
      unset H5VL_LOG_PASSTHRU
      run_func $1 $log_vol_file_only $2 $3
   fi
}

