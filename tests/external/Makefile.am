#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#
# $Id$
#
# @configure_input@

C_SUBDIRS =

if ENABLE_SHARED
if HAVE_QMCPACK
    C_SUBDIRS += qmcpack
endif
if HAVE_HDF5_IOTEST
    C_SUBDIRS += hdf5_iotest
endif
if HAVE_OPENPMD
    C_SUBDIRS += openpmd
endif
endif

DIST_SUBDIRS = qmcpack hdf5_iotest openpmd
SUBDIRS = $(C_SUBDIRS)

ptest:
	@for d in $(SUBDIRS) ; do \
		$(MAKE) $(MFLAGS) -C $$d ptest $$* || exit 1 ; \
	done

.PHONY: ptest
