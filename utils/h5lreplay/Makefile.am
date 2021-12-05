#
# Copyright (C) 2021, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#
# $Id$
#
# @configure_input@

AM_CPPFLAGS = -I${srcdir}
AM_CPPFLAGS += -I${top_srcdir}
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

bin_PROGRAMS = h5lreplay
h5lreplay_LDADD  = $(top_builddir)/src/libH5VL_log.la
h5lreplay_LDFLAGS = -no-install

h5lreplay_SOURCES =   h5lreplay.cpp \
                     h5lreplay.hpp \
                     h5lreplay_copy.cpp \
                     h5lreplay_copy.hpp \
                     h5lreplay_data.cpp \
                     h5lreplay_data.hpp \
                     h5lreplay_meta.cpp \
                     h5lreplay_meta.hpp 

CLEANFILES = core core.* *.gcda *.gcno *.gcov gmon.out

dist-hook:
#	$(SED_I) -e "s|RELEASE_DATE|@LOGVOL_RELEASE_DATE@|g" $(distdir)/main.cpp
#	$(SED_I) -e "1,10s|_LOGVOL_RELEASE_DATE_|@LOGVOL_RELEASE_DATE@|" $(distdir)/RELEASE_NOTES.md
#	$(SED_I) -e "1,10s|_LOGVOL_VERSION_|$(LOGVOL_VERSION)|" $(distdir)/RELEASE_NOTES.md

tests-local: all

.PHONY: