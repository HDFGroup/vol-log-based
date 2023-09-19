#!/bin/bash -l
#
# Copyright (C) 2023, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

# set -x
export CACHE_DIR=${HOME}/CACHE_VOL
export ABT_DIR=${HOME}/Argobots/1.1
export ASYNC_DIR=${HOME}/ASYNC_VOL
export HDF5_DIR=${HOME}/HDF5/1.14.1-2-thread
export HDF5_ROOT=${HDF5_DIR}
export LD_LIBRARY_PATH=${CACHE_DIR}/lib:${ASYNC_DIR}/lib:${ABT_DIR}/lib:${HDF5_DIR}/lib:${LD_LIBRARY_PATH}
export HDF5_USE_FILE_LOCKING=FALSE

check_PROGRAMS="attr \
                dread \
                dset \
                dwrite \
                file \
                fill \
                group \
                memsel \
                multiblockselection \
                multipointselection"

echo ""
echo "-----------------------------------------"
echo "    Test Cache VOL only"
echo "-----------------------------------------"
export HDF5_PLUGIN_PATH=${CACHE_DIR}/lib
export HDF5_VOL_CONNECTOR="cache_ext config=${PWD}/cache.cfg;under_vol=0;under_info={}"
echo "HDF5_VOL_CONNECTOR=$HDF5_VOL_CONNECTOR"
echo "HDF5_PLUGIN_PATH=$HDF5_PLUGIN_PATH"

for p in ${check_PROGRAMS} ; do
   CMD="mpiexec -n 4 ./$p"
   echo "$CMD"
   $CMD
done


echo ""
echo "-----------------------------------------"
echo "    Test Cache + Async VOLs"
echo "-----------------------------------------"
export HDF5_PLUGIN_PATH=${CACHE_DIR}/lib:${ASYNC_DIR}/lib
export HDF5_VOL_CONNECTOR="cache_ext config=${PWD}/cache.cfg;under_vol=512;under_info={under_vol=0;under_info={}}"

echo "HDF5_VOL_CONNECTOR=$HDF5_VOL_CONNECTOR"
echo "HDF5_PLUGIN_PATH=$HDF5_PLUGIN_PATH"
for p in ${check_PROGRAMS} ; do
   CMD="mpiexec -n 4 ./$p"
   echo "$CMD"
   $CMD
done

