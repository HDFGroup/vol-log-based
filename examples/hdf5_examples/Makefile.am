#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#
# @configure_input@

SUFFIXES = .o .c

AM_DEFAULT_SOURCE_EXT = .c

AM_CPPFLAGS = -I${srcdir}
AM_CPPFLAGS += -I${top_srcdir}
AM_CPPFLAGS += -I${top_srcdir}/src
AM_CPPFLAGS += -I${top_srcdir}/tests
AM_CPPFLAGS += -I${top_builddir}
AM_CPPFLAGS += -I${top_builddir}/src
AM_CPPFLAGS += -DUSE_HDF5
AM_CPPFLAGS += -DHDF5_PARALLEL
LDADD = $(top_builddir)/src/libH5VL_log.la

# All Test programs will be downloaded from HDF5 library repo

# Running dependentcy: (thus using -j option in 'make check' can fail)
#     h5_crtatt.c  must run after h5_crtdat.c   (file dependency: dset.h5)
#     h5_rdwt.c    must run after h5_crtdat.c   (file dependency: dset.h5)
#     h5_crtgrpd.c must run after h5_crtgrpar.c (file dependency: groups.h5)
#     h5_read.c    must run after h5_write.c    (file dependency: SDS.h5)
SEQ_SRCS =  h5_crtdat.c \
            h5_crtatt.c \
            h5_crtgrp.c \
            h5_crtgrpar.c \
            h5_crtgrpd.c \
            h5_interm_group.c \
            h5_rdwt.c \
            h5_write.c \
            h5_read.c \
            h5_select.c \
            h5_subset.c \
            h5_attribute.c

PAR_SRCS = ph5example.c

SEQ_TESTS = $(SEQ_SRCS:.c=)
PAR_TESTS = $(PAR_SRCS:.c=)

TESTPROGRAMS =

if ENABLE_SHARED
TESTPROGRAMS += $(SEQ_TESTS) $(PAR_TESTS)

BUILT_SOURCES = $(SEQ_SRCS) $(PAR_SRCS)

# Do not include these files in the distribution
nodist_h5_attribute_SOURCES = h5_attribute.c
nodist_h5_crtdat_SOURCES = h5_crtdat.c
nodist_h5_crtatt_SOURCES = h5_crtatt.c
nodist_h5_crtgrp_SOURCES = h5_crtgrp.c
nodist_h5_crtgrpar_SOURCES = h5_crtgrpar.c
nodist_h5_crtgrpd_SOURCES = h5_crtgrpd.c
nodist_h5_interm_group_SOURCES = h5_interm_group.c
nodist_h5_rdwt_SOURCES = h5_rdwt.c
nodist_h5_write_SOURCES = h5_write.c
nodist_h5_read_SOURCES = h5_read.c
nodist_h5_select_SOURCES = h5_select.c
nodist_h5_subset_SOURCES = h5_subset.c
nodist_ph5example_SOURCES = ph5example.c
endif

HDF5_VERSION = hdf5-1_13_0
HDF5_URL = https://raw.githubusercontent.com/HDFGroup/hdf5/${HDF5_VERSION}

.SECONDEXPANSION:

$(SEQ_SRCS): %.c : $$(wildcard %.patch.$(HDF5_VERSION))
	@DOWNLOAD_CMD@ $(HDF5_URL)/examples/$@
	if [ -f ${srcdir}/$(basename $@).patch.$(HDF5_VERSION) ]; then patch -st $@ < ${srcdir}/$(basename $@).patch.$(HDF5_VERSION); fi
	$(SED_I) -e "s|main(.*|main(int argc, char **argv)|g" $@
	$(SED_I) -e "s|H5Fcreate(FILE|H5Fcreate(argv[1]|g" $@
	$(SED_I) -e "s|H5Fcreate(H5FILE_NAME|H5Fcreate(argv[1]|g" $@
	$(SED_I) -e "s|H5Fopen(FILE|H5Fopen(argv[1]|g" $@
	$(SED_I) -e "s|H5Fopen(H5FILE_NAME|H5Fopen(argv[1]|g" $@

$(PAR_SRCS): %.c : $$(wildcard %.patch.$(HDF5_VERSION))
	@DOWNLOAD_CMD@ $(HDF5_URL)/examples/$@
	if [ -f ${srcdir}/$(basename $@).patch.$(HDF5_VERSION) ]; then patch -st $@ < ${srcdir}/$(basename $@).patch.$(HDF5_VERSION); fi

check_PROGRAMS = $(TESTPROGRAMS)

EXTRA_DIST = seq_runs.sh \
             parallel_run.sh \
             ph5example.patch.$(HDF5_VERSION) \
             h5_attribute.patch.$(HDF5_VERSION) \
             h5_interm_group.patch.$(HDF5_VERSION)

# autimake 1.11.3 has not yet implemented AM_TESTS_ENVIRONMENT
# For newer versions, we can use AM_TESTS_ENVIRONMENT instead
# AM_TESTS_ENVIRONMENT  = export TESTPROGRAMS="$(TESTPROGRAMS)";
# AM_TESTS_ENVIRONMENT += export TESTSEQRUN="$(TESTSEQRUN)";
# AM_TESTS_ENVIRONMENT += export TESTOUTDIR="$(TESTOUTDIR)";
TESTS_ENVIRONMENT  = export SED="$(SED)";
TESTS_ENVIRONMENT += export EGREP="$(EGREP)";
TESTS_ENVIRONMENT += export srcdir="$(srcdir)";
TESTS_ENVIRONMENT += export top_srcdir="$(top_srcdir)";
TESTS_ENVIRONMENT += export top_builddir="$(top_builddir)";
TESTS_ENVIRONMENT += export TESTOUTDIR="$(TESTOUTDIR)";
TESTS_ENVIRONMENT += export TESTSEQRUN="$(TESTSEQRUN)";
TESTS_ENVIRONMENT += export TESTMPIRUN="$(TESTMPIRUN)";
TESTS_ENVIRONMENT += export TESTPROGRAMS="$(TESTPROGRAMS)";
TESTS_ENVIRONMENT += export PAR_TESTS="$(PAR_TESTS)";

H5_OUTPUT_FILES = Attributes.h5 \
                  dset.h5 \
                  group.h5 \
                  groups.h5 \
                  interm_group.h5 \
                  ParaEg0.h5 \
                  ParaEg1.h5 \
                  SDS.h5 \
                  Select.h5 \
                  subset.h5

H5_FILES = $(check_PROGRAMS:%=$(TESTOUTDIR)/%.h5) \
           $(H5_OUTPUT_FILES:%=$(TESTOUTDIR)/%)

CLEANFILES = $(H5_FILES) core core.*

DISTCLEANFILES = $(SEQ_SRCS) $(PAR_SRCS)

TEST_EXTENSIONS = .sh
LOG_COMPILER = $(srcdir)/seq_runs.sh
SH_LOG_COMPILER =

TESTS = $(TESTPROGRAMS)

ptest: $(check_PROGRAMS)
	@echo "============================================================================"
	@echo "    $(subdir): Parallel testing on 4 MPI processes"
	@echo "============================================================================"
	@$(TESTS_ENVIRONMENT) \
	$(srcdir)/parallel_run.sh 4 || exit 1

# build check targets but not invoke
tests-local: all $(check_PROGRAMS)

.PHONY: ptest tests

.NOTPARALLEL:

