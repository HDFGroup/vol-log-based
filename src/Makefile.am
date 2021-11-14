#
# Copyright (C) 2019, Northwestern University and Fermi National Accelerator Laboratory
# See COPYRIGHT notice in top-level directory.
#
# @configure_input@

SUFFIXES = .cpp .hpp .o

AM_CPPFLAGS = -I${srcdir}
AM_CPPFLAGS += -I${top_srcdir}
AM_CPPFLAGS += -I${top_srcdir}/src
AM_CPPFLAGS += -I${top_builddir}
AM_CPPFLAGS += -I${top_builddir}/src
if LOGVOL_DEBUG
   AM_CPPFLAGS += -DLOGVOL_DEBUG=1
endif
if LOGVOL_PROFILING
   AM_CPPFLAGS += -DLOGVOL_PROFILING=1
endif
M4FLAGS = -I${top_srcdir}/m4 -I${top_srcdir}/src

lib_LTLIBRARIES = libH5VL_log.la

M4_SRCS = 

M4H_SRCS = H5VL_logi_profiling.m4h

H_SRCS =    H5VL_log_att.hpp \
            H5VL_log_blob.hpp \
            H5VL_log_dataset.hpp \
            H5VL_log_dataseti.hpp \
            H5VL_log_datatype.hpp \
            H5VL_log_file.hpp \
            H5VL_log_filei.hpp \
            H5VL_log_group.hpp \
            H5VL_log_info.hpp \
            H5VL_log_introspect.hpp \
            H5VL_log_link.hpp \
            H5VL_log_main.hpp \
            H5VL_log_obj.hpp \
            H5VL_log_obji.hpp \
            H5VL_log_req.hpp \
            H5VL_log_reqi.hpp \
            H5VL_log_token.hpp \
            H5VL_log_wrap.hpp \
            H5VL_logi.hpp \
            H5VL_logi_dataspace.hpp \
            H5VL_logi_debug.hpp \
            H5VL_logi_err.hpp \
            H5VL_logi_filter.hpp \
            H5VL_logi_filter_deflate.hpp \
            H5VL_logi_idx.hpp \
	         H5VL_logi_mem.hpp \
            H5VL_logi_meta.hpp \
            H5VL_logi_nb.hpp \
            H5VL_logi_util.hpp \
            H5VL_logi_wrapper.hpp \
            H5VL_logi_zip.hpp

C_SRCS = 

CXX_SRCS =  H5VL_log_att.cpp \
            H5VL_log_blob.cpp \
            H5VL_log_dataset.cpp \
            H5VL_log_dataseti.cpp \
            H5VL_log_datatype.cpp \
            H5VL_log_datatypei.cpp \
            H5VL_log_file.cpp \
            H5VL_log_filei.cpp \
            H5VL_log_filei_meta.cpp \
            H5VL_log_group.cpp \
            H5VL_log_info.cpp \
            H5VL_log_introspect.cpp \
            H5VL_log_link.cpp \
            H5VL_log_main.cpp \
            H5VL_log_obj.cpp \
            H5VL_log_obji.cpp \
            H5VL_log_req.cpp \
            H5VL_log_reqi.cpp \
            H5VL_log_token.cpp \
            H5VL_log_wrap.cpp \
            H5VL_log.cpp \
            H5VL_logi_dataspace.cpp \
            H5VL_logi_filter.cpp \
            H5VL_logi_filter_deflate.cpp \
            H5VL_logi_idx.cpp \
	         H5VL_logi_idx_list.cpp \
	         H5VL_logi_mem.cpp \
            H5VL_logi_meta.cpp \
            H5VL_logi_nb.cpp \
            H5VL_logi_util.cpp \
            H5VL_logi_wrapper.cpp \
            H5VL_logi_zip.cpp
            
if LOGVOL_DEBUG
   CXX_SRCS += H5VL_logi_debug.cpp
endif
if LOGVOL_PROFILING
   M4_SRCS += H5VL_logi_profiling.m4
endif

nodist_include_HEADERS = H5VL_log.h

$(M4_SRCS:.m4=.cpp): Makefile H5VL_logi_profiling_timers.m4
$(M4H_SRCS:.m4h=.hpp): Makefile H5VL_logi_profiling_timers.m4

.m4.cpp: H5VL_logi_profiling_timers.m4
	$(M4) $(AM_M4FLAGS) $(M4FLAGS) $< >$@

.m4h.hpp: H5VL_logi_profiling_timers.m4
	$(M4) $(AM_M4FLAGS) $(M4FLAGS) $< >$@

libH5VL_log_la_SOURCES = $(C_SRCS) $(CXX_SRCS) $(H_SRCS)
nodist_libH5VL_log_la_SOURCES = $(M4_SRCS:.m4=.cpp) $(M4H_SRCS:.m4h=.h)
PNETCDF_HEADER = $(top_builddir)/src/include/pnetcdf.h

BUILT_SOURCES = $(M4_SRCS:.m4=.cpp) $(M4H_SRCS:.m4h=.hpp)
EXTRA_DIST = $(M4H_SRCS) $(M4_SRCS) H5VL_logi_profiling.m4 H5VL_logi_profiling_timers.m4
CLEANFILES = $(M4_SRCS:.m4=.cpp) $(M4H_SRCS:.m4h=.hpp)

dist-hook:
#	$(SED_I) -e "s|RELEASE_DATE|@LOGVOL_RELEASE_DATE@|g" $(distdir)/main.cpp
#	$(SED_I) -e "1,10s|_LOGVOL_RELEASE_DATE_|@LOGVOL_RELEASE_DATE@|" $(distdir)/RELEASE_NOTES.md
#	$(SED_I) -e "1,10s|_LOGVOL_VERSION_|$(LOGVOL_VERSION)|" $(distdir)/RELEASE_NOTES.md

.PHONY:
