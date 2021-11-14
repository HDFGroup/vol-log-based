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

bin_PROGRAMS = h5replay
h5replay_LDADD  = $(top_builddir)/src/libH5VL_log.la
h5replay_LDFLAGS = -no-install

h5replay_SOURCES =   h5replay.cpp \
                     h5replay.hpp \
                     h5replay_copy.cpp \
                     h5replay_copy.hpp \
                     h5replay_data.cpp \
                     h5replay_data.hpp \
                     h5replay_meta.cpp \
                     h5replay_meta.hpp 

bin_SCRIPTS = logvol-config
EXTRA_DIST = logvol-config.in

CLEANFILES = core core.* *.gcda *.gcno *.gcov gmon.out

install-exec-hook:
	chmod +x $(DESTDIR)$(bindir)/logvol-config

tests-local: all

.PHONY:
