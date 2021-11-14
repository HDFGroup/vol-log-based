dnl
dnl Copyright (C) 2019, Northwestern University and Fermi National Accelerator Laboratory
dnl See COPYRIGHT notice in top-level directory.
dnl

dnl -*- Mode: shell-script-mode; -*-
dnl Process this file with GNU autoconf(1) to produce a configure script.
dnl

dnl autoconf v2.69 was released in 2012-04-24
AC_PREREQ([2.69])
AC_INIT([H5VL_log],[1.0.0],[],[H5VL_log],[])

AM_EXTRA_RECURSIVE_TARGETS([tests])

AC_CONFIG_HEADERS([config.h])
AH_TOP([/*
 * Copyright (C) 2021, Northwestern University and Argonne National Laboratory
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

AH_TEMPLATE([ENABLE_ZLIB],              [Define if to enable zlib compression method])
AH_TEMPLATE([ENABLE_ZLIB],              [Define if to enable zlib compression method])

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

AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
                    [Enable H5VL_log internal debug mode.
                     @<:@default: disabled@:>@])],
    [debug=${enableval}], [debug=no]
)
AM_CONDITIONAL(LOGVOL_DEBUG, [test "x$debug" = xyes])

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

   AC_DEFINE(LOGVOL_DEBUG, 1, ["Debug build"])
fi

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

AC_ARG_WITH([hdf5],
   [AS_HELP_STRING([--with-hdf5=/path/to/hdf5], [Specify HDF5 installation path(s):
    --with-hdf5=INC,LIB for include directory and library directory separated by a comma
    --with-hdf5=DIR for directory containing include/ and lib/ subdirectories
   ])], [
   case $withval in
     *,*)
        hdf5_inc="`echo $withval |cut -f1 -d,`"
        hdf5_lib="`echo $withval |cut -f2 -d, -s`"
        ;;
     *)
        if test -n "$withval"; then
          hdf5_inc="$withval/include"
          hdf5_lib="$withval/lib"
        fi
        ;;
   esac
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
AC_MSG_CHECKING([whether HDF5 version is 1.12.0 (develop branch) or later])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <hdf5.h>
#if (H5_VERS_MAJOR*1000000 + H5_VERS_MINOR*1000 + H5_VERS_RELEASE < 1012000)
#error HDF5 version is older than 1.12.0
#endif
   ]])], [hdf5_ge_1_13_0=yes], [hdf5_ge_1_13_0=no])
AC_MSG_RESULT([$hdf5_ge_1_13_0])
if test x$hdf5_ge_1_13_0 = xno; then
   AC_MSG_ERROR([
   -----------------------------------------------------------------------
     H5VL_log requires HDF5 1.12.0 and later. Abort.
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
   
   if test "x${ZLIB_INSTALL}" != x ; then
      CPPFLAGS+=" -I${ZLIB_INSTALL}/include"
      LDFLAGS+=" -L${ZLIB_INSTALL}/lib"
      LIBS+=" -lz"
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
fi

AC_ARG_VAR(TESTOUTDIR, [Output file directory for "make check" and "make ptest", @<:@default: ./@:>@])
if test "x${TESTOUTDIR}" = x ; then
   dnl set default to current directory
   TESTOUTDIR=.
fi

AC_CONFIG_FILES(Makefile \
                src/Makefile \
                src/H5VL_log.h \
                test/Makefile \
                test/basic/Makefile \
                utils/Makefile \
                examples/Makefile \
                examples/hdf5/Makefile \
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

   Now run 'make' to build the executable.

------------------------------------------------------------------------------"

