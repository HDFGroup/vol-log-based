#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#
# @configure_input@

SUFFIXES = .o .c

AM_DEFAULT_SOURCE_EXT = .c

AM_CPPFLAGS = -I${srcdir}
AM_CPPFLAGS += -I${top_srcdir}
AM_CPPFLAGS += -I${top_srcdir}/src
AM_CPPFLAGS += -I${top_srcdir}/tests
AM_CPPFLAGS += -I${top_builddir}
AM_CPPFLAGS += -I${top_builddir}/src
AM_CPPFLAGS += -DUSE_HDF5
AM_CPPFLAGS += -DHDF5_PARALLEL
LDADD = $(top_builddir)/src/libH5VL_log.la

# The following files will be downloaded from NetCDF4 library
SEQ_SRCS = t_type.c \
           tst_files.c \
           tst_grps.c \
           tst_grps2.c \
           tst_atts.c \
           tst_atts1.c \
           tst_atts2.c \
           tst_atts3.c \
           tst_dims2.c \
           tst_types.c \
           tst_vars3.c

# These 3 need patched to disable fill and filter modes that log-based VOL
# has not supported yet.
SEQ_SRCS_PATCH = tst_dims.c \
                 tst_dims3.c \
                 tst_vars.c

PAR_SRCS =

SEQ_TESTS = $(SEQ_SRCS:.c=) $(SEQ_SRCS_PATCH:.c=)
PAR_TESTS = $(PAR_SRCS:.c=)

TESTPROGRAMS =

if ENABLE_SHARED
if ENABLE_NETCDF4
TESTPROGRAMS += $(SEQ_TESTS) $(PAR_TESTS)

BUILT_SOURCES = $(SEQ_SRCS) \
                $(SEQ_SRCS_PATCH) \
                nc4internal.h \
                hdf5internal.h \
                err_macros.h \
                ref_tst_dims.nc

# Do not include these files in the distribution
nodist_t_type_SOURCES = t_type.c
nodist_tst_files_SOURCES = tst_files.c
nodist_tst_grps_SOURCES = tst_grps.c
nodist_tst_grps2_SOURCES = tst_grps2.c
nodist_tst_atts_SOURCES = tst_atts.c
nodist_tst_atts1_SOURCES = tst_atts1.c
nodist_tst_atts2_SOURCES = tst_atts2.c
nodist_tst_atts3_SOURCES = tst_atts3.c
nodist_tst_dims_SOURCES = tst_dims.c
nodist_tst_dims2_SOURCES = tst_dims2.c
nodist_tst_dims3_SOURCES = tst_dims3.c
nodist_tst_types_SOURCES = tst_types.c
nodist_tst_vars_SOURCES = tst_vars.c
nodist_tst_vars3_SOURCES = tst_vars3.c
endif
endif

NETCDF_VERSION = v4.7.4
NETCDF4_URL = https://raw.githubusercontent.com/Unidata/netcdf-c/$(NETCDF_VERSION)

nc4internal.h:
	touch nc4internal.h

hdf5internal.h:
	touch hdf5internal.h

$(SEQ_SRCS):
	wget -cq $(NETCDF4_URL)/nc_test4/$@

tst_dims.c: tst_dims.patch.$(NETCDF_VERSION)
	wget -cq $(NETCDF4_URL)/nc_test4/$@
	patch -s $@ < $<

tst_dims3.c: tst_dims3.patch.$(NETCDF_VERSION)
	wget -cq $(NETCDF4_URL)/nc_test4/$@
	patch -s $@ < $<

tst_vars.c: tst_vars.patch.$(NETCDF_VERSION)
	wget -cq $(NETCDF4_URL)/nc_test4/$@
	patch -s $@ < $<

err_macros.h:
	wget -cq $(NETCDF4_URL)/include/$@

ref_tst_dims.nc:
	wget -cq $(NETCDF4_URL)/nc_test4/$@

check_PROGRAMS = $(TESTPROGRAMS)

EXTRA_DIST = wrap_runs.sh parallel_run.sh nc_tests.h \
             tst_dims.patch.$(NETCDF_VERSION) \
             tst_dims3.patch.$(NETCDF_VERSION) \
             tst_vars.patch.$(NETCDF_VERSION)

# autimake 1.11.3 has not yet implemented AM_TESTS_ENVIRONMENT
# For newer versions, we can use AM_TESTS_ENVIRONMENT instead
# AM_TESTS_ENVIRONMENT  = export TESTPROGRAMS="$(TESTPROGRAMS)";
# AM_TESTS_ENVIRONMENT += export TESTSEQRUN="$(TESTSEQRUN)";
# AM_TESTS_ENVIRONMENT += export TESTOUTDIR="$(TESTOUTDIR)";
TESTS_ENVIRONMENT  = export SED="$(SED)";
TESTS_ENVIRONMENT += export top_builddir="$(top_builddir)";
TESTS_ENVIRONMENT += export TESTOUTDIR="$(TESTOUTDIR)";
TESTS_ENVIRONMENT += export TESTSEQRUN="$(TESTSEQRUN)";
TESTS_ENVIRONMENT += export TESTMPIRUN="$(TESTMPIRUN)";
TESTS_ENVIRONMENT += export TESTPROGRAMS="$(TESTPROGRAMS)";
TESTS_ENVIRONMENT += export check_PROGRAMS="$(check_PROGRAMS)";
TESTS_ENVIRONMENT += export PAR_TESTS="$(PAR_TESTS)";

H5_FILES = $(check_PROGRAMS:%=$(TESTOUTDIR)/%.h5)

CLEANFILES = $(H5_FILES) core core.* *.gcda *.gcno *.gcov gmon.out *.h5 *.nc  *.log

DISTCLEANFILES = $(SEQ_SRCS) $(SEQ_SRCS_PATCH) nc4internal.h err_macros.h \
                 hdf5internal.h ref_tst_dims.nc

TEST_EXTENSIONS = .sh
LOG_COMPILER = $(srcdir)/wrap_runs.sh
SH_LOG_COMPILER =

TESTS = $(TESTPROGRAMS)

# build check targets but not invoke
tests-local: all $(check_PROGRAMS)

ptest ptest4: $(PAR_TESTS)
	@echo "==========================================================="
	@echo "    $(subdir): Parallel testing on 4 MPI processes"
	@echo "==========================================================="
	$(TESTS_ENVIRONMENT) \
	$(srcdir)/parallel_run.sh 4 || exit 1

ptest2: $(PAR_TESTS)
	@echo "==========================================================="
	@echo "    $(subdir): Parallel testing on 2 MPI processes"
	@echo "==========================================================="
	$(TESTS_ENVIRONMENT) \
	$(srcdir)/parallel_run.sh 2 || exit 1

ptest6: $(PAR_TESTS)
	@echo "==========================================================="
	@echo "    $(subdir): Parallel testing on 6 MPI processes"
	@echo "==========================================================="
	$(TESTS_ENVIRONMENT) \
	$(srcdir)/parallel_run.sh 6 || exit 1

ptests: ptest2 ptest4 ptest6
ptest8 ptest10:

.PHONY: ptest ptests ptest2 ptest4 ptest6 ptest8 ptest10
