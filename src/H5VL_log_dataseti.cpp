/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>
//
#include <mpi.h>
//
#include "H5VL_log.h"
#include "H5VL_log_dataset.hpp"
#include "H5VL_log_dataseti.hpp"
#include "H5VL_log_filei.hpp"
#include "H5VL_log_req.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_idx.hpp"
#include "H5VL_logi_util.hpp"
#include "H5VL_logi_wrapper.hpp"

static bool interleve (int ndim, int *sa, int *ca, int *sb) {
    int i;

    for (i = 0; i < ndim; i++) {
        if (sa[i] + ca[i] - 1 < sb[i]) {
            return false;
        } else if (sa[i] + ca[i] - 1 > sb[i]) {
            return true;
        }
    }

    return true;
}

#define OSWAP(A, B)    \
    {                  \
        to    = oa[A]; \
        oa[A] = oa[B]; \
        oa[B] = to;    \
        to    = ob[A]; \
        ob[A] = ob[B]; \
        ob[B] = to;    \
        tl    = l[A];  \
        l[A]  = l[B];  \
        l[B]  = tl;    \
    }
void sortoffsets (int len, MPI_Aint *oa, MPI_Aint *ob, int *l) {
    int i, j, p;
    MPI_Aint to;
    int tl;

    if (len < 16) {
        j = 1;
        while (j) {
            j = 0;
            for (i = 0; i < len - 1; i++) {
                if (oa[i + 1] < oa[i]) {
                    OSWAP (i, i + 1)
                    j = 1;
                }
            }
        }
        return;
    }

    p = len / 2;
    OSWAP (p, len - 1);
    p = len - 1;

    j = 0;
    for (i = 0; i < len; i++) {
        if (oa[i] < oa[p]) {
            if (i != j) { OSWAP (i, j) }
            j++;
        }
    }

    OSWAP (p, j)

    sortoffsets (j, oa, ob, l);
    sortoffsets (len - j - 1, oa + j + 1, ob + j + 1, l + j + 1);
}

/*
A log dataset contains multiple entries, one for each write req. An entry can
be an ndim array. This helper function to select a hyperslab from the entry.

sid: the id of the log dataset file space.
start0: the starting pos of an entry w.r.t the beginning of the log dataset, in bytes.
esize: the type size of an element in the ndim array, unit is byte.
starts: the starting indexes of the hyperslab w.r.t the ndim entry.
sizes: the dimensions of the ndim entry.
counts: num of elements of the selected hyperslab block, which should be smaller than "sizes".
ndim: num of dimension of the entry.
*/
herr_t sel_space (hid_t sid,
                       hsize_t start0,
                       hsize_t esize,
                       const int *starts,
                       const int *sizes,
                       const int *counts,
                       int ndim) {
    herr_t err = 0;
    hsize_t start = 0, count = 1, block = 1, off = 1, stride = 1;
    int i;

    if (ndim == 0) {
        count = 1;
        block = counts[0] * esize;
        err = H5Sselect_hyperslab (sid, H5S_SELECT_SET, &start0, NULL, &count, &block);
        return err;
    }
    for (i = 0; i < ndim; i++) {
        start += starts[ndim - i - 1] * off;
        off *= sizes[ndim - i - 1];
        count *= counts[i];
        stride *= sizes[i];
    }
    start = start0 + start * esize;
    block = counts[ndim - 1] * esize;
    count = count / counts[ndim - 1];

    if (count > 1) {
        stride = stride / sizes[0];  // - counts[ndim -1];
        stride *= esize;
        err = H5Sselect_hyperslab (sid, H5S_SELECT_SET, &start, &stride, &count, &block);
    } else {
        err = H5Sselect_hyperslab (sid, H5S_SELECT_SET, &start, NULL, &count, &block);
    }
    return err;
}

/*
Given a file offset (foff) within a log dataset, determine which log dataset
it belongs to.

foff: the file offset of interest, within a log dataset
doffs: the array of file offsets. contains the starting pos (offset) of each log dset.
nldset: number of log dataset. Also equals the lens of doffs.
*/
int foff2logidx (haddr_t foff, haddr_t *doffs, int nldset) {
    for (int i = nldset - 1; i >= 0; i--) {
        if (doffs[i] <= foff) { return i; }
    }
    return -1;
}

