#
# Copyright (C) 2019, Northwestern University and Fermi National Accelerator Laboratory
# See COPYRIGHT notice in top-level directory.
#
# @configure_input@

SUFFIXES = .cpp .hpp .o

AM_CPPFLAGS = -I.
if LOGVOL_DEBUG
   AM_CPPFLAGS += -DLOGVOL_DEBUG=1
endif
if LOGVOL_PROFILING
   AM_CPPFLAGS += -DLOGVOL_PROFILING=1
endif
M4FLAGS = -I${top_srcdir}/m4

noinst_LTLIBRARIES = liblogvol.la

M4_SRCS = 

H_SRCS =    logvol.h \
            logvol_internal.h

C_SRCS = 

CXX_SRCS =  logvol.cpp \
            logvol_att.cpp \
            logvol_dataset.cpp \
            logvol_dataset_internal.cpp \
            logvol_datatype_internal.cpp \
            logvol_file.cpp \
            logvol_file_internal.cpp \
            logvol_introspect.cpp \
            logvol_group.cpp \
            logvol_link.cpp \
            logvol_nb.cpp \
            logvol_obj.cpp \
            logvol_req.cpp \
            logvol_type.cpp \
            logvol_util.cpp \
            logvol_internal.cpp


EXTRA_DIST = ph5_concat.hpp \
             acinclude.m4 \
             COPYRIGHT \
             CREDITS.md \
             AUTHORS.md \
             README.md \
             RELEASE_NOTES.md \
             list.txt

$(M4_SRCS:.m4=.cpp): Makefile

.m4.cpp:
	$(M4) $(AM_M4FLAGS) $(M4FLAGS) $< >$@

liblogvol_la_SOURCES = $(C_SRCS) $(CXX_SRCS) $(H_SRCS)
nodist_liblogvol_la_SOURCES = $(M4_SRCS:.m4=.cpp) $(M4H_SRCS:.m4h=.h)

dist-hook:
	$(SED_I) -e "s|RELEASE_DATE|@LOGVOL_RELEASE_DATE@|g" $(distdir)/main.cpp
	$(SED_I) -e "1,10s|_LOGVOL_RELEASE_DATE_|@LOGVOL_RELEASE_DATE@|" $(distdir)/RELEASE_NOTES.md
	$(SED_I) -e "1,10s|_LOGVOL_VERSION_|$(LOGVOL_VERSION)|" $(distdir)/RELEASE_NOTES.md

.PHONY:
