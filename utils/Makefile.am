#
# Copyright (C) 2021, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#
# @configure_input@

SUBDIRS = h5lreplay h5ldump
DIST_SUBDIRS = h5lreplay h5ldump

bin_SCRIPTS = h5lconfig h5lenv.bash h5lenv.csh h5lpcc h5lpcxx
EXTRA_DIST = h5lconfig.in h5lenv.bash.in h5lenv.csh.in h5lpcc.in h5lpcxx.in

dist-hook:
	$(SED_I) -e "1,10s|_RELEASE_DATE_|@H5VL_LOG_RELEASE_DATE@|" $(distdir)/h5lconfig.in
	$(SED_I) -e "1,10s|_VERSION_|$(VERSION)|" $(distdir)/h5lconfig.in

install-exec-hook:
	$(SED_I) -e "1,10s|_RELEASE_DATE_|@H5VL_LOG_RELEASE_DATE@|" $(DESTDIR)$(bindir)/h5lconfig
	$(SED_I) -e "1,10s|_VERSION_|$(VERSION)|" $(DESTDIR)$(bindir)/h5lconfig