/*
This function is the actual function that performs read using the underlying
VOLs.
*/
void H5VL_log_dataset_readi_passthru (std::vector<H5VL_log_idx_search_ret_t> &blocks,
                                        std::vector<H5VL_log_copy_ctx> &overlaps,
                                        H5VL_log_file_t *fp) {
    herr_t err;
    hsize_t ii;
    int32_t i, j, k, l;
    int nblock = blocks.size ();  // Number of place to read
    std::vector<bool> newgroup (
        nblock,
        0);      // Whether the current block does not interleave with the next block
    int nt;      // Number of sub-types in ftype and mtype
    int nrow;    // Number of contiguous sections after breaking down a set of interleaving blocks
    int old_nt;  // Number of elements in foffs, moffs, and lens that has been sorted by foffs
    int *lens       = NULL;  // array_of_blocklengths in MPI_Type_create_struct for ftype and mtype
    MPI_Aint *foffs = NULL;  // array_of_displacements in ftype
    MPI_Aint *moffs = NULL;  // array_of_displacements in mtype
    MPI_Offset fssize[H5S_MAX_RANK];  // number of elements in the subspace below each dimensions in
                                      // dataset dataspace
    MPI_Offset mssize[H5S_MAX_RANK];  // number of elements in the subspace below each dimensions in
                                      // memory dataspace
    MPI_Offset ctr[H5S_MAX_RANK];     // Logical position of the current contiguous section in the
                                      // dataspace of the block being broken down
    H5VL_log_copy_ctx ctx;  // Datastructure to record overlaps so we can copy the data to all the
                            // destination buffer
    char dname[16];         // name of log dataset
    void **ldps = NULL;     // array of all log datasets
    H5VL_loc_params_t loc;
    haddr_t *doffs    = NULL;  // Stores the file offset for each log dataset
    hid_t *fspace_ids = NULL;  // Stores the file space ids for each log dataset
    hid_t mspace_id;           // memory space id
    hsize_t msize;             // The size of the memory space, along one dimension.
    H5VL_dataset_get_args_t args;
    int log_dset;       // the id of a log dataset.
    hid_t dxplid = -1;  // dataset transfer property list id

    // RAII to free resources
    H5VL_logi_err_finally finally (
        [&foffs, &moffs, &lens, &nt, &ldps, &fp, &doffs, &fspace_ids, &dxplid] () -> void {
            int i;
            if (ldps != NULL) {
                for (i = 0; i < fp->nldset; i++) {
                    if (ldps[i] != NULL) {
                        H5VLdataset_close (ldps[i], fp->uvlid, H5P_DATASET_XFER_DEFAULT, NULL);
                    }
                }
            }
            if (fspace_ids) {
                for (i = 0; i < fp->nldset; i++) { H5VL_log_Sclose (fspace_ids[i]); }
            }

            if (foffs) { H5VL_log_free (foffs); }
            if (moffs) { H5VL_log_free (moffs); }
            if (lens) { H5VL_log_free (lens); }
            if (ldps) { H5VL_log_free (ldps); }
            if (doffs) { H5VL_log_free (doffs); }
            if (fspace_ids) { H5VL_log_free (fspace_ids); }
            H5VL_log_Pclose (dxplid);
        });

    if (!nblock) { return; }

    ldps       = (void **)malloc (sizeof (void *) * fp->nldset);
    fspace_ids = (hid_t *)malloc (sizeof (hid_t) * fp->nldset);
    doffs      = (haddr_t *)malloc (sizeof (haddr_t) * fp->nldset);

    dxplid = H5Pcreate (H5P_DATASET_XFER);
    CHECK_ID (dxplid);

    for (i = 0; i < fp->nldset; i++) {
        sprintf (dname, "%s_%d", H5VL_LOG_FILEI_DSET_DATA, i);
        loc.type     = H5VL_OBJECT_BY_SELF;
        loc.obj_type = H5I_GROUP;
        ldps[i] = H5VLdataset_open (fp->lgp, &loc, fp->uvlid, dname, H5P_DATASET_ACCESS_DEFAULT,
                                    dxplid, NULL);
        CHECK_PTR (ldps[i]);

        // Get the file offset for this dataset, save in doffs[i]
        H5VL_logi_dataset_get_foff (fp, ldps[i], fp->uvlid, dxplid, &doffs[i]);

        // Get the file space, save in fspace_ids[i]
        args.op_type = H5VL_DATASET_GET_SPACE;
        err          = H5VLdataset_get (ldps[i], fp->uvlid, &args, dxplid, NULL);
        CHECK_ERR;
        fspace_ids[i] = args.args.get_space.space_id;
    }

    std::sort (blocks.begin (), blocks.end ());

    j = 0;
    for (i = 0; i < nblock - 1; i++) {
        if (blocks[i].zbuf) {
            newgroup[i] = true;
            j           = i + 1;
        } else if ((blocks[j].foff == blocks[i + 1].foff) &&
                   (blocks[j].doff == blocks[i + 1].doff) &&
                   interleve (blocks[j].info->ndim, blocks[j].dstart, blocks[j].count,
                              blocks[i + 1].dstart)) {
            if (blocks[i + 1] > blocks[j]) { j = i + 1; }
            newgroup[i] = false;
        } else {
            newgroup[i] = true;
            j           = i + 1;
        }
    }
    newgroup[nblock - 1] = true;

    // Count total types after breakdown
    nt = 0;
    for (i = j = 0; i < nblock; i++) {
        if (newgroup[i]) {
            if (i == j) {  // only 1
                nt++;
                j++;
            } else {
                for (; j <= i; j++) {  // Breakdown
                    nrow = 1;
                    for (k = 0; k < (int32_t)(blocks[i].info->ndim) - 1; k++) {
                        nrow *= blocks[j].count[k];
                    }
                    nt += nrow;
                }
            }
        }
    }

    lens  = (int *)malloc (sizeof (int) * nt);
    foffs = (MPI_Aint *)malloc (sizeof (MPI_Aint) * nt);
    moffs = (MPI_Aint *)malloc (sizeof (MPI_Aint) * nt);

    // Construct the actual type
    nt = 0;
    for (i = j = 0; i < nblock; i++) {
        if (newgroup[i]) {
            if (i == j) {  // only 1
                if (blocks[j].zbuf) {
                    // Read raw data for filtered datasets
                    if (blocks[j].fsize >
                        0) {  // Don't need to read if read by other intersections already

                        log_dset = foff2logidx (blocks[j].foff, doffs, fp->nldset);
                        CHECK_ID(log_dset);

                        err = H5Sselect_all (fspace_ids[log_dset]);
                        CHECK_ERR;

                        msize    = blocks[j].fsize;
                        mspace_id = H5Screate_simple (1, &msize, &msize);
                        CHECK_ID (mspace_id);

                        err = H5VL_log_under_dataset_read (ldps[log_dset], fp->uvlid, H5T_STD_B8LE,
                                                           mspace_id, fspace_ids[log_dset], dxplid,
                                                           blocks[j].zbuf, NULL);
                        CHECK_ERR;
                        H5VL_log_Sclose (mspace_id);
                        nt++;
                    }
                } else {
                    foffs[nt] = blocks[j].foff + blocks[j].doff;
                    if (blocks[i].info->ndim) {
                        log_dset = foff2logidx (foffs[nt], doffs, fp->nldset);
                        CHECK_ID (log_dset);

                        err = sel_space (fspace_ids[log_dset], foffs[nt] - doffs[log_dset],
                                         blocks[i].info->esize, blocks[j].dstart, blocks[j].dsize,
                                         blocks[j].count, blocks[i].info->ndim);
                        CHECK_ERR;

                        msize = blocks[i].info->esize;
                        for (ii = 0; ii < blocks[i].info->ndim; ii++) {
                            msize *= blocks[j].msize[ii];
                        }
                        mspace_id = H5Screate_simple (1, &msize, &msize);
                        err = sel_space (mspace_id, 0, blocks[i].info->esize, blocks[j].mstart,
                                         blocks[j].msize, blocks[j].count, blocks[i].info->ndim);
                        CHECK_ERR;
                        err = H5VL_log_under_dataset_read (ldps[log_dset], fp->uvlid, H5T_STD_B8LE,
                                                           mspace_id, fspace_ids[log_dset], dxplid,
                                                           blocks[j].xbuf, NULL);
                        CHECK_ERR;
                        H5VL_log_Sclose (mspace_id);

                    } else {  // Special case for scalar entry
                        log_dset = foff2logidx (foffs[nt], doffs, fp->nldset);
                        CHECK_ID (log_dset);

                        lens[nt] = blocks[i].info->esize;
                        err = sel_space (fspace_ids[log_dset], foffs[nt] - doffs[log_dset], 1, NULL,
                                         NULL, &lens[nt], 0);
                        CHECK_ERR;

                        msize     = blocks[j].fsize;
                        mspace_id = H5Screate_simple (1, &msize, &msize);

                        err = H5VL_log_under_dataset_read (ldps[log_dset], fp->uvlid, H5T_STD_B8LE,
                                                           mspace_id, fspace_ids[log_dset], dxplid,
                                                           blocks[j].xbuf, NULL);
                        CHECK_ERR;
                        H5VL_log_Sclose (mspace_id);
                    }
                    nt++;
                }

                j++;
            } else {
                old_nt = nt;
                for (; j <= i; j++) {  // Breakdown each interleaving blocks
                    fssize[blocks[i].info->ndim - 1] = blocks[j].info->esize;
                    mssize[blocks[i].info->ndim - 1] = blocks[j].info->esize;
                    for (k = blocks[i].info->ndim - 2; k > -1; k--) {
                        fssize[k] = fssize[k + 1] * blocks[j].dsize[k + 1];
                        mssize[k] = mssize[k + 1] * blocks[j].msize[k + 1];
                    }

                    memset (ctr, 0, sizeof (MPI_Offset) * blocks[i].info->ndim);
                    while (ctr[0] < blocks[j].count[0]) {  // Foreach row
                        if (blocks[i].info->ndim) {
                            lens[nt] =
                                blocks[j].count[blocks[i].info->ndim - 1] * blocks[j].info->esize;
                        } else {
                            lens[nt] = blocks[j].info->esize;
                        }
                        foffs[nt] = blocks[j].foff + blocks[j].doff;
                        moffs[nt] = (MPI_Offset)(blocks[j].xbuf);
                        for (k = 0; k < (int32_t)(blocks[i].info->ndim); k++) {  // Calculate offset
                            foffs[nt] += fssize[k] * (blocks[j].dstart[k] + ctr[k]);
                            moffs[nt] += mssize[k] * (blocks[j].mstart[k] + ctr[k]);
                        }
                        nt++;

                        if (blocks[i].info->ndim < 2) break;  // Special case for < 2-D
                        ctr[blocks[i].info->ndim - 2]++;      // Move to next position
                        for (k = blocks[i].info->ndim - 2; k > 0; k--) {
                            if (ctr[k] >= blocks[j].count[k]) {
                                ctr[k] = 0;
                                ctr[k - 1]++;
                            }
                        }
                    }
                }

                // Sort into order
                sortoffsets (nt - old_nt, foffs + old_nt, moffs + old_nt, lens + old_nt);

                // Should there be overlapping read, we have to adjust
                for (k = old_nt; k < nt; k++) {
                    for (l = k + 1; l < nt; l++) {
                        if (foffs[k] + lens[k] > foffs[l]) {  // Adjust for overlap
                            // Record a memory copy req that copy the result from the former read to
                            // cover the later one
                            ctx.dst = (char *)moffs[l];
                            ctx.size =
                                std::min ((size_t)lens[l], (size_t)(foffs[k] - foffs[l] + lens[k]));
                            ctx.src = (char *)(moffs[k] - ctx.size + lens[k]);
                            overlaps.push_back (ctx);

                            // Trim off the later one
                            foffs[l] += ctx.size;
                            moffs[l] += ctx.size;
                            lens[l] -= ctx.size;
                        }
                    }
                }
                for (k = old_nt; k < nt; k++) {
                    log_dset = foff2logidx (foffs[k], doffs, fp->nldset);
                    CHECK_ID (log_dset);
                    err = sel_space (fspace_ids[log_dset], foffs[k] - doffs[log_dset], 1, NULL,
                                     NULL, &lens[k], 0);
                    CHECK_ERR;

                    msize     = lens[k];
                    mspace_id = H5Screate_simple (1, &msize, &msize);

                    err = H5VL_log_under_dataset_read (ldps[log_dset], fp->uvlid, H5T_STD_B8LE,
                                                       mspace_id, fspace_ids[log_dset], dxplid,
                                                       moffs[k], NULL);
                    CHECK_ERR;
                    H5VL_log_Sclose (mspace_id);
                }
            }
        }
    }
}

