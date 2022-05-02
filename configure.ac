dnl
dnl Copyright (C) 2012, Northwestern University and Argonne National Laboratory
dnl See COPYRIGHT notice in top-level directory.
dnl

dnl -*- Mode: shell-script-mode; -*-
dnl Process this file with GNU autoconf(1) to produce a configure script.
dnl

dnl autoconf v2.69 was released in 2012-04-24
AC_PREREQ([2.69])
AC_INIT([H5VL_log],[1.3.0],[],[H5VL_log],[])

AM_EXTRA_RECURSIVE_TARGETS([tests])

AC_CONFIG_HEADERS([config.h])
AH_TOP([/*
 * Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 * See COPYRIGHT notice in top-level directory.
 */
])
AC_CONFIG_SRCDIR([src/H5VL_log.h.in])
AC_CONFIG_MACRO_DIRS([m4])

AC_CONFIG_AUX_DIR([./scripts])

AM_INIT_AUTOMAKE([foreign])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE([enable])

dnl parse the version numbers to 3 env variables
H5VL_LOG_VERSION_MAJOR=`echo ${PACKAGE_VERSION} | cut -d. -f1`
H5VL_LOG_VERSION_MINOR=`echo ${PACKAGE_VERSION} | cut -d. -f2`
H5VL_LOG_VERSION_SUB=`echo ${PACKAGE_VERSION} | cut -d. -f3`
H5VL_LOG_VERSION=${PACKAGE_VERSION}

AC_DEFUN([UD_MSG_DEBUG],
         [if test "x${pnc_ac_debug}" = xyes ; then
             AC_MSG_NOTICE([DEBUG: $1])
          fi
         ]
)

UD_MSG_DEBUG([H5VL_LOG_VERSION_MAJOR=$H5VL_LOG_VERSION_MAJOR])
UD_MSG_DEBUG([H5VL_LOG_VERSION_MINOR=$H5VL_LOG_VERSION_MINOR])
UD_MSG_DEBUG([H5VL_LOG_VERSION_SUB=$H5VL_LOG_VERSION_SUB])
UD_MSG_DEBUG([H5VL_LOG_VERSION_PRE=$H5VL_LOG_VERSION_PRE])
UD_MSG_DEBUG([H5VL_LOG_VERSION=$H5VL_LOG_VERSION])

AC_SUBST(H5VL_LOG_VERSION_MAJOR)
AC_SUBST(H5VL_LOG_VERSION_MINOR)
AC_SUBST(H5VL_LOG_VERSION_SUB)
AC_SUBST(H5VL_LOG_VERSION)

dnl Note that command 'date' is not portable across Unix platforms.
dnl But release date matters only to H5VL_log developers who make the releases.
H5VL_LOG_RELEASE_DATE="`date '+%B %-d, %Y'`"
AC_SUBST(H5VL_LOG_RELEASE_DATE)
H5VL_LOG_RELEASE_DATE_FULL="`date '+%Y-%m-%d'`"
AC_SUBST(H5VL_LOG_RELEASE_DATE_FULL)
AC_SUBST(PACKAGE_VERSION)

AH_TEMPLATE([ENABLE_ZLIB], [Define if to enable zlib compression method])

