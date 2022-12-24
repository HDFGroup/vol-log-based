#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#
# $Id$
#
# @configure_input@

AM_CPPFLAGS = -I${srcdir}
AM_CPPFLAGS += -I${top_srcdir}
AM_CPPFLAGS += -I${top_srcdir}/src
AM_CPPFLAGS += -I${top_srcdir}/tests
AM_CPPFLAGS += -I${top_builddir}
AM_CPPFLAGS += -I${top_builddir}/src
LDADD = $(top_builddir)/src/libH5VL_log.la

EXTRA_DIST = test.sh config.sh.in

# autimake 1.11.3 has not yet implemented AM_TESTS_ENVIRONMENT
# For newer versions, we can use AM_TESTS_ENVIRONMENT instead
# AM_TESTS_ENVIRONMENT  = export TESTPROGRAMS="$(TESTPROGRAMS)";
# AM_TESTS_ENVIRONMENT += export TESTSEQRUN="$(TESTSEQRUN)";
# AM_TESTS_ENVIRONMENT += export TESTOUTDIR="$(TESTOUTDIR)";
TESTS_ENVIRONMENT  = export SED="$(SED)";
TESTS_ENVIRONMENT += export srcdir="$(srcdir)";
TESTS_ENVIRONMENT += export top_builddir="$(top_builddir)";
TESTS_ENVIRONMENT += export builddir="$(builddir)";
TESTS_ENVIRONMENT += export TESTOUTDIR="$(TESTOUTDIR)";
TESTS_ENVIRONMENT += export TESTSEQRUN="$(TESTSEQRUN)";
TESTS_ENVIRONMENT += export TESTMPIRUN="$(TESTMPIRUN)";
TESTS_ENVIRONMENT += export TESTPROGRAMS="$(TESTPROGRAMS)";
TESTS_ENVIRONMENT += export check_PROGRAMS="$(check_PROGRAMS)";

CLEANFILES = 8a_benchmark_write_parallel 8b_benchmark_read_parallel \
             8a_benchmark_write_parallel.log \
             core core.* *.gcda *.gcno *.gcov gmon.out *.h5 *.log

check_PROGRAMS = 8a_benchmark_write_parallel 8b_benchmark_read_parallel

8a_benchmark_write_parallel_SOURCES =
8b_benchmark_read_parallel_SOURCES =

TESTS = test.sh

8a_benchmark_write_parallel$(EXEEXT):
	cd build && $(MAKE) -j4
	cp build/bin/8a_benchmark_write_parallel .

8b_benchmark_read_parallel$(EXEEXT):
	cd build && $(MAKE) -j4
	cp build/bin/8b_benchmark_read_parallel .

ptest: $(check_PROGRAMS)
	@echo "==========================================================="
	@echo "    $(subdir): Parallel testing on 4 MPI processes"
	@echo "==========================================================="
	@$(TESTS_ENVIRONMENT) \
	$(srcdir)/test.sh 4 || exit 1

clean-local:
	if [ -d "build" ]; then cd build && $(MAKE) clean ; fi
	-rm -rf  ../samples

DISTCLEANFILES = openPMD-api.tar.gz

distclean-local:
	-rm -rf build openPMD-api

# build check targets but not invoke
tests-local: all $(check_PROGRAMS)

.PHONY: ptest tests

