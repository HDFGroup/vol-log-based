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
TESTS_ENVIRONMENT += export TESTOUTDIR="$(TESTOUTDIR)";
TESTS_ENVIRONMENT += export TESTSEQRUN="$(TESTSEQRUN)";
TESTS_ENVIRONMENT += export TESTMPIRUN="$(TESTMPIRUN)";
TESTS_ENVIRONMENT += export TESTPROGRAMS="$(TESTPROGRAMS)";
TESTS_ENVIRONMENT += export check_PROGRAMS="$(check_PROGRAMS)";

CLEANFILES = core core.* *.gcda *.gcno *.gcov gmon.out *.h5

TESTS = test.sh

8a_benchmark_write_parallel:
	cd build && make -j 4
	cp build/bin/8a_benchmark_write_parallel .

8b_benchmark_read_parallel:
	cd build && make -j 4
	cp build/bin/8b_benchmark_read_parallel .

# build check targets but not invoke
tests-local: all 8a_benchmark_write_parallel 8b_benchmark_read_parallel

ptest ptest4: $(check_PROGRAMS)
	@echo "==========================================================="
	@echo "    $(subdir): Parallel testing on 4 MPI processes"
	@echo "==========================================================="
	$(TESTS_ENVIRONMENT) \
	$(srcdir)/test.sh 4 || exit 1

ptest2: $(check_PROGRAMS)
	@echo "==========================================================="
	@echo "    $(subdir): Parallel testing on 2 MPI processes"
	@echo "==========================================================="
	$(TESTS_ENVIRONMENT) \
	$(srcdir)/test.sh 2 || exit 1

ptest6: $(check_PROGRAMS)
	@echo "==========================================================="
	@echo "    $(subdir): Parallel testing on 6 MPI processes"
	@echo "==========================================================="
	$(TESTS_ENVIRONMENT) \
	$(srcdir)/test.sh 6 || exit 1

ptests: ptest2 ptest4 ptest6
ptest8 ptest10:

.PHONY: ptest ptests ptest2 ptest4 ptest6 ptest8 ptest10

clean-local:
	if [ -d "build" ]; then cd build && make clean ; fi
	rm -f  8a_benchmark_write_parallel 
	rm -f  8b_benchmark_read_parallel
	rm -rf  ../samples

distclean-local:
	rm -rf build