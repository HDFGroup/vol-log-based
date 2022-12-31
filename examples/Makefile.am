#
# Copyright (C) 2021, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#
# $Id$
#
# @configure_input@

SUFFIXES = .o .cpp

AM_DEFAULT_SOURCE_EXT = .cpp

AM_CPPFLAGS = -I${srcdir}
AM_CPPFLAGS += -I${top_srcdir}
AM_CPPFLAGS += -I${top_srcdir}/src
AM_CPPFLAGS += -I${top_builddir}
AM_CPPFLAGS += -I${top_builddir}/src
LDADD = $(top_builddir)/src/libH5VL_log.la

check_PROGRAMS = create_open \
                 dwrite_n \
                 dread_n \
                 non_blocking \
                 demo_logvol \
                 demo_buffered \
                 demo_sblock \
                 demo_dwrite_n

EXTRA_DIST = seq_runs.sh \
             parallel_run.sh \
             README.md \
             demo.cpp

TESTPROGRAMS = $(check_PROGRAMS)

# autimake 1.11.3 has not yet implemented AM_TESTS_ENVIRONMENT
# For newer versions, we can use AM_TESTS_ENVIRONMENT instead
# AM_TESTS_ENVIRONMENT  = export TESTPROGRAMS="$(TESTPROGRAMS)";
# AM_TESTS_ENVIRONMENT += export TESTSEQRUN="$(TESTSEQRUN)";
# AM_TESTS_ENVIRONMENT += export TESTOUTDIR="$(TESTOUTDIR)";
TESTS_ENVIRONMENT  = export SED="$(SED)";
TESTS_ENVIRONMENT += export EGREP="$(EGREP)";
TESTS_ENVIRONMENT += export srcdir="$(srcdir)";
TESTS_ENVIRONMENT += export top_builddir="$(top_builddir)";
TESTS_ENVIRONMENT += export TESTOUTDIR="$(TESTOUTDIR)";
TESTS_ENVIRONMENT += export TESTSEQRUN="$(TESTSEQRUN)";
TESTS_ENVIRONMENT += export TESTMPIRUN="$(TESTMPIRUN)";
TESTS_ENVIRONMENT += export TESTPROGRAMS="$(TESTPROGRAMS)";
TESTS_ENVIRONMENT += export check_PROGRAMS="$(check_PROGRAMS)";

H5_FILES = $(check_PROGRAMS:%=$(TESTOUTDIR)/%.h5)

CLEANFILES = $(H5_FILES) core core.* *.gcda *.gcno *.gcov gmon.out *.h5

TEST_EXTENSIONS = .sh
LOG_COMPILER = $(srcdir)/seq_runs.sh
SH_LOG_COMPILER =

TESTS = $(TESTPROGRAMS)

C_SUBDIRS = hdf5_examples

DIST_SUBDIRS = $(C_SUBDIRS) 

SUBDIRS = . $(C_SUBDIRS)

PTEST_SUBDIRS = $(C_SUBDIRS)

ptest: $(check_PROGRAMS)
	@echo "============================================================================"
	@echo "    $(subdir): Parallel testing on 4 MPI processes"
	@echo "============================================================================"
	@$(TESTS_ENVIRONMENT) \
	$(srcdir)/parallel_run.sh 4 || exit 1
	@for d in $(PTEST_SUBDIRS) ; do \
		$(MAKE) -C $$d ptest $$* || exit 1 ; \
	done

# build check targets but not invoke
tests-local: all $(check_PROGRAMS)

.PHONY: ptest tests