AH_TOP([#ifndef _CONFIG_H
#define _CONFIG_H])
AH_BOTTOM([#endif])

AC_PROG_SED
dnl check sed command option -i and set SED_I (this requires RM defined)
if test "x$RM" = x ; then
   RM=rm
fi
UD_PROG_SED_I
AC_PROG_EGREP

UD_PROG_M4

MPI_INSTALL=
AC_ARG_WITH(mpi,
   [AS_HELP_STRING([--with-mpi=/path/to/implementation],
                   [The installation prefix path for MPI implementation.])
   ],[ dnl this clause is run when --with-mpi or --without-mpi is used
   if test "x${withval}" = xno ; then
      AC_MSG_ERROR([
      -----------------------------------------------------------------------
        H5VL_log is built on top of MPI. Configure option --without-mpi or
        --with-mpi=no should not be used. Abort.
      -----------------------------------------------------------------------])
   elif test "x${withval}" = x ; then
      AC_MSG_ERROR(--with-mpi is set but the value is NULL)
   elif test "x${withval}" != xyes && test ! -d "${withval}" ; then
      # user may use --with-mpi without an argument, which results in withval
      # being "yes". This case is OK and we simply take no action, as H5VL_log
      # requires MPI compilers and will check them.
      AC_MSG_ERROR(Directory '${withval}' specified in --with-mpi does not exist or is not a directory)
   fi
   MPI_INSTALL=${withval}
   ]
)

AC_ARG_VAR(MPICC,  [MPI C compiler, @<:@default: CC@:>@])
AC_ARG_VAR(MPICXX, [MPI C++ compiler, @<:@default: CXX@:>@])

ac_user_MPICC=$MPICC
ac_user_MPICXX=$MPICXX
if test "x$MPICC"  = x && test "x$CC"  != x ; then ac_user_MPICC=$CC   ; fi
if test "x$MPICXX" = x && test "x$CXX" != x ; then ac_user_MPICXX=$CXX ; fi

CANDIDATE_MPICC="${MPICC} mpicc mpicc_r"
CANDIDATE_MPICXX="${MPICXX} mpicxx mpic++ mpiCC mpcxx mpc++ mpicxx_r mpiCC_r mpcxx_r mpic++_r mpc++_r"
dnl add GNU MPI compilers
CANDIDATE_MPICC="$CANDIDATE_MPICC mpigcc mpgcc mpigcc_r mpgcc_r"
CANDIDATE_MPICXX="$CANDIDATE_MPICXX mpig++ mpg++ mpig++_r mpg++_r"
dnl add IBM MPI compilers
CANDIDATE_MPICC="$CANDIDATE_MPICC mpcc_r mpcc mpixlc_r mpixlc"
CANDIDATE_MPICXX="$CANDIDATE_MPICXX mpCC_r mpCC mpixlcxx_r mpixlcxx mpixlC_r mpixlC"
dnl add IBM BGL MPI compilers
CANDIDATE_MPICC="$CANDIDATE_MPICC blrts_xlc mpxlc_r mpxlc"
CANDIDATE_MPICXX="$CANDIDATE_MPICXX blrts_xlC mpxlC_r mpxlC mpixlc++ mpxlcxx mpxlc++ mpxlCC mpixlc++_r mpxlcxx_r mpxlc++_r mpxlCC_r"
dnl add Fujitsu MPI compilers
CANDIDATE_MPICC="$CANDIDATE_MPICC mpifccpx"
CANDIDATE_MPICXX="$CANDIDATE_MPICXX mpiFCCpx"
dnl add Cray MPI compiler wrappers
CANDIDATE_MPICC="$CANDIDATE_MPICC cc"
CANDIDATE_MPICXX="$CANDIDATE_MPICXX CC"
dnl add Intel MPI compiler wrappers
CANDIDATE_MPICC="$CANDIDATE_MPICC mpiicc icc"
CANDIDATE_MPICXX="$CANDIDATE_MPICXX mpiicpc mpiicxx mpiic++ mpiiCC icpc"
dnl add PGI MPI compiler wrappers
CANDIDATE_MPICC="$CANDIDATE_MPICC mpipgcc mppgcc"
CANDIDATE_MPICXX="$CANDIDATE_MPICXX mpipgCC mppgCC"

dnl find the full path of MPICC from CANDIDATE_MPICC and MPI_INSTALL
if test "x${ac_user_MPICC}" = x ; then
   dnl if MPICC or CC has not been set by users, then search from
   dnl CANDIDATE_MPICC, and find the full path of MPICC
   UD_MPI_PATH_PROGS([MPICC], [$CANDIDATE_MPICC])
else
   dnl check whether user specified MPICC is valid
   UD_MPI_PATH_PROG([MPICC], [$ac_user_MPICC])
fi

if test "x${MPICC}" = x ; then
   if test "x$ac_user_MPICC" = x ; then
      ERR_MSG="No MPI C compiler can be found"
   else
      ERR_MSG="Specified MPI C compiler \"$ac_user_MPICC\" cannot be found"
   fi
   if test "x$MPI_INSTALL" != x ; then
      ERR_MSG="$ERR_MSG under $MPI_INSTALL"
   fi
   AC_MSG_ERROR([
   -----------------------------------------------------------------------
     $ERR_MSG
     H5VL_log requires a working MPI C compiler. Please specify the
     location of an MPI C compiler, either in the MPICC environment
     variable (not CC variable) or through --with-mpi configure flag.
     Abort.
   -----------------------------------------------------------------------])
fi
CC=${MPICC}
AC_PROG_CC

dnl find the full path of MPICXX from CANDIDATE_MPICXX and MPI_INSTALL
if test "x${ac_user_MPICXX}" = x ; then
   dnl if MPICXX or CXX has not been set by users, then search from
   dnl CANDIDATE_MPICXX, and find the full path of MPICXX
   UD_MPI_PATH_PROGS([MPICXX], [$CANDIDATE_MPICXX])
else
   dnl check whether user specified MPICXX is valid
   UD_MPI_PATH_PROG([MPICXX], [$ac_user_MPICXX])
fi
if test "x${MPICXX}" = x ; then
   if test "x$ac_user_MPICXX" = x ; then
      ERR_MSG="No MPI C++ compiler can be found"
   else
      ERR_MSG="Specified MPI C++ compiler \"$ac_user_MPICXX\" cannot be found"
   fi
   if test "x$MPI_INSTALL" != x ; then
      ERR_MSG="$ERR_MSG under $MPI_INSTALL"
   fi
   AC_MSG_ERROR([
   -----------------------------------------------------------------------
     $ERR_MSG
     H5VL_log requires a working MPI C++ compiler. Please specify the
     location of an MPI C++ compiler, either in the MPICXX environment
     variable (not CXX variable) or through --with-mpi configure flag.
     Abort.
   -----------------------------------------------------------------------])
fi
CXX=${MPICXX}
AC_PROG_CXX

dnl Set output variable CPP to a command that runs the C preprocessor.
dnl Some C compilers require -E to be used as C preprocessor.
AC_PROG_CPP

AC_SYS_LARGEFILE
AC_C_CHAR_UNSIGNED
AC_C_BIGENDIAN
AM_CONDITIONAL(IS_BIGENDIAN, [test x$ac_cv_c_bigendian = xyes])
AC_SUBST(ac_cv_c_bigendian)dnl for src/utils/pnetcdf-config.in

dnl check if MPICXX works for basic MPI call: MPI_Comm_rank()
AC_LANG_PUSH(C++)
AC_CHECK_FUNC([MPI_Comm_rank], [],
   dnl maybe -lmpi is needed at link stage
   [AC_SEARCH_LIBS([MPI_Comm_rank], [mpi mpi++ mpich mpichcxx mpi_cxx], [],
                   [AC_MSG_ERROR([
   -----------------------------------------------------------------------
     Invalid MPI compiler specified or detected: "${MPICXX}"
     A working MPI C++ compiler is required. Please specify the location
     of one either in the MPICXX environment variable (not CXX variable)
     or through --with-mpi configure flag. Abort.
   -----------------------------------------------------------------------])
])])

AC_CHECK_FUNC([MPI_File_open], [],
   dnl maybe -lmpi++ is needed at link stage
   [AC_SEARCH_LIBS([MPI_File_open], [mpio], [],
                   [AC_MSG_ERROR([
   -----------------------------------------------------------------------
     The underneath MPI implementation does not support MPI-IO.
     H5VL_log requires MPI-IO support to work properly. Abort.
   -----------------------------------------------------------------------])
])])
AC_LANG_POP(C++)

AC_CHECK_DECL([access], [], [], [[#include <unistd.h>]])
if test "x$ac_cv_have_decl_access" = xyes ; then
   AC_CHECK_FUNCS([access])
fi
AC_CHECK_DECL([unlink], [], [], [[#include <unistd.h>]])
if test "x$ac_cv_have_decl_unlink" = xyes ; then
   AC_CHECK_FUNCS([unlink])
fi

LT_PREREQ([2.4.2])
LT_INIT()

AC_TYPE_SIZE_T
AC_CHECK_SIZEOF(size_t)

AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
                    [Enable H5VL_log internal debug mode.
                     @<:@default: disabled@:>@])],
    [debug=${enableval}], [debug=no]
)
ENABLE_DEBUG=0
if test "x${debug}" = xyes; then
   dnl add -g flag if not presented
   dnl remove all -O and -fast flags
   dnl add -O0 to all flags
   # check exit status of grep command is more portable than using -q
   str_found=`echo "${CXXFLAGS}" | ${EGREP} -- "-g"`
   if test "x$?" != x0 ; then
      CXXFLAGS="$CXXFLAGS -g"
   fi
   CXXFLAGS=`echo $CXXFLAGS | ${SED} 's/-O. *//g' | ${SED} 's/-fast *//g'`
   CXXFLAGS="$CXXFLAGS -O0"
   unset str_found

   ENABLE_DEBUG=1
   AC_DEFINE(LOGVOL_DEBUG, 1, ["Debug build"])
fi
AC_SUBST(ENABLE_DEBUG)
AM_CONDITIONAL(LOGVOL_DEBUG, [test "x$debug" = xyes])

AC_ARG_ENABLE([profiling],
   [AS_HELP_STRING([--enable-profiling],
                   [Enable internal time profiling. @<:@default: disabled@:>@])],
   [enable_profiling=${enableval}], [enable_profiling=no]
)
ENABLE_PROFILING=0
if test "x${enable_profiling}" = xyes; then
   AC_DEFINE(ENABLE_PROFILING, 1, ["Internal profiling"])
   ENABLE_PROFILING=1
fi
AC_SUBST(ENABLE_PROFILING)
AM_CONDITIONAL(LOGVOL_PROFILING, [test "x$enable_profiling" = xyes])

AC_C_BIGENDIAN
AM_CONDITIONAL(IS_BIGENDIAN, [test x$ac_cv_c_bigendian = xyes])
if test "x${ac_cv_c_bigendian}" = xyes; then
   AC_DEFINE(WORDS_BIGENDIAN, 1, ["Underlying platform is big endian"])
fi

AC_ARG_WITH([hdf5],
   [AS_HELP_STRING([--with-hdf5=/path/to/hdf5], [Specify HDF5 installation path(s):
    --with-hdf5=INC,LIB for include directory and library directory separated by a comma
    --with-hdf5=DIR for directory containing include/ and lib/ subdirectories
   ])], [
   case $withval in
     *,*)
        hdf5_inc="`echo $withval |cut -f1 -d,`"
        hdf5_lib="`echo $withval |cut -f2 -d, -s`"
        hdf5_root="${hdf5_lib}/.."
        ;;
     *)
        if test -n "$withval"; then
          hdf5_inc="$withval/include"
          hdf5_lib="$withval/lib"
          hdf5_root="${withval}"
        fi
        ;;
   esac
   hdf5_bindir=${hdf5_root}/bin
   if test "x$hdf5_inc" != x ; then
      if test "x$CPPFLAGS" = x ; then
         CPPFLAGS="-I$hdf5_inc"
      elif ! echo "${CPPFLAGS}" | ${EGREP} -q -w -- "-I$hdf5_inc" ; then
         # append only if not already appear in CPPFLAGS
         CPPFLAGS="$CPPFLAGS -I$hdf5_inc"
      fi
   fi
   if test "x$hdf5_lib" != x ; then
      if test "x$LDFLAGS" = x ; then
         LDFLAGS="-L$hdf5_lib"
      elif ! echo "${LDFLAGS}" | ${EGREP} -q -w -- "-L$hdf5_lib" ; then
         # append only if not already appear in LDFLAGS
         LDFLAGS="$LDFLAGS -L$hdf5_lib"
      fi
   fi
])

# Only HDF5 1.10.x supports compression
AC_LANG_PUSH(C++)
AC_CHECK_HEADER([hdf5.h], [have_hdf5=yes], [have_hdf5=no])
if test "x$have_hdf5" = xno ; then
      AC_MSG_ERROR([
      -----------------------------------------------------------------------
      Missing HDF5-header files 'hdf5.h' required to build H5VL_log. Use
      configure command-line option --with-hdf5=/path/to/implementation
      to specify the location of HDF5 installation. Abort.
      -----------------------------------------------------------------------])
fi
AC_MSG_CHECKING([whether HDF5 version is 1.13.0 (develop branch) or later])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <hdf5.h>
#if (H5_VERS_MAJOR*1000000 + H5_VERS_MINOR*1000 + H5_VERS_RELEASE < 1013000)
#error HDF5 version is older than 1.13.0
#endif
   ]])], [hdf5_ge_1_13_0=yes], [hdf5_ge_1_13_0=no])
