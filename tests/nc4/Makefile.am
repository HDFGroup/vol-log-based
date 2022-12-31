#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#
# @configure_input@

SUFFIXES = .o .c

AM_DEFAULT_SOURCE_EXT = .c

AM_CPPFLAGS = -I${srcdir}/../common
AM_CPPFLAGS += -I${top_srcdir}
AM_CPPFLAGS += -I${top_srcdir}/src
AM_CPPFLAGS += -I${top_srcdir}/tests
AM_CPPFLAGS += -I${top_builddir}
AM_CPPFLAGS += -I${top_builddir}/src
AM_CPPFLAGS += -DUSE_HDF5
AM_CPPFLAGS += -DHDF5_PARALLEL
LDADD = $(top_builddir)/src/libH5VL_log.la ../common/libtestutils.la

../common/libtestutils.la:
	set -e; cd ../common && $(MAKE) $(MFLAGS) tests

# The following files will be downloaded from NetCDF4 library
SEQ_SRCS = tst_files.c \
           tst_grps.c \
           tst_grps2.c \
           tst_atts.c \
           tst_atts1.c \
           tst_atts2.c \
           tst_atts3.c \
           tst_dims2.c \
           tst_types.c \
           tst_vars3.c \
           tst_dims.c \
           tst_dims3.c \
           tst_vars.c

PAR_SRCS = parallel_vara.c

SEQ_TESTS = $(SEQ_SRCS:.c=)
PAR_TESTS = $(PAR_SRCS:.c=)

TESTPROGRAMS =

if ENABLE_SHARED
if ENABLE_NETCDF4
TESTPROGRAMS += $(SEQ_TESTS) $(PAR_TESTS)

BUILT_SOURCES = $(SEQ_SRCS) \
                $(PAR_SRCS) \
                nc4internal.h \
                hdf5internal.h \
                err_macros.h \
                ref_tst_dims.nc

# Do not include these files in the distribution
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
nodist_parallel_vara_SOURCES = parallel_vara.c
endif
endif

NETCDF_VERSION = v4.7.4
NETCDF4_URL = https://raw.githubusercontent.com/Unidata/netcdf-c/$(NETCDF_VERSION)

nc4internal.h:
	touch nc4internal.h

hdf5internal.h:
	touch hdf5internal.h

.SECONDEXPANSION:

$(SEQ_SRCS): %.c : $$(wildcard %.patch.$(NETCDF_VERSION)) err_macros.h
	@DOWNLOAD_CMD@ $(NETCDF4_URL)/nc_test4/$@
	if [ -f $(srcdir)/$(basename $@).patch.$(NETCDF_VERSION) ]; then patch -st $@ < $(srcdir)/$(basename $@).patch.$(NETCDF_VERSION); fi

err_macros.h: Makefile
	@DOWNLOAD_CMD@ $(NETCDF4_URL)/include/$@

ref_tst_dims.nc:
	@DOWNLOAD_CMD@ $(NETCDF4_URL)/nc_test4/$@

parallel_vara.c: parallel_vara.patch.$(NETCDF_VERSION)
	@DOWNLOAD_CMD@ $(NETCDF4_URL)/examples/C/$@
	patch -st $@ < $<

check_PROGRAMS = $(TESTPROGRAMS)

EXTRA_DIST = wrap_runs.sh parallel_run.sh nc_tests.h \
             tst_dims.patch.$(NETCDF_VERSION) \
             tst_dims3.patch.$(NETCDF_VERSION) \
             tst_vars.patch.$(NETCDF_VERSION) \
             tst_atts.patch.$(NETCDF_VERSION) \
             tst_atts3.patch.$(NETCDF_VERSION) \
             tst_files.patch.$(NETCDF_VERSION) \
             tst_grps.patch.$(NETCDF_VERSION) \
             tst_types.patch.$(NETCDF_VERSION) \
             tst_vars3.patch.$(NETCDF_VERSION) \
             parallel_vara.patch.$(NETCDF_VERSION)

# autimake 1.11.3 has not yet implemented AM_TESTS_ENVIRONMENT
# For newer versions, we can use AM_TESTS_ENVIRONMENT instead
# AM_TESTS_ENVIRONMENT  = export TESTPROGRAMS="$(TESTPROGRAMS)";
# AM_TESTS_ENVIRONMENT += export TESTSEQRUN="$(TESTSEQRUN)";
# AM_TESTS_ENVIRONMENT += export TESTOUTDIR="$(TESTOUTDIR)";
TESTS_ENVIRONMENT  = export SED="$(SED)";
TESTS_ENVIRONMENT += export EGREP="$(EGREP)";
TESTS_ENVIRONMENT += export top_builddir="$(top_builddir)";
TESTS_ENVIRONMENT += export TESTOUTDIR="$(TESTOUTDIR)";
TESTS_ENVIRONMENT += export TESTSEQRUN="$(TESTSEQRUN)";
TESTS_ENVIRONMENT += export TESTMPIRUN="$(TESTMPIRUN)";
TESTS_ENVIRONMENT += export TESTPROGRAMS="$(TESTPROGRAMS)";
TESTS_ENVIRONMENT += export check_PROGRAMS="$(check_PROGRAMS)";
TESTS_ENVIRONMENT += export PAR_TESTS="$(PAR_TESTS)";
TESTS_ENVIRONMENT += export NCDUMP=@NCDUMP@;

H5_FILES = $(check_PROGRAMS:%=$(TESTOUTDIR)/%.h5)

CLEANFILES = $(H5_FILES) core core.* *.gcda *.gcno *.gcov gmon.out *.h5 *.nc  *.log *.dump

DISTCLEANFILES = $(SEQ_SRCS) $(PAR_SRCS) \
                 nc4internal.h err_macros.h \
                 hdf5internal.h ref_tst_dims.nc

TEST_EXTENSIONS = .sh
LOG_COMPILER = $(srcdir)/wrap_runs.sh
SH_LOG_COMPILER =

TESTS = $(TESTPROGRAMS)

ptest: $(PAR_TESTS)
	@echo "============================================================================"
	@echo "    $(subdir): Parallel testing on 4 MPI processes"
	@echo "============================================================================"
	@$(TESTS_ENVIRONMENT) \
	$(srcdir)/parallel_run.sh 4 || exit 1

# build check targets but not invoke
tests-local: all $(check_PROGRAMS)

.PHONY: ptest tests

