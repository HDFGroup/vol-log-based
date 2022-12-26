#!/bin/sh
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

# $1 -- executable
# $2 -- output file name
# $3 -- 2nd output file name if there is any

test_func() {
   outfile_kind=HDF5-LogVOL

   # Exit immediately if a command exits with a non-zero status.
   set -e

   if test "x$2" != x ; then
      outfile="${TESTOUTDIR}/$2"
   fi
   if test "x$3" != x ; then
      outfile2="${TESTOUTDIR}/$3"
   fi

echo "------- $outfile $outfile2"
echo "------- ${TESTSEQRUN} ./$1 $outfile"
   ${TESTSEQRUN} ./$1 $outfile

   for f in ${outfile} ${outfile2} ; do
      FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k $f`
      if test "x${FILE_KIND}" != x$outfile_kind ; then
         echo "Error (as $vol_type VOL): expecting file kind $outfile_kind but got ${FILE_KIND}"
         exit 1
      fi
   done
}

