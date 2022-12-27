#!/bin/bash -l
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

RUN_CMD=${TESTSEQRUN}

. $srcdir/../common/wrap_runs.sh

log_vol_file_only=0

test_func $1

