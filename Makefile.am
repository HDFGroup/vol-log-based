#
# Copyright (C) 2019, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#
# @configure_input@

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src utils doc tests examples
DIST_SUBDIRS = src utils doc tests examples
EXTRA_DIST =    m4/foreach_idx.m4 \
                m4/foreach.m4 \
                m4/list_len.m4 \
                README.md \
                COPYRIGHT \
                RELEASE_NOTE.md

PTEST_SUBDIRS = tests examples

ptest:
	@for d in $(PTEST_SUBDIRS) ; do \
		$(MAKE) -C $$d ptest $$* || exit 1 ; \
	done

tests-local:
	@echo '|'
	@echo '|  All test programs have been successfully built.'
	@echo '|'

.PHONY: ptest tests

dist-hook:
#	$(SED_I) -e "1,10s|_RELEASE_DATE_|@H5VL_LOG_RELEASE_DATE@|" $(distdir)/RELEASE_NOTE.md
#	$(SED_I) -e "1,10s|_VERSION_|$(VERSION)|" $(distdir)/RELEASE_NOTE.md

