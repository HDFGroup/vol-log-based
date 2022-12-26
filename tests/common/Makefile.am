#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#
# @configure_input@

SUFFIXES = .cpp .hpp

check_LTLIBRARIES = libtestutils.la

libtestutils_la_SOURCES = testutils.cpp testutils.hpp

EXTRA_DIST = wrap_runs.sh

# build check targets but not invoke
tests-local: all $(check_LTLIBRARIES)

.PHONY: tests ptest

