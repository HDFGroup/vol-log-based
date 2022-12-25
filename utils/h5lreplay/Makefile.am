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
AM_CPPFLAGS += -I${top_srcdir}/tests/common
AM_CPPFLAGS += -I${top_srcdir}/src
AM_CPPFLAGS += -I${top_builddir}
AM_CPPFLAGS += -I${top_builddir}/src
AM_CPPFLAGS += -DREPLAY_BUILD
if LOGVOL_DEBUG
   AM_CPPFLAGS += -DLOGVOL_DEBUG=1
endif
if LOGVOL_PROFILING
   AM_CPPFLAGS += -DLOGVOL_PROFILING=1
endif

check_PROGRAMS = h5lgen

bin_PROGRAMS = h5lreplay
h5lreplay_LDADD  = $(top_builddir)/src/libH5VL_log.la
h5lreplay_LDFLAGS = -no-install

h5lreplay_SOURCES =  h5lreplay.cpp \
                     h5lreplay.hpp \
                     h5lreplay_copy.cpp \
                     h5lreplay_copy.hpp \
                     h5lreplay_data.cpp \
                     h5lreplay_data.hpp \
                     h5lreplay_meta.cpp \
                     h5lreplay_meta.hpp 

EXTRA_DIST = test.sh

CLEANFILES = core core.* *.gcda *.gcno *.gcov gmon.out *.h5 *.h5l *.h5r

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
TESTS_ENVIRONMENT += export H5DIFF="@H5DIFF_PATH@";
TESTS_ENVIRONMENT += export check_PROGRAMS="$(check_PROGRAMS)";

TEST_EXTENSIONS = .sh
LOG_COMPILER = $(srcdir)/wrap_runs.sh
SH_LOG_COMPILER =

TESTS = test.sh

dist-hook:
#	$(SED_I) -e "s|RELEASE_DATE|@LOGVOL_RELEASE_DATE@|g" $(distdir)/main.cpp
#	$(SED_I) -e "1,10s|_LOGVOL_RELEASE_DATE_|@LOGVOL_RELEASE_DATE@|" $(distdir)/RELEASE_NOTES.md
#	$(SED_I) -e "1,10s|_LOGVOL_VERSION_|$(LOGVOL_VERSION)|" $(distdir)/RELEASE_NOTES.md

# build check targets but not invoke
tests-local: all $(check_PROGRAMS)

.PHONY: ptest tests

