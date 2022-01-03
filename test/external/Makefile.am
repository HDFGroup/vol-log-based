#
# Copyright (C) 2020, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#
# $Id$
#
# @configure_input@

C_SUBDIRS =

if HAVE_QMCPACK
    C_SUBDIRS += qmcpack
endif

DIST_SUBDIRS = qmcpack
SUBDIRS = $(C_SUBDIRS)

ptest:
	for d in $(SUBDIRS) ; do \
		$(MAKE) $(MFLAGS) -C $$d ptest $$* || exit 1 ; \
	done

ptests:
	for d in $(SUBDIRS) ; do \
		$(MAKE) $(MFLAGS) -C $$d ptests $$* || exit 1 ; \
	done

.PHONY: ptest ptests