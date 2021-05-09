#
# Copyright (C) 2021, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#
# $Id$
#
# @configure_input@

AM_CPPFLAGS = -I$(top_srcdir)/src \
              -I$(top_srcdir) \
              -I$(srcdir) 
if LOGVOL_DEBUG
   AM_CPPFLAGS += -DLOGVOL_DEBUG=1
endif
if LOGVOL_PROFILING
   AM_CPPFLAGS += -DLOGVOL_PROFILING=1
endif

bin_PROGRAMS = h5replay
h5replay_LDADD  = $(top_builddir)/src/libH5VL_log.la
h5replay_LDFLAGS = -no-install

h5replay_SOURCES =   h5replay.cpp \
                     h5replay_copy.cpp \
                     h5replay_data.cpp \
                     h5replay_meta.cpp 

CLEANFILES = core core.* *.gcda *.gcno *.gcov gmon.out

dist-hook:
	$(SED_I) -e "s|RELEASE_DATE|@LOGVOL_RELEASE_DATE@|g" $(distdir)/main.cpp
	$(SED_I) -e "1,10s|_LOGVOL_RELEASE_DATE_|@LOGVOL_RELEASE_DATE@|" $(distdir)/RELEASE_NOTES.md
	$(SED_I) -e "1,10s|_LOGVOL_VERSION_|$(LOGVOL_VERSION)|" $(distdir)/RELEASE_NOTES.md

tests-local: all

.PHONY:
