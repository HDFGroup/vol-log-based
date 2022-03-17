#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#
# @configure_input@

C_SUBDIRS = basic dynamic testcases external

DIST_SUBDIRS = $(C_SUBDIRS) nc4

SUBDIRS = $(C_SUBDIRS)

if ENABLE_NETCDF4
   SUBDIRS += nc4
endif

PTEST_SUBDIRS = $(SUBDIRS)

EXTRA_DIST = testutils.hpp

ptest:
	@for d in $(PTEST_SUBDIRS) ; do \
		$(MAKE) $(MFLAGS) -C $$d ptest $$* || exit 1 ; \
	done

ptests:
	@for d in $(PTEST_SUBDIRS) ; do \
		$(MAKE) $(MFLAGS) -C $$d ptests $$* || exit 1 ; \
	done

.PHONY: ptest ptests
