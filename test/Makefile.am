#
# Copyright (C) 2020, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#
# $Id$
#
# @configure_input@

C_SUBDIRS = basic dynamic testcases

DIST_SUBDIRS = $(C_SUBDIRS) 

SUBDIRS = $(C_SUBDIRS)

EXTRA_DIST = testutils.hpp

ptest:
	for d in $(SUBDIRS) ; do \
		$(MAKE) $(MFLAGS) -C $$d ptest $$* || exit 1 ; \
	done

ptests:
	for d in $(SUBDIRS) ; do \
		$(MAKE) $(MFLAGS) -C $$d ptests $$* || exit 1 ; \
	done

.PHONY: ptest ptests