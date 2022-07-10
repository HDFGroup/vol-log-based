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

# The following files will be downloaded from HDF5 library
SEQ_SRCS = h5_attribute.c h5_group.c h5_extend_write.c \
           h5_cmprss.c h5_subset.c


TESTPROGRAMS = $(SEQ_SRCS:.c=)


HDF5_EXAMPLE_URL = https://raw.githubusercontent.com/HDFGroup/hdf5/develop/examples



.SECONDEXPANSION:

$(SEQ_SRCS): %.c : $$(wildcard %.patch)
	@DOWNLOAD_CMD@ $(HDF5_EXAMPLE_URL)/$@
	if [ -f $(srcdir)/$(basename $@).patch ]; then patch -st $@ < $(srcdir)/$(basename $@).patch; fi

check_PROGRAMS = $(TESTPROGRAMS)

EXTRA_DIST = wrap_runs.sh parallel_run.sh \
             h5_attribute.patch h5_group.patch h5_chunk_read.patch \
			 h5_extend_write.patch
             


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

DISTCLEANFILES = $(SEQ_SRCS) $(PAR_SRCS) \
                 nc4internal.h err_macros.h \
                 hdf5internal.h ref_tst_dims.nc

TEST_EXTENSIONS = .sh
LOG_COMPILER = $(srcdir)/wrap_runs.sh
SH_LOG_COMPILER =

TESTS = $(TESTPROGRAMS)

# build check targets but not invoke
tests-local: all $(check_PROGRAMS)

ptest: $(PAR_TESTS)
	@echo "==========================================================="
	@echo "    $(subdir): Parallel testing on 4 MPI processes"
	@echo "==========================================================="
	@$(TESTS_ENVIRONMENT) \
	$(srcdir)/parallel_run.sh 4 || exit 1

.PHONY: ptest
