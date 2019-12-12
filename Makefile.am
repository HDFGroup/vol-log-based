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

noinst_PROGRAMS = ph5_concat

ph5_concat_SOURCES = ph5_concat.cpp \
                     file_partition.cpp \
                     dataset_partition.cpp \
                     main.cpp

EXTRA_DIST = ph5_concat.hpp \
             acinclude.m4 \
             COPYRIGHT \
             CREDITS.md \
             AUTHORS.md \
             README.md \
             RELEASE_NOTES.md \
             list.txt

dist-hook:
	$(SED_I) -e "s|RELEASE_DATE|@LOGVOL_RELEASE_DATE@|g" $(distdir)/main.cpp
	$(SED_I) -e "1,10s|_LOGVOL_RELEASE_DATE_|@LOGVOL_RELEASE_DATE@|" $(distdir)/RELEASE_NOTES.md
	$(SED_I) -e "1,10s|_LOGVOL_VERSION_|$(LOGVOL_VERSION)|" $(distdir)/RELEASE_NOTES.md

.PHONY:
