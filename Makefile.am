#
# Copyright (C) 2019, Northwestern University and Fermi National Accelerator Laboratory
# See COPYRIGHT notice in top-level directory.
#
# @configure_input@

SUBDIRS = src test examples utils doc
DIST_SUBDIRS = src test examples utils doc
EXTRA_DIST =    m4/foreach_idx.m4 \
                m4/foreach.m4 \
                m4/list_len.m4 \
                m4/utils.m4 \
                README.md \
                COPYRIGHT \
                RELEASE_NOTE.md

.PHONY:

dist-hook:
#	$(SED_I) -e "1,10s|_RELEASE_DATE_|@H5VL_LOG_RELEASE_DATE@|" $(distdir)/RELEASE_NOTE.md
#	$(SED_I) -e "1,10s|_VERSION_|$(VERSION)|" $(distdir)/RELEASE_NOTE.md