AC_MSG_RESULT([$hdf5_ge_1_13_0])
if test x$hdf5_ge_1_13_0 = xno; then
   AC_MSG_ERROR([
   -----------------------------------------------------------------------
     H5VL_log requires HDF5 1.13.0 and later. Abort.
   -----------------------------------------------------------------------])
fi
AC_MSG_CHECKING([whether HDF5 parallel I/O is enabled])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <hdf5.h>
#if !defined(H5_HAVE_PARALLEL) || H5_HAVE_PARALLEL != 1
#error Parallel I/O is not enabled in HDF5
#endif
]])], [hdf5_parallel_io=yes], [hdf5_parallel_io=no])
AC_MSG_RESULT([$hdf5_parallel_io])
if test "x$hdf5_parallel_io" = xno ; then
   AC_MSG_ERROR([
   -----------------------------------------------------------------------
     H5VL_log requires parallel I/O feature enabled in HDF5. Abort.
   -----------------------------------------------------------------------])
fi

if test "x${hdf5_bindir}" != x ; then
   AC_PATH_PROG([h5diff_path],[h5diff],,[$hdf5_bindir])
else
   dnl Check netcdf4-config under $PATH
   AC_PATH_PROG([h5diff_path],[h5diff])
fi
AC_ARG_VAR(TESTOUTDIR, [Output file directory for "make check" and "make ptest", @<:@default: ./@:>@])
if test "x$h5diff_path" != x ; then
   AC_SUBST(H5DIFF_PATH, [$h5diff_path])
