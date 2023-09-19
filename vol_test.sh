#!/usr/bin/bash -l

set -e
ABT_DIR=${HOME}/Argobots/1.1
ASYNC_DIR=${HOME}/ASYNC_VOL
CACHE_DIR=${HOME}/CACHE_VOL
HDF5_DIR=${HOME}/HDF5/1.14.1-2-thread
HDF5_ROOT=${HDF5_DIR}
LOGVOL_DIR=${HOME}/LOG_VOL
CACHE_CONF=${HOME}/LOG_VOL/cache.cfg

export MPICH_MAX_THREAD_SAFETY=multiple
export HDF5_USE_FILE_LOCKING=FALSE
# export TEST_NATIVE_VOL_ONLY=1

CASE="async"
CASE="cache"
CASE="cache+async"
CASE="log"
CASE="log+async"
CASE="log+cache"
CASE="log+cache+async"
CASE="all"

if test $CASE = "async" || test $CASE = "all" ; then
echo "-----------------------------------------------------------"
echo "    Test Async VOL"
echo "-----------------------------------------------------------"
export LD_LIBRARY_PATH=${ASYNC_DIR}/lib:${ABT_DIR}/lib:${HDF5_DIR}/lib
export HDF5_PLUGIN_PATH=${ASYNC_DIR}/lib
export HDF5_VOL_CONNECTOR="async under_vol=0;under_info={}"

make -s check
make -s ptest

fi

if test $CASE = "cache" || test $CASE = "all" ; then
echo "-----------------------------------------------------------"
echo "    Test Cache VOL"
echo "-----------------------------------------------------------"
export LD_LIBRARY_PATH=${CACHE_DIR}/lib:${ASYNC_DIR}/lib:${ABT_DIR}/lib:${HDF5_DIR}/lib
export HDF5_PLUGIN_PATH=${CACHE_DIR}/lib
export HDF5_VOL_CONNECTOR="cache_ext config=${CACHE_CONF};under_vol=0;under_info={}"

make -s check
make -s ptest
fi

if test $CASE = "cache+async" || test $CASE = "all" ; then
echo "-----------------------------------------------------------"
echo "    Test Cache and Async VOL"
echo "-----------------------------------------------------------"
export LD_LIBRARY_PATH=${CACHE_DIR}/lib:${ASYNC_DIR}/lib:${ABT_DIR}/lib:${HDF5_DIR}/lib
export HDF5_PLUGIN_PATH=${CACHE_DIR}/lib:${ASYNC_DIR}/lib
export HDF5_VOL_CONNECTOR="cache_ext config=${CACHE_CONF};under_vol=512;under_info={under_vol=0;under_info={}}"

make -s check
make -s ptest
fi

if test $CASE = "log" || test $CASE = "all" ; then
echo "-----------------------------------------------------------"
echo "    Test Log VOL"
echo "-----------------------------------------------------------"
export HDF5_PLUGIN_PATH=${LOGVOL_DIR}/lib
export LD_LIBRARY_PATH=${LOGVOL_DIR}/lib:${HDF5_DIR}/lib
export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}"

make -s check
make -s ptest
fi

if test $CASE = "log+async" || test $CASE = "all" ; then
echo "-----------------------------------------------------------"
echo "    Test Log VOL + Async VOL"
echo "-----------------------------------------------------------"
export HDF5_PLUGIN_PATH=${LOGVOL_DIR}/lib:${ASYNC_DIR}/lib
export LD_LIBRARY_PATH=${LOGVOL_DIR}/lib:${ASYNC_DIR}/lib:${ABT_DIR}/lib:${HDF5_DIR}/lib
export HDF5_VOL_CONNECTOR="LOG under_vol=512;under_info={under_vol=0;under_info={}}"

make -s check
make -s ptest
fi

if test $CASE = "log+cache" || test $CASE = "all" ; then
echo "-----------------------------------------------------------"
echo "    Test Log VOL + Cache VOL"
echo "-----------------------------------------------------------"
export HDF5_PLUGIN_PATH=${LOGVOL_DIR}/lib:${CACHE_DIR}/lib
export LD_LIBRARY_PATH=${LOGVOL_DIR}/lib:${CACHE_DIR}/lib:${ASYNC_DIR}/lib:${ABT_DIR}/lib:${HDF5_DIR}/lib
export HDF5_VOL_CONNECTOR="LOG under_vol=513;under_info={config=${CACHE_CONF};under_vol=0;under_info={}}"

make -s check
make -s ptest
fi

if test $CASE = "log+cache+async" || test $CASE = "all" ; then
echo "-----------------------------------------------------------"
echo "    Test Log VOL + Cache VOL + Async VOL"
echo "-----------------------------------------------------------"
export HDF5_PLUGIN_PATH=${LOGVOL_DIR}/lib:${CACHE_DIR}/lib:${ASYNC_DIR}/lib
export LD_LIBRARY_PATH=${LOGVOL_DIR}/lib:${CACHE_DIR}/lib:${ASYNC_DIR}/lib:${ABT_DIR}/lib:${HDF5_DIR}/lib
export HDF5_VOL_CONNECTOR="LOG under_vol=513;under_info={config=${CACHE_CONF};under_vol=512;under_info={under_vol=0;under_info={}}}"

make -s check
make -s ptest
fi