void H5VL_log_dataset_readi_gen_rtypes (std::vector<H5VL_log_idx_search_ret_t> &blocks,
                                        MPI_Datatype *ftype,
                                        MPI_Datatype *mtype,
                                        std::vector<H5VL_log_copy_ctx> &overlaps) {
    int mpierr;
    int32_t i, j, k, l;
    int nblock = blocks.size ();  // Number of place to read
    std::vector<bool> newgroup (
        nblock,
        0);      // Whether the current block does not interleave with the next block
    int nt;      // Number of sub-types in ftype and mtype
    int nrow;    // Number of contiguous sections after breaking down a set of interleaving blocks
    int old_nt;  // Number of elements in foffs, moffs, and lens that has been sorted by foffs
    int *lens;   // array_of_blocklengths in MPI_Type_create_struct for ftype and mtype
    MPI_Aint *foffs      = NULL;               // array_of_displacements in ftype
    MPI_Aint *moffs      = NULL;               // array_of_displacements in mtype
    MPI_Datatype *ftypes = NULL;               // array_of_types in ftype
    MPI_Datatype *mtypes = NULL;               // array_of_types in mtype
    MPI_Datatype etype   = MPI_DATATYPE_NULL;  // element type for each ftypes and mtypes
    MPI_Offset fssize[H5S_MAX_RANK];  // number of elements in the subspace below each dimensions in
                                      // dataset dataspace
    MPI_Offset mssize[H5S_MAX_RANK];  // number of elements in the subspace below each dimensions in
                                      // memory dataspace
    MPI_Offset ctr[H5S_MAX_RANK];     // Logical position of the current contiguous section in the
                                      // dataspace of the block being broken down
    H5VL_log_copy_ctx ctx;  // Datastructure to record overlaps so we can copy the data to all the
                            // destination buffer
    H5VL_logi_err_finally finally ([&ftypes, &mtypes, &foffs, &moffs, &lens, &nt] () -> void {
        int i;
        if (ftypes != NULL) {
            for (i = 0; i < nt; i++) {
                if (ftypes[i] != MPI_BYTE) { MPI_Type_free (ftypes + i); }
            }
            free (ftypes);
        }

        if (mtypes != NULL) {
            for (i = 0; i < nt; i++) {
                if (mtypes[i] != MPI_BYTE) { MPI_Type_free (mtypes + i); }
            }
            free (mtypes);
        }

        H5VL_log_free (foffs);
        H5VL_log_free (moffs);
        H5VL_log_free (lens);
    });

    if (!nblock) {
        *ftype = *mtype = MPI_DATATYPE_NULL;
        return;
    }

    std::sort (blocks.begin (), blocks.end ());

    j = 0;
    for (i = 0; i < nblock - 1; i++) {
        if (blocks[i].zbuf) {
            newgroup[i] = true;
            j           = i + 1;
        } else if ((blocks[j].foff == blocks[i + 1].foff) &&
                   (blocks[j].doff == blocks[i + 1].doff) &&
                   interleve (blocks[j].info->ndim, blocks[j].dstart, blocks[j].count,
                              blocks[i + 1].dstart)) {
            if (blocks[i + 1] > blocks[j]) { j = i + 1; }
            newgroup[i] = false;
        } else {
            newgroup[i] = true;
            j           = i + 1;
        }
    }
    newgroup[nblock - 1] = true;

    // Count total types after breakdown
    nt = 0;
    for (i = j = 0; i < nblock; i++) {
        if (newgroup[i]) {
            if (i == j) {  // only 1
                nt++;
                j++;
            } else {
                for (; j <= i; j++) {  // Breakdown
                    nrow = 1;
                    for (k = 0; k < (int32_t) (blocks[i].info->ndim) - 1; k++) {
                        nrow *= blocks[j].count[k];
                    }
                    nt += nrow;
                }
            }
        }
    }

    lens   = (int *)malloc (sizeof (int) * nt);
    foffs  = (MPI_Aint *)malloc (sizeof (MPI_Aint) * nt);
    moffs  = (MPI_Aint *)malloc (sizeof (MPI_Aint) * nt);
    ftypes = (MPI_Datatype *)malloc (sizeof (MPI_Datatype) * nt);
    mtypes = (MPI_Datatype *)malloc (sizeof (MPI_Datatype) * nt);

    // Construct the actual type
    nt = 0;
    for (i = j = 0; i < nblock; i++) {
        if (newgroup[i]) {
            if (i == j) {  // only 1
                if (blocks[j].zbuf) {
                    // Read raw data for filtered datasets
                    if (blocks[j].fsize >
                        0) {  // Don't need to read if read by other intersections already
                        foffs[nt]  = blocks[j].foff;
                        moffs[nt]  = (MPI_Aint) (blocks[j].zbuf);
                        lens[nt]   = blocks[j].fsize;
                        ftypes[nt] = MPI_BYTE;
                        mtypes[nt] = MPI_BYTE;
                        nt++;
                    }
                } else {
                    if (blocks[i].info->ndim) {
                        etype = H5VL_logi_get_mpi_type_by_size (blocks[i].info->esize);
                        if (etype == MPI_DATATYPE_NULL) {
                            mpierr = MPI_Type_contiguous (blocks[i].info->esize, MPI_BYTE, &etype);
                            CHECK_MPIERR
                            mpierr = MPI_Type_commit (&etype);
                            CHECK_MPIERR
                            k = 1;
                        } else {
                            k = 0;
                        }

                        mpierr = H5VL_log_debug_MPI_Type_create_subarray (
                            blocks[i].info->ndim, blocks[j].dsize, blocks[j].count,
                            blocks[j].dstart, MPI_ORDER_C, etype, ftypes + nt);
                        CHECK_MPIERR
                        mpierr = H5VL_log_debug_MPI_Type_create_subarray (
                            blocks[i].info->ndim, blocks[j].msize, blocks[j].count,
                            blocks[j].mstart, MPI_ORDER_C, etype, mtypes + nt);

                        CHECK_MPIERR
                        mpierr = MPI_Type_commit (ftypes + nt);
                        CHECK_MPIERR
                        mpierr = MPI_Type_commit (mtypes + nt);
                        CHECK_MPIERR

                        if (k) {
                            mpierr = MPI_Type_free (&etype);
                            CHECK_MPIERR
                        }

                        lens[nt] = 1;
                    } else {  // Special case for scalar entry
                        ftypes[nt] = MPI_BYTE;
                        mtypes[nt] = MPI_BYTE;
                        lens[nt]   = blocks[i].info->esize;
                    }

                    foffs[nt] = blocks[j].foff + blocks[j].doff;
                    moffs[nt] = (MPI_Offset) (blocks[j].xbuf);
                    nt++;
                }

                j++;
            } else {
                old_nt = nt;
                for (; j <= i; j++) {  // Breakdown each interleaving blocks
                    fssize[blocks[i].info->ndim - 1] = blocks[j].info->esize;
                    mssize[blocks[i].info->ndim - 1] = blocks[j].info->esize;
                    for (k = blocks[i].info->ndim - 2; k > -1; k--) {
                        fssize[k] = fssize[k + 1] * blocks[j].dsize[k + 1];
                        mssize[k] = mssize[k + 1] * blocks[j].msize[k + 1];
                    }

                    memset (ctr, 0, sizeof (MPI_Offset) * blocks[i].info->ndim);
                    while (ctr[0] < blocks[j].count[0]) {  // Foreach row
                        if (blocks[i].info->ndim) {
                            lens[nt] =
                                blocks[j].count[blocks[i].info->ndim - 1] * blocks[j].info->esize;
                        } else {
                            lens[nt] = blocks[j].info->esize;
                        }
                        foffs[nt] = blocks[j].foff + blocks[j].doff;
                        moffs[nt] = (MPI_Offset) (blocks[j].xbuf);
                        for (k = 0; k < (int32_t) (blocks[i].info->ndim);
                             k++) {  // Calculate offset
                            foffs[nt] += fssize[k] * (blocks[j].dstart[k] + ctr[k]);
                            moffs[nt] += mssize[k] * (blocks[j].mstart[k] + ctr[k]);
                        }
                        ftypes[nt] = MPI_BYTE;
                        mtypes[nt] = MPI_BYTE;
                        nt++;

                        if (blocks[i].info->ndim < 2) break;  // Special case for < 2-D
                        ctr[blocks[i].info->ndim - 2]++;      // Move to next position
                        for (k = blocks[i].info->ndim - 2; k > 0; k--) {
                            if (ctr[k] >= blocks[j].count[k]) {
                                ctr[k] = 0;
                                ctr[k - 1]++;
                            }
                        }
                    }
                }

                // Sort into order
                sortoffsets (nt - old_nt, foffs + old_nt, moffs + old_nt, lens + old_nt);

                // Should there be overlapping read, we have to adjust
                for (k = old_nt; k < nt; k++) {
                    for (l = k + 1; l < nt; l++) {
                        if (foffs[k] + lens[k] > foffs[l]) {  // Adjust for overlap
                            // Record a memory copy req that copy the result from the former read to
                            // cover the later one
                            ctx.dst  = (char *)moffs[l];
                            ctx.size = std::min ((size_t)lens[l],
                                                 (size_t) (foffs[k] - foffs[l] + lens[k]));
                            ctx.src  = (char *)(moffs[k] - ctx.size + lens[k]);
                            overlaps.push_back (ctx);

                            // Trim off the later one
                            foffs[l] += ctx.size;
                            moffs[l] += ctx.size;
                            lens[l] -= ctx.size;
                        }
                    }
                }
            }
        }
    }

    mpierr = MPI_Type_create_struct (nt, lens, foffs, ftypes, ftype);
    CHECK_MPIERR
    mpierr = MPI_Type_commit (ftype);
    CHECK_MPIERR
    mpierr = MPI_Type_create_struct (nt, lens, moffs, mtypes, mtype);
    CHECK_MPIERR
    mpierr = MPI_Type_commit (mtype);
    CHECK_MPIERR
}

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataseti_open
 *
 * Purpose:     Opens a dataset in a container
 *
 * Return:      Success:    Pointer to a dataset object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_log_dataseti_open (void *obj, void *uo, hid_t dxpl_id) {
    int i;
    herr_t err         = 0;
    hid_t dcpl_id      = -1;
    H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
    std::unique_ptr<H5VL_log_dset_t> dp;        // Dataset handle
    std::unique_ptr<H5VL_log_dset_info_t> dip;  // Dataset info
    H5D_fill_value_t stat;
    void *lib_state = NULL;
    void *lib_context = NULL;
    H5VL_logi_err_finally finally ([&dcpl_id, &lib_state, &lib_context] () -> void {
        if (dcpl_id >= 0) { H5Pclose (dcpl_id); }
        H5VL_logi_restore_lib_stat (lib_state, lib_context);
    });
    H5VL_LOGI_PROFILING_TIMER_START;

    // Reset hdf5 context to allow file operations within a dataset operation
    H5VL_logi_reset_lib_stat (lib_state, lib_context);

    dp = std::make_unique<H5VL_log_dset_t> (op, H5I_DATASET, uo);

    if (!op->fp->is_log_based_file) { return (void *)(dp.release ()); }

    // Atts
    H5VL_logi_get_att (dp.get (), H5VL_LOG_DATASETI_ATTR_ID, H5T_NATIVE_INT32, &(dp->id), dxpl_id);

    // Construct new dataset info if not already constructed
    if (!(dp->fp->dsets_info[dp->id])) {
        dip = std::make_unique<H5VL_log_dset_info_t> ();
        CHECK_PTR (dip)

        dip->dtype = H5VL_logi_dataset_get_type (dp->fp, dp->uo, dp->uvlid, dxpl_id);
        CHECK_ID (dip->dtype)

        dip->esize = H5Tget_size (dip->dtype);
        CHECK_ID (dip->esize)

        H5VL_logi_get_att_ex (dp.get (), H5VL_LOG_DATASETI_ATTR_DIMS, H5T_NATIVE_INT64,
                              &(dip->ndim), dip->dims, dxpl_id);
        H5VL_logi_get_att (dp.get (), H5VL_LOG_DATASETI_ATTR_MDIMS, H5T_NATIVE_INT64, dip->mdims,
                           dxpl_id);

        // Dstep for encoding selection
        if (dp->fp->config & H5VL_FILEI_CONFIG_SEL_ENCODE) {
            dip->dsteps[dip->ndim - 1] = 1;
            for (i = dip->ndim - 2; i > -1; i--) {
                dip->dsteps[i] = dip->dsteps[i + 1] * dip->dims[i + 1];
            }
        }

        // Filters
        dcpl_id = H5VL_logi_dataset_get_dcpl (dp->fp, dp->uo, dp->uvlid, dxpl_id);
        CHECK_ID (dcpl_id)
        H5VL_logi_get_filters (dcpl_id, dip->filters);

        // Fill value
        err = H5Pfill_value_defined (dcpl_id, &stat);
        if (stat == H5D_FILL_VALUE_DEFAULT || stat == H5D_FILL_VALUE_USER_DEFINED) {
            dip->fill = (char *)malloc (dip->esize);
            CHECK_PTR (dip->fill)
            err = H5Pget_fill_value (dcpl_id, dip->dtype, dip->fill);
            CHECK_ERR
        } else {
            dip->fill = NULL;
        }

        // Record metadata in fp
        dp->fp->dsets_info[dp->id] = dip.release ();
        // dp->fp->mreqs[dp->id]	   = new H5VL_log_merged_wreq_t (dp, 1);
    }
    H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_OPEN);

    return (void *)(dp.release ());
} /* end H5VL_log_dataset_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_open
 *
 * Purpose:     Opens a dataset in a container
 *
 * Return:      Success:    Pointer to a dataset object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_log_dataseti_wrap (void *uo, H5VL_log_obj_t *cp) {
    return H5VL_log_dataseti_open (cp, uo, cp->fp->dxplid);
} /* end H5VL_log_dataset_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_write
 *
 * Purpose:     Writes data elements from a buffer into a dataset.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
void H5VL_log_dataseti_write (H5VL_log_dset_t *dp,
                              hid_t mem_type_id,
                              hid_t mem_space_id,
                              H5VL_log_selections *dsel,
                              hid_t plist_id,
                              const void *buf,
                              void **req) {
    herr_t err = 0;
    int i;
    H5VL_log_dset_info_t *dip = dp->fp->dsets_info[dp->id];  // Dataset info
    size_t esize;                                            // Element size of the memory type
    size_t selsize;                // Size of metadata selection after deduplication and compression
    H5VL_log_wreq_t *r;            // Request obj
    H5VL_log_req_data_block_t db;  // Request data
    htri_t eqtype;                 // user buffer type equals dataset type?
    H5S_sel_type mstype;           // Memory space selection type
    hbool_t rtype;                 // Whether req is nonblocking
    MPI_Datatype ptype = MPI_DATATYPE_NULL;  // Packing type for non-contiguous memory buffer
#ifdef ENABLE_ZLIB
    int clen, inlen;  // Compressed size; Size of data to be compressed
#endif
    void *lib_state = NULL;
    void *lib_context = NULL;
    H5VL_logi_err_finally finally ([&ptype, &lib_state, &lib_context] () -> void {
        H5VL_log_type_free (ptype);
        H5VL_logi_restore_lib_stat (lib_state, lib_context);
    });
    H5VL_LOGI_PROFILING_TIMER_START;

    H5VL_LOGI_PROFILING_TIMER_START;

    // Check mem space selection
    if (mem_space_id == H5S_ALL)
        mstype = H5S_SEL_ALL;
    else if (mem_space_id == H5S_BLOCK)
        mstype = H5S_SEL_ALL;
    else {
        mstype = H5Sget_select_type (mem_space_id);
        CHECK_ID (mstype)
    }

    // Sanity check
    if (dsel->nsel == 0) return;  // No elements selected
    if (!buf) ERR_OUT ("user buffer can't be NULL");
    H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE_INIT);

    // Reset hdf5 context to allow file operations within a dataset operation
    H5VL_logi_reset_lib_stat (lib_state, lib_context);

    if (dp->fp->config ^ H5VL_FILEI_CONFIG_METADATA_MERGE) {
        H5VL_LOGI_PROFILING_TIMER_START;

        db.ubuf = (char *)buf;
        db.size = dsel->get_sel_size ();  // Number of data elements in the record

        r       = new H5VL_log_wreq_t (dp, dsel);
        selsize = r->hdr->meta_size - (r->sel_buf - r->meta_buf);

        H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE_ENCODE);

#ifdef LOGVOL_PROFILING
        H5VL_log_profile_add_time (dp->fp, TIMER_H5VL_LOG_FILEI_METASIZE_RAW,
                                   (double)(r->hdr->meta_size) / 1048576);
#endif
        // Deduplication
        H5VL_LOGI_PROFILING_TIMER_START;
        if (dp->fp->config & H5VL_FILEI_CONFIG_METADATA_SHARE) {
            if (selsize > sizeof (MPI_Offset)) {  // If selection larger than reference
                auto ret = dp->fp->wreq_hash.find (*r);
                if (ret == dp->fp->wreq_hash.end ()) {
                    dp->fp->wreq_hash[*r] = r;
                } else {
                    r->hdr->flag |= H5VL_LOGI_META_FLAG_SEL_REF;
                    r->hdr->flag &= ~(H5VL_LOGI_META_FLAG_SEL_DEFLATE);  // Remove compression flag
                    if (r->hdr->flag & H5VL_LOGI_META_FLAG_REC) {
                        // If same record, we can remove the record field and make it a full
                        // reference
                        if (ret->second->hdr->flag & H5VL_LOGI_META_FLAG_REC) {
                            if (*((MPI_Offset *)(r->hdr + 1)) ==
                                *((MPI_Offset *)(ret->second->hdr + 1))) {
                                r->sel_buf -= sizeof (MPI_Offset);
                                r->hdr->flag &= ~(H5VL_LOGI_META_FLAG_REC);
                            }
                        }
                    }
                    *((MPI_Offset *)(r->sel_buf)) =
                        ret->second->meta_off - dp->fp->mdsize;  // Record the relative offset
#ifdef WORDS_BIGENDIAN
                    H5VL_logi_llreverse ((uint64_t *)(r->sel_buf));
#endif
                    selsize = sizeof (MPI_Offset);  // New metadata size
#ifdef LOGVOL_PROFILING
                    // Count size saved by duplication
                    H5VL_log_profile_add_time (
                        dp->fp, TIMER_H5VL_LOG_FILEI_METASIZE_DEDUP,
                        (double)(r->sel_buf - r->meta_buf + selsize) / 1048576);
#endif
                }
            }
        }
        H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE_META_DEDUP);

        // Compress metadata
#ifdef ENABLE_ZLIB
        H5VL_LOGI_PROFILING_TIMER_START;

        if (r->hdr->flag & H5VL_LOGI_META_FLAG_SEL_DEFLATE) {
            // Enlarge zip buffer if needed
            if (dp->fp->zbsize < selsize) {
                dp->fp->zbsize = selsize;
                dp->fp->zbuf   = (char *)realloc (dp->fp->zbuf, dp->fp->zbsize);
                CHECK_PTR (dp->fp->zbuf)
            }
            // Compress selections
            inlen = selsize;
            clen  = dp->fp->zbsize;
            err   = H5VL_log_zip_compress (r->sel_buf, inlen, dp->fp->zbuf, &clen);
            if ((err == 0) && (clen < inlen)) {
                memcpy (r->sel_buf, dp->fp->zbuf, clen);
                selsize = clen;  // New metadata size
#ifdef LOGVOL_PROFILING
                // Count size saved by compression
                H5VL_log_profile_add_time (dp->fp, TIMER_H5VL_LOG_FILEI_METASIZE_ZIP,
                                           (double)(r->sel_buf - r->meta_buf + selsize) / 1048576);
#endif
            } else {
                // Compressed size larger, abort compression
                r->hdr->flag &= ~(H5VL_LOGI_META_FLAG_SEL_DEFLATE);
            }
        }
        H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE_META_ZIP);
#endif

        // Resize metadata buffer
        if (r->hdr->meta_size > (int32_t) (r->sel_buf + selsize - r->meta_buf)) {
            r->resize (r->sel_buf - r->meta_buf + selsize);
        }
    }

    // Non-blocking?
    err = H5Pget_buffered (plist_id, &rtype);
    CHECK_ERR

    // Need convert?
    eqtype = H5Tequal (dip->dtype, mem_type_id);

    // Can reuse user buffer
    if (rtype == true && eqtype > 0 && mstype == H5S_SEL_ALL) {
        db.xbuf = db.ubuf;
    } else {  // Need internal buffer
        H5VL_LOGI_PROFILING_TIMER_START;
        // Get element size
        esize = H5Tget_size (mem_type_id);
        CHECK_ID (esize)

        // HDF5 type conversion is in place, allocate for whatever larger
        H5VL_log_filei_balloc (dp->fp, db.size * std::max (esize, (size_t) (dip->esize)),
                               (void **)(&(db.xbuf)));
        // err = H5VL_log_filei_pool_alloc (&(dp->fp->data_buf),
        //								 db.size * std::max (esize,
        //(size_t)
        //(dip->esize)), 								 (void
        //**)(&(r->xbuf)));
        // CHECK_ERR

        // Need packing
        if (mstype != H5S_SEL_ALL) {
            i = 0;

            H5VL_LOGI_PROFILING_TIMER_START
            H5VL_log_selections (mem_space_id).get_mpi_type (esize, &ptype);

            H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOGI_GET_DATASPACE_SEL_TYPE);

            MPI_Pack (db.ubuf, 1, ptype, db.xbuf, db.size * esize, &i, dp->fp->comm);

            LOG_VOL_ASSERT (i == (int)(db.size * esize))
        } else {
            memcpy (db.xbuf, db.ubuf, db.size * esize);
        }
        H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE_PACK);

        H5VL_LOGI_PROFILING_TIMER_START;
        // Need convert
        if (eqtype == 0) {
            void *bg = NULL;

            if (H5Tget_class (dip->dtype) == H5T_COMPOUND) bg = malloc (db.size * dip->esize);

            err = H5Tconvert (mem_type_id, dip->dtype, db.size, db.xbuf, bg, plist_id);
            free (bg);
        }
        H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE_CONVERT);
    }

    // Convert request size to number of bytes to be used by later routines
    db.size *= dip->esize;

    // Filtering
    H5VL_LOGI_PROFILING_TIMER_START;
    if (dip->filters.size ()) {
        char *buf_filter = NULL;
        int csize = 0;

        H5VL_logi_filter (dip->filters, db.xbuf, db.size, (void **)&buf_filter, &csize);

        if (db.xbuf != db.ubuf) { H5VL_log_filei_bfree (dp->fp, db.xbuf); }

        // Copy to xbuf
        // xbuf is managed buffer, buf is not, can't assign directly
        H5VL_log_filei_balloc (dp->fp, csize, (void **)(&(db.xbuf)));
        memcpy (db.xbuf, buf_filter, csize);
        free (buf_filter);
        db.size = csize;
    }
    H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE_FILTER);

    H5VL_LOGI_PROFILING_TIMER_START;
    // Put request in queue
    if (dp->fp->config & H5VL_FILEI_CONFIG_METADATA_MERGE) {
        // Allocate merged request if not yet allocated
        if (!(dp->fp->mreqs[dp->id])) {
            dp->fp->mreqs[dp->id] = new H5VL_log_merged_wreq_t (dp, 1);
        }
        dp->fp->mreqs[dp->id]->append (dp, db, dsel);
    } else {
        r->hdr->fsize = db.size;
        r->dbufs.push_back (db);
        // Append to request list
        dp->fp->wreqs.push_back (r);
        // Update total metadata size in wreqs
        dp->fp->mdsize += r->hdr->meta_size;
    }
    H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE_FINALIZE);

    H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE);
} /* end H5VL_log_dataseti_write() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataseti_read
 *
 * Purpose:     Reads data elements from a dataset into a buffer.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
void H5VL_log_dataseti_read (H5VL_log_dset_t *dp,
                             hid_t mem_type_id,
                             hid_t mem_space_id,
                             H5VL_log_selections *dsel,
                             hid_t plist_id,
                             void *buf,
                             void **req) {
    herr_t err                = 0;
    H5VL_log_dset_info_t *dip = dp->fp->dsets_info[dp->id];  // Dataset info
    size_t esize;                                            // Element size of mem_type_id
    htri_t eqtype;        // Is mem_type_id same as dataset external type
    H5VL_log_rreq_t *r;   // Request entry
    H5S_sel_type mstype;  // Type of selection in mem_space_id
    hbool_t rtype;        // Non-blocking?
    size_t num_pending_writes = 0;
    void *lib_state = NULL;
    void *lib_context = NULL;
    H5FD_mpio_xfer_t xfer_mode;
    H5VL_logi_err_finally finally (
        [&lib_state, &lib_context] () -> void { H5VL_logi_restore_lib_stat (lib_state, lib_context); });
    H5VL_LOGI_PROFILING_TIMER_START;

    H5VL_LOGI_PROFILING_TIMER_START;
    // Sanity check
    if (dsel->nsel == 0) return;
    if (!buf) ERR_OUT ("user buffer can't be NULL");
    H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_READ_INIT);

    // Reset hdf5 context to allow file operations within a dataset operation
    H5VL_logi_reset_lib_stat (lib_state, lib_context);

    // Check mem space selection
    if (mem_space_id == H5S_ALL)
        mstype = H5S_SEL_ALL;
    else if (mem_space_id == H5S_BLOCK)
        mstype = H5S_SEL_ALL;
    else {
        mstype = H5Sget_select_type (mem_space_id);
        CHECK_ID (mstype)
    }

    // Setting metadata;
    r          = new H5VL_log_rreq_t ();
    r->info    = dip;
    r->hdr.did = dp->id;
    r->ndim    = dip->ndim;
    r->ubuf    = (char *)buf;
    r->ptype   = MPI_DATATYPE_NULL;
    r->dtype   = -1;
    r->mtype   = -1;
    r->esize   = dip->esize;
    r->rsize   = dsel->get_sel_size ();  // Number of elements in selection
    r->sels    = dsel;

    // Non-blocking?
    err = H5Pget_buffered (plist_id, &rtype);
    CHECK_ERR

    // Need convert?
    eqtype = H5Tequal (dip->dtype, mem_type_id);
    CHECK_ID (eqtype);

    // Can reuse user buffer
    if (eqtype > 0 && mstype == H5S_SEL_ALL) {
        r->xbuf = r->ubuf;
    } else {  // Need internal buffer
        // Get element size
        esize = H5Tget_size (mem_type_id);
        CHECK_ID (esize)

        // HDF5 type conversion is in place, allocate for whatever larger
        H5VL_log_filei_balloc (dp->fp, r->rsize * std::max (esize, (size_t) (dip->esize)),
                               (void **)(&(r->xbuf)));

        // Need packing
        if (mstype != H5S_SEL_ALL) {
            H5VL_LOGI_PROFILING_TIMER_START;
            H5VL_log_selections (mem_space_id).get_mpi_type (esize, &(r->ptype));
            H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOGI_GET_DATASPACE_SEL_TYPE);
        }

        // Need convert
        if (eqtype == 0) {
            r->dtype = H5Tcopy (dip->dtype);
            CHECK_ID (r->dtype)
            r->mtype = H5Tcopy (mem_type_id);
            CHECK_ID (r->mtype)
        }
    }

    // Flush it immediately if blocking, otherwise place into queue
    if (rtype != true) {
        // flush pending write requests if collective
        err = H5Pget_dxpl_mpio (dp->fp->dxplid, &xfer_mode);
        CHECK_ERR
        if ((xfer_mode == H5FD_MPIO_COLLECTIVE) || dp->fp->np == 1) {
            num_pending_writes = H5VL_log_filei_get_num_pending_writes (dp->fp);
            MPI_Allreduce (MPI_IN_PLACE, &num_pending_writes, 1, MPI_UNSIGNED_LONG, MPI_MAX,
                           dp->fp->comm);
            if (num_pending_writes > 0) { H5VL_log_nb_flush_write_reqs (dp->fp); }
        }

        std::vector<H5VL_log_rreq_t *> tmp (1, r);
        H5VL_log_nb_flush_read_reqs (dp->fp, tmp, plist_id);
    } else {
        dp->fp->rreqs.push_back (r);
    }
    H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_READ);
} /* end H5VL_log_dataseti_read() */