fi
AM_CONDITIONAL(HAVE_H5DIFF, [test "x$h5diff_path" != x])

# Check for NETCDF4
have_netcdf4=no
AC_ARG_ENABLE([test-netcdf4],
   [AS_HELP_STRING([--enable-test-netcdf4@<:@=INC,LIB | =DIR@:>@],
                   [Enable NetCDF4 tests and provide the NetCDF installation path(s):
    --enable-test-netcdf4=INC,LIB for include and lib paths separated by a comma.
    --enable-test-netcdf4=DIR for the path containing include/ and lib/ subdirectories.
    @<:@default: disabled@:>@
   ])], [
   if test "x${enableval}" != xyes && test "x${enableval}" != xno ; then
      NETCDF4_INSTALL=${enableval}
   fi
   case $enableval in
     *,*)
        netcdf4_inc="`echo $enableval |cut -f1 -d,`"
        netcdf4_lib="`echo $enableval |cut -f2 -d, -s`"
        netcdf4_bindir="$netcdf4_inc/../bin"
        ;;
     *)
        if test -n "$enableval"; then
          netcdf4_inc="$enableval/include"
          netcdf4_lib="$enableval/lib"
          netcdf4_bindir="$NETCDF4_INSTALL/bin"
        fi
        ;;
   esac
   if test "x$NETCDF4_INSTALL" != x ; then
      AC_PATH_PROG([netcdf4_config],[nc-config],,[$netcdf4_bindir])
   else
      dnl Check netcdf4-config under $PATH
      AC_PATH_PROG([netcdf4_config],[nc-config])
   fi
   if test "x$netcdf4_config" != x ; then
      netcdf4_cflags=`$netcdf4_config --cflags`
      netcdf4_lflags=`$netcdf4_config --libs`
   else
      netcdf4_cflags="-I$netcdf4_inc"
      netcdf4_lflags="-L$netcdf4_lib"
   fi
   if test "x$netcdf4_cflags" != x ; then
      if test "x$CPPFLAGS" = x ; then
         CPPFLAGS="${netcdf4_cflags}"
      elif ! echo "${CPPFLAGS}" | ${EGREP} -q -w -- "${netcdf4_cflags}" ; then
         # append only if not already appear in CPPFLAGS
         CPPFLAGS="$CPPFLAGS ${netcdf4_cflags}"
      fi
   fi
   if test "x$netcdf4_lflags" != x ; then
      if test "x$LDFLAGS" = x ; then
         LDFLAGS="$netcdf4_lflags"
      elif ! echo "${LDFLAGS}" | ${EGREP} -q -w -- "${netcdf4_lflags}" ; then
         # append only if not already appear in LDFLAGS
         LDFLAGS="$LDFLAGS $netcdf4_lflags"
      fi
   fi
   AC_CHECK_HEADER([netcdf.h], [have_netcdf4=yes], [have_netcdf4=no])
   if test "x$have_netcdf4" = xno ; then
      AC_MSG_WARN([
      -----------------------------------------------------------------------
      Missing NetCDF-header files 'netcdf4_c.h' required to build NetCDF4 test 
      programs. Use configure command-line option 
      --enable-test-netcdf4=/path/to/implementation to specify the location 
      of NetCDF installation. Disable NetCDF4 tests.
      -----------------------------------------------------------------------])
   else
      AC_MSG_CHECKING([NetCDF-C version])
      # MACRO_FLAG is the CPP flag to show macro definitions. For GCC, Intel, and
      # PGI, it is -dM and prints to stdout. For Oracle Solaris Studio compiler,
      # it is -xdumpmacros=defs and prints to stderr
      MACRO_FLAG="-dM"
      if test "x$ax_cv_c_compiler_vendor" = xibm ; then
         MACRO_FLAG="-qshowmacros"
      fi
      saved_CPPFLAGS=$CPPFLAGS
      CPPFLAGS="$CPPFLAGS $MACRO_FLAG"
      AC_PREPROC_IFELSE([AC_LANG_PROGRAM([[#include <netcdf_meta.h>]], [])],
                        [`cp conftest.i saved_conftest.i`])
      CPPFLAGS=$saved_CPPFLAGS
      unset MACRO_FLAG
      nc_version=`${GREP} 'NC_VERSION[[ \t]]' saved_conftest.i | cut -d' ' -d'"' -f2`
      if test "x$nc_version" = x ; then
         AC_MSG_RESULT([unknown])
         netcdf_version=main
      else
         AC_MSG_RESULT($nc_version)
         netcdf_version="v$nc_version"
      fi
      AC_SUBST(NETCDF_VERSION, [$netcdf_version])
      rm -f saved_conftest.i

      AC_MSG_CHECKING([if NetCDF $netcdf_version is configured with parallel I/O enabled])
      if test "x$netcdf4_config" != x ; then
         netcdf4_has_par=`$netcdf4_config --has-parallel4`
      else
         AC_CHECK_HEADERS([netcdf_par.h], [netcdf4_has_par=yes], [netcdf4_has_par=no], [[#include <netcdf.h>]])
      fi
      AC_MSG_RESULT($netcdf4_has_par)
      if test "x${netcdf4_has_par}" != xyes; then
         AC_MSG_WARN([
         ------------------------------------------------------------
         NetCDF was not built with parallel NetCDF 4 support.
         Disable NetCDF4 tests. 
         ------------------------------------------------------------])
         have_netcdf4 = no
      fi
   fi
])
AM_CONDITIONAL(ENABLE_NETCDF4, [test "x$have_netcdf4" = xyes])

AC_ARG_ENABLE([zlib],
   [AS_HELP_STRING([--enable-zlib],
                   [Enable zlib compression method support. @<:@default: disabled@:>@])],
   [enable_zlib=${enableval}], [enable_zlib=no]
)

ENABLE_ZLIB=0
if test "x$enable_zlib" = "xyes" ; then
   AC_DEFINE(ENABLE_ZLIB)
   ENABLE_ZLIB=1
fi
AC_SUBST(ENABLE_ZLIB)
AM_CONDITIONAL(ENABLE_ZLIB, [test x$enable_zlib = xyes])

if test "x$enable_zlib" = "xyes" ; then
   ZLIB_INSTALL=""
   AC_ARG_WITH(zlib,
       [AS_HELP_STRING([--with-zlib=/path/to/implementation],
                       [installation prefix for zlib implementation])],
       if test "x${withval}" = xyes; then
          AC_MSG_ERROR(--with-zlib is set but the value is NULL)
       else
          ZLIB_INSTALL=${withval}
       fi
   )
   
   ZLIB_LDFLAGS=
   ZLIB_LIBS=" -lz"
   if test "x${ZLIB_INSTALL}" != x ; then
      CPPFLAGS+=" -I${ZLIB_INSTALL}/include"
      LDFLAGS+=" -L${ZLIB_INSTALL}/lib"
      LIBS+=" -lz"
      ZLIB_LDFLAGS+=" -L${ZLIB_INSTALL}/lib"
   fi

   LIBS+=" -lm -ldl"

   have_zlib=no
   AC_MSG_CHECKING(ZLIB library)
   AC_SEARCH_LIBS([deflate], [z], [have_zlib=yes], [have_zlib=no])
   if test "x${have_zlib}" = xyes; then
      AC_CHECK_HEADERS([zlib.h], [], [have_zlib=no])
   fi

   if test "x${have_zlib}" = xno; then
      AC_MSG_ERROR([
      ------------------------------------------------------------
      The ZLIB library and header file are required to build
      log with ZLIB compression support. Use option
         --with-zlib=/path/to/implementation
      to specify the location of ZLIB build.
      Stopping ...
      Check 'config.log' for more information.
      ------------------------------------------------------------])
   fi

   AC_DEFINE(ENABLE_ZLIB)
fi

AM_CONDITIONAL(ENABLE_SHARED, [test x$enable_shared = xyes])

AC_LANG_POP(C++)

if test "x$LIBS" = x ; then
   LIBS="-lhdf5"
else
   LIBS="$LIBS -lhdf5"
fi

AC_ARG_VAR(TESTSEQRUN, [Run command (on one MPI process) for "make check" on cross-compile environment. Example: "aprun -n 1". @<:@default: none@:>@])

AC_ARG_VAR(TESTMPIRUN, [MPI run command for "make ptest", @<:@default: mpiexec -n NP@:>@])
if test "x${TESTMPIRUN}" = x ; then
   dnl if TESTMPIRUN has not been set by users, then
   dnl set default to "mpiexec -n NP"
   UD_MPI_PATH_PROGS([TESTMPIRUN], [mpiexec mpirun srun aprun])
   if test "x${TESTMPIRUN}" != x ; then
      TESTMPIRUN="$TESTMPIRUN -n NP"
   fi
fi

AC_ARG_VAR(TESTOUTDIR, [Output file directory for "make check" and "make ptest", @<:@default: ./@:>@])
if test "x${TESTOUTDIR}" = x ; then
   dnl set default to current directory
   TESTOUTDIR=.
fi

AC_CHECK_PROGS(download_cmd, wget curl)
if test "x$download_cmd" = x ; then
   AC_MSG_ERROR([
   -----------------------------------------------------------------------
     Either wget or curl are required to download test files. Abort.
   -----------------------------------------------------------------------])
elif test "x$download_cmd" = xwget ; then
   download_cmd+=" -qc"
else # command curl
   download_cmd+=" -sLO"
fi
AC_SUBST(DOWNLOAD_CMD, [$download_cmd])

have_qmcpack=no
AC_ARG_ENABLE([test-qmcpack],
    [AS_HELP_STRING([--enable-test-qmcpack],
                    [Enable qmcpack external test.
                     @<:@default: disabled@:>@])],
    [have_qmcpack=${enableval}], [have_qmcpack=no]
)
if test "x${have_qmcpack}" = xyes; then
   AC_MSG_CHECKING([downloading qmcpack])
   mkdir -p tests/external/qmcpack
   cd tests/external/qmcpack
   rm -rf ./qmcpack
   qmcpack_version=v3.13.0
   rm -f qmcpack.tar.gz
   ${download_cmd} https://github.com/QMCPACK/qmcpack/archive/refs/tags/${qmcpack_version}.tar.gz -O qmcpack.tar.gz
   AC_MSG_RESULT([${qmcpack_version}.tar.gz])
   mkdir qmcpack
   tar -zxf qmcpack.tar.gz -C qmcpack --strip-components=1
   cd ../../..
   AC_CHECK_FILE([tests/external/qmcpack/qmcpack/src/Sandbox/restart.cpp], [have_qmcpack=yes], [have_qmcpack=no])
   if test "x${have_qmcpack}" = xyes; then
      AC_MSG_NOTICE([configuring qmcpack])
      AC_CONFIG_COMMANDS(qmcpack,[cd tests/external/qmcpack && chmod 755 config.sh && ./config.sh && cd ../../..])
   else
      AC_MSG_WARN([
         ------------------------------------------------------------
         Test program source file not found. Disable qmcpack test.
         ------------------------------------------------------------])
   fi
fi
AM_CONDITIONAL(HAVE_QMCPACK, [test "x${have_qmcpack}" = xyes])

have_hdf5io=no
AC_ARG_ENABLE([test-hdf5-iotest],
    [AS_HELP_STRING([--enable-test-hdf5-iotest],
                    [Enable hdf5-iotest external test.
                     @<:@default: disabled@:>@])],
    [have_hdf5io=${enableval}], [have_hdf5io=no]
)
if test "x${have_hdf5io}" = xyes; then
   AC_MSG_CHECKING([downloading hdf5-iotest])
   mkdir -p tests/external/hdf5_iotest
   cd tests/external/hdf5_iotest
   rm -rf ./hdf5-iotest
   git clone -q https://github.com/HDFGroup/hdf5-iotest.git hdf5-iotest
   AC_MSG_RESULT([github main branch])
   cd ../../..
   AC_CHECK_FILE([tests/external/hdf5_iotest/hdf5-iotest/src/hdf5_iotest.c], [have_hdf5io=yes], [have_hdf5io=no])
   if test "x${have_hdf5io}" = xyes; then
      AC_MSG_NOTICE([configuring hdf5-iotest])
      AC_CONFIG_COMMANDS(hdf5_iotest,[cd tests/external/hdf5_iotest && chmod 755 config.sh && ./config.sh && cd ../../..])
      AC_CONFIG_LINKS([tests/external/hdf5_iotest/hdf5_iotest.m4:tests/external/hdf5_iotest/hdf5_iotest.m4])
   else
      AC_MSG_WARN([
         ------------------------------------------------------------
         Test program source file not found. Disable hdf5-iotest test.
         ------------------------------------------------------------])
   fi
fi
AM_CONDITIONAL(HAVE_HDF5_IOTEST, [test "x${have_hdf5io}" = xyes])

have_openpmd=no
AC_ARG_ENABLE([test-openpmd],
    [AS_HELP_STRING([--enable-test-openpmd],
                    [Enable openpmd external test.
                     @<:@default: disabled@:>@])],
    [have_openpmd=${enableval}], [have_openpmd=no]
)
if test "x${have_openpmd}" = xyes; then
   AC_MSG_CHECKING([downloading openpmd])
   mkdir -p tests/external/openpmd
   cd tests/external/openpmd
   rm -rf ./openPMD-api
   openpmd_version=0.14.4
   rm -f openPMD-api.tar.gz
   ${download_cmd} https://github.com/openPMD/openPMD-api/archive/refs/tags/${openpmd_version}.tar.gz -O openPMD-api.tar.gz
   AC_MSG_RESULT([${openpmd_version}.tar.gz])
   mkdir openPMD-api
   tar -zxf openPMD-api.tar.gz -C openPMD-api --strip-components=1
   cd ../../..
   AC_CHECK_FILE([tests/external/openpmd/openPMD-api/examples/8a_benchmark_write_parallel.cpp], [have_openpmd=yes], [have_openpmd=no])
   if test "x${have_openpmd}" = xyes; then
      AC_MSG_NOTICE([configuring openpmd])
      AC_CONFIG_COMMANDS(openpmd,[cd tests/external/openpmd && chmod 755 config.sh && ./config.sh && cd ../../..])
   else
      AC_MSG_WARN([
         ------------------------------------------------------------
         Test program source file not found. Disable openpmd test.
         ------------------------------------------------------------])
   fi
fi
AM_CONDITIONAL(HAVE_OPENPMD, [test "x${have_openpmd}" = xyes])

dnl Configuration Date
dnl Note that command 'date' is not portable across Unix platforms
if test "x$SOURCE_DATE_EPOCH" != x ; then
    date_str=`date -d @${SOURCE_DATE_EPOCH}`
    if test "x$date_str" != x ; then
       AC_SUBST([CONFIG_DATE], ["$date_str"])
    else
       date_str=`date -r $SOURCE_DATE_EPOCH`
       if test "x$date_str" != x ; then
          AC_SUBST([CONFIG_DATE], ["$date_str"])
       else dnl ignore SOURCE_DATE_EPOCH
          AC_SUBST([CONFIG_DATE], ["`date`"])
       fi
    fi
else
    AC_SUBST([CONFIG_DATE], ["`date`"])
fi
UD_MSG_DEBUG([CONFIG_DATE=$CONFIG_DATE])

AC_SUBST(HDF5_LIB_PATH,${hdf5_lib})
AC_SUBST(HDF5_ROOT_PATH,${hdf5_root})

AC_SUBST(INSTALL_PREFIX,${prefix})
AC_SUBST(INSTALL_EXEC_PREFIX,${exec_prefix})
AC_SUBST(PACKAGE_NAME)

dnl Enable creation of libtool-style versioning or no versioning
AC_ARG_ENABLE(versioning,
        [AS_HELP_STRING([--disable-versioning],[Disable library versioning. @<:@default: enabled@:>@])],
        [enable_versioning=${enableval}], [enable_versioning=yes]
)
dnl For libtool ABI versioning rules see:
dnl http://www.gnu.org/software/libtool/manual/libtool.html#Updating-version-info
dnl Update the version information only immediately before a public release.
ABIVERSION="2:0:0"
AC_SUBST(ABIVERSION)
if test "$enable_versioning" = "yes" ; then
   ABIVERSIONFLAGS="-version-info \$(ABIVERSION)"
else
   ABIVERSIONFLAGS="-avoid-version"
fi
AC_SUBST(ABIVERSIONFLAGS)

AC_CONFIG_FILES(Makefile \
                doc/Makefile \
                src/Makefile \
                src/H5VL_log.h \
                tests/Makefile \
                tests/basic/Makefile \
                tests/dynamic/Makefile \
                tests/external/Makefile \
                tests/passthru_features/Makefile \
                tests/external/hdf5_iotest/config.sh \
                tests/external/hdf5_iotest/Makefile \
                tests/external/openpmd/config.sh \
                tests/external/openpmd/Makefile \
                tests/external/qmcpack/config.sh \
                tests/external/qmcpack/Makefile \
                tests/nc4/Makefile \
                tests/testcases/Makefile \
                utils/Makefile \
                utils/h5ldump/Makefile \
                utils/h5lenv.bash \
                utils/h5lenv.csh \
                utils/h5lreplay/Makefile \
                utils/h5lconfig \
                utils/h5lpcc \
                utils/h5lpcxx \
                examples/Makefile \
                examples/hdf5_examples/Makefile
                )

AC_OUTPUT
echo "------------------------------------------------------------------------------"
echo \
"
   ${PACKAGE_NAME} Version ${PACKAGE_VERSION}

   Features:  Internal debug mode         - ${debug}
              Internal profiling mode     - ${enable_profiling}"

echo "\

   Compilers: MPICC    = ${MPICC}
              MPICXX   = ${MPICXX}"
if test "x${CPPFLAGS}" != x ; then
   echo "\
              CPPFLAGS = ${CPPFLAGS}"
fi
echo "\
              CFLAGS   = ${CFLAGS}"
if test "${CXXFLAGS}" != x ; then
   echo "\
              CXXFLAGS = ${CXXFLAGS}"
fi
if test "x${LDFLAGS}" != x ; then
   echo "\
              LDFLAGS  = ${LDFLAGS}"
fi
if test "x${LIBS}" != x ; then
   echo "\
              LIBS     = ${LIBS}"
fi
echo "\

   External test programs:
              hdf5-iotest    - ${have_hdf5io}
              NetCDF-4       - ${have_netcdf4}
              QMCPACK        - ${have_qmcpack}
              OpenPMD        - ${have_openpmd}

   Now run 'make' to build the executable.

------------------------------------------------------------------------------"

