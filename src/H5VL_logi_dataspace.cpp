/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <hdf5.h>
#include <mpi.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "H5VL_log_dataset.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_dataspace.hpp"
#include "H5VL_logi_debug.hpp"
#include "H5VL_logi_err.hpp"
#include "H5VL_logi_meta.hpp"
#include "H5VL_logi_util.hpp"

template <class T>
static bool lessthan (int ndim, T *a, T *b) {
    int i;

    for (i = 0; i < ndim; i++) {
        if (*a < *b) {
            return true;
        } else if (*b < *a) {
            return false;
        }
        ++a;
        ++b;
    }

    return false;
}

#define ELEM_SWAP_EX(A, B)     \
    {                          \
        tmp       = starts[A]; \
        starts[A] = starts[B]; \
        starts[B] = tmp;       \
        tmp       = counts[A]; \
        counts[A] = counts[B]; \
        counts[B] = tmp;       \
    }

static void sortblocks (int ndim, int len, hsize_t **starts, hsize_t **counts) {
    int i, j, k;
    hsize_t *tmp1, *tmp2;
    std::vector<int> idx (len);

    for (i = 0; i < len; i++) { idx[i] = i; }

    auto comp = [&] (int &l, int &r) -> bool { return lessthan (ndim, starts[l], starts[r]); };
    std::sort (idx.begin (), idx.end (), comp);

    // Reorder according to index
    for (i = 0; i < len; i++) {
        if (idx[i] != i) {
            // Pull out element i
            tmp1 = starts[i];
            tmp2 = counts[i];

            // Cyclic swap until we ran back
            j = i;
            k = idx[j];
            while (k != i) {
                starts[j] = starts[k];
                counts[j] = counts[k];
                idx[j]    = j;

                j = k;
                k = idx[j];
            }

            starts[j] = tmp1;
            counts[j] = tmp2;
            idx[j]    = j;
        }
    }
}

template <bool use_end, typename T>
static void merge_blocks (int ndim, T &len, hsize_t **starts, hsize_t **counts) {
    int d, i, j, k;
    std::vector<int> idx (len);

    for (d = ndim - 1; d > -1; d--) {
        j = 0;
        for (i = 1; i < len; i++) {
            for (k = 0; k < ndim; k++) {
                if (k == d) {
                    if (use_end) {
                        if (counts[j][k] + 1 !=
                            starts[i][k]) {  // start of next block must follow end of prev block
                            break;           // Cannot merge
                        }
                    } else {
                        if (starts[j][k] + counts[j][k] !=
                            starts[i][k]) {  // start + count of prev block must match start of next
                                             // block
                            break;           // Cannot merge
                        }
                    }
                } else {
                    if (starts[i][k] != starts[j][k] ||
                        counts[i][k] != counts[j][k]) {  // size and position must align on
                                                         // non-merging dimensions
                        break;                           // Cannot merge
                    }
                }
            }
            if (k == ndim) {  // Merge
                if (use_end) {
                    counts[j][d] = counts[i][d];
                } else {
                    counts[j][d] += counts[i][d];
                }

            } else {
                j++;  // Can't merge
                starts[j] = starts[i];
                counts[j] = counts[i];
            }
        }
        len = j + 1;
    }
}

H5VL_log_selections::H5VL_log_selections () : ndim (0), nsel (0), dims (NULL), sels_arr (NULL) {}

H5VL_log_selections::H5VL_log_selections (int ndim, hsize_t *dims, int nsel) : ndim (ndim) {
    herr_t err = 0;
    if (dims) {
        this->dims = (hsize_t *)malloc (sizeof (hsize_t) * ndim);
        CHECK_PTR (this->dims)
        memcpy (this->dims, dims, sizeof (hsize_t) * ndim);
    } else {
        this->dims = NULL;
    }
    this->nsel = nsel;

    if (err) { throw "OOM"; }
}

H5VL_log_selections::H5VL_log_selections (
    int ndim, hsize_t *dims, int nsel, hsize_t **starts, hsize_t **counts)
    : H5VL_log_selections (ndim, dims, nsel) {
    this->starts = starts;
    this->counts = counts;
}

H5VL_log_selections::H5VL_log_selections (H5VL_log_selections &rhs) {
    this->sels_arr = NULL;
    *this          = rhs;
}

H5VL_log_selections::H5VL_log_selections (hid_t dsid) {
    herr_t err = 0;
    int i, j, k, l;
    int nreq;            // Number of non-interleaving sections in the selection
    int old_nreq;        // Number of non-interleaving sections in previous processed groups
    int nbreq;           // Number of non-interleaving sections in current block
    hssize_t nblock;     // Number of blocks in the selection (before breaking interleaving blocks)
    H5S_sel_type stype;  // Tpye of selection (block list, point list ...)
    hsize_t **hstarts = NULL, **hends;  // Output buffer of H5Sget_select_hyper_nblocks
    int *group        = NULL;           // blocks with the same group number are interleaved
    htri_t isregular;                   // If hyperslab selection is regular

    // Get space dim
    ndim = H5Sget_simple_extent_ndims (dsid);
    CHECK_ID (ndim)
    this->dims = (hsize_t *)malloc (sizeof (hsize_t) * ndim);
    CHECK_PTR (this->dims)
    ndim = H5Sget_simple_extent_dims (dsid, this->dims, NULL);
    LOG_VOL_ASSERT (ndim == this->ndim)

    // Get selection type
    if (dsid == H5S_ALL)
        stype = H5S_SEL_ALL;
    else {
        stype = H5Sget_select_type (dsid);
        CHECK_ID (stype)
    }

    switch (stype) {
        case H5S_SEL_HYPERSLABS: {
            isregular = H5Sis_regular_hyperslab (dsid);
            if (isregular == true) {
                hsize_t start[H5S_MAX_RANK], count[H5S_MAX_RANK], stride[H5S_MAX_RANK],
                    block[H5S_MAX_RANK];
                err = H5Sget_regular_hyperslab (dsid, start, stride, count, block);
                CHECK_ERR

                int is_stride_all_ones = true;
                int is_count_all_ones = true;
                int is_block_all_ones = true;
                for (i = 0; i < ndim; i++) {
                    if (stride[i] != 1) is_stride_all_ones = false;
                    if (count[i]  != 1) is_count_all_ones  = false;
                    if (block[i]  != 1) is_block_all_ones  = false;
                }

                if ((is_stride_all_ones && is_block_all_ones) || is_count_all_ones) {
                    this->nsel = 1;   /* there is only 1 block in this case */
                    this->alloc (1);
                    for (i = 0; i < ndim; i++) {
                        starts[0][i] = start[i];
                        counts[0][i] = (is_block_all_ones) ? count[i] : block[i];
                    }
                    break;
                }
            }

            nblock = H5Sget_select_hyper_nblocks (dsid);
            CHECK_ID (nblock)

            if (nblock == 1) {
                hsize_t cord[H5S_MAX_RANK * 2];

                // Allocate buffer
                this->nsel = nblock;
                this->alloc (nblock);

                err = H5Sget_select_hyper_blocklist (dsid, 0, 1, cord);

                for (i = 0; i < ndim; i++) {
                    starts[0][i] = (MPI_Offset)cord[i];
                    counts[0][i] = (MPI_Offset)cord[i + ndim] - starts[0][i] + 1;
                }
            } else {
                hstarts = (hsize_t **)malloc (sizeof (hsize_t *) * nblock * 2);
                CHECK_PTR (hstarts)
                hends = hstarts + nblock;

                hstarts[0] = (hsize_t *)malloc (sizeof (hsize_t) * ndim * 2 * nblock);
                CHECK_PTR (hstarts[0])
                hends[0] = hstarts[0] + ndim;
                for (i = 1; i < nblock; i++) {
                    hstarts[i] = hstarts[i - 1] + ndim * 2;
                    hends[i]   = hends[i - 1] + ndim * 2;
                }

                err = H5Sget_select_hyper_blocklist (dsid, 0, nblock, hstarts[0]);

                merge_blocks<true> (ndim, nblock, hstarts, hends);

                sortblocks (ndim, nblock, hstarts, hends);

                merge_blocks<true> (ndim, nblock, hstarts, hends);

                // Check for interleving
                group = (int *)malloc (sizeof (int) * (nblock + 1));
                CHECK_PTR (group)
                group[0] = 0;
                for (i = 0; i < nblock - 1; i++) {
                    if (lessthan (
                            ndim, hends[i],
                            hstarts[i + 1])) {  // The end cord is the max offset of the
                                                // previous block. If less than the start offset
                                                // of the next block, there won't be interleving
                        group[i + 1] = group[i] + 1;
                    } else {
                        group[i + 1] = group[i];
                    }
                }
                group[i + 1] = group[i] + 1;  // Dummy group to trigger process for final group

                // Count number of requests
                j    = 0;
                nreq = 0;
                for (i = j = 0; i <= nblock; i++) {
                    if (group[i] != group[j]) {  // Within 1 group
                        if (i - j == 1) {        // Sinlge block, no need to breakdown
                            // offs[k] = nreq; // Record offset
                            nreq++;
                            j++;
                        } else {
                            for (; j < i; j++) {
                                // offs[i] = nreq; // Record offset
                                nbreq = 1;
                                for (k = 0; k < ndim - 1; k++) {
                                    nbreq *= hends[j][k] - hstarts[j][k] + 1;
                                }
                                nreq += nbreq;
                            }
                        }
                    }
                }

                // Allocate buffer
                this->nsel = nreq;
                this->alloc (nreq);

                // Fill up selections
                nreq = 0;
                for (i = j = 0; i <= nblock; i++) {
                    if (group[i] != group[j]) {  // Within 1 group
                        if (i - j == 1) {        // Sinlge block, no need to breakdown
                            for (k = 0; k < ndim; k++) {
                                starts[nreq][k] = hstarts[j][k];
                                counts[nreq][k] = hends[j][k] - hstarts[j][k] + 1;
                            }
                            // offs[k] = nreq; // Record offset
                            nreq++;
                            j++;
                        } else {
                            old_nreq = nreq;
                            for (; j < i; j++) {  // Breakdown each block
                                for (k = 0; k < ndim; k++) {
                                    starts[nreq][k] = (hsize_t)hstarts[j][k];
                                    counts[nreq][k] = 1;
                                }
                                counts[nreq][ndim - 1] =
                                    (hsize_t) (hends[j][ndim - 1] - hstarts[j][ndim - 1] + 1);

                                for (l = 0; l < ndim; l++) {  // The lowest dimension that we
                                                              // haven't reach the end
                                    if (starts[nreq][l] < hends[j][l]) break;
                                }
                                nreq++;
                                while (l < ndim - 1) {  // While we haven't handle the last one
                                    memcpy (starts[nreq], starts[nreq - 1],
                                            sizeof (hsize_t) * ndim);  // Start from previous value
                                    memcpy (counts[nreq], counts[nreq - 1],
                                            sizeof (hsize_t) * ndim);  // Fill in Count

                                    // Increase start to the next location, carry to lower dim
                                    // if overflow
                                    starts[nreq][ndim - 2]++;
                                    for (k = ndim - 2; k > 0; k--) {
                                        if (starts[nreq][k] > hends[j][k]) {
                                            starts[nreq][k] = hstarts[j][k];
                                            starts[nreq][k - 1]++;
                                        } else {
                                            break;
                                        }
                                    }

                                    for (l = 0; l < ndim; l++) {  // The lowest dimension that
                                                                  // we haven't reach the end
                                        if (starts[nreq][l] < hends[j][l]) break;
                                    }
                                    nreq++;
                                }
                            }

                            // Sort into non-decreasing order
                            sortblocks (ndim, nreq - old_nreq, starts + old_nreq,
                                        counts + old_nreq);

                            // HDF5 selection guarantees no interleving, still merge coaleasable
                            // blocks
                            k = nreq - old_nreq;
                            merge_blocks<false> (ndim, k, starts + old_nreq, counts + old_nreq);
                            nreq = old_nreq + k;
                        }
                    }
                }
            }
        } break;
        case H5S_SEL_POINTS: {
            nblock = H5Sget_select_elem_npoints (dsid);
            CHECK_ID (nblock)

            this->nsel = nblock;
            this->alloc (nblock);

            if (nblock) {
                hstarts    = (hsize_t **)malloc (sizeof (hsize_t) * nblock);
                hstarts[0] = (hsize_t *)malloc (sizeof (hsize_t) * ndim * nblock);
                for (i = 1; i < nblock; i++) { hstarts[i] = hstarts[i - 1] + ndim; }

                err = H5Sget_select_elem_pointlist (dsid, 0, nblock, hstarts[0]);
                CHECK_ERR

                for (i = 0; i < nblock; i++) {
                    for (j = 0; j < ndim; j++) {
                        starts[i][j] = (MPI_Offset)hstarts[i][j];
                        counts[i][j] = 1;
                    }
                }

                // Merge blocks
                auto comp = [this] (hsize_t *l, hsize_t *r) -> bool {
                    return lessthan (this->ndim, l, r);
                };
                std::sort (starts, starts + nblock, comp);
                merge_blocks<false> (ndim, nblock, starts, counts);
            }
        } break;
        case H5S_SEL_ALL: {
            hsize_t dims[32];

            /* check if file space is created from H5S_NULL */
            if (0 == H5Sget_simple_extent_npoints (dsid)) {
                this->nsel = 0;
                this->alloc (0);
                goto err_out;
            }

            ndim = H5Sget_simple_extent_dims (dsid, dims, NULL);
            CHECK_ID (ndim)

            this->nsel = 1;
            this->alloc (1);

            for (j = 0; j < ndim; j++) {
                starts[0][j] = 0;
                counts[0][j] = (MPI_Offset)dims[j];
            }

        } break;
        case H5S_SEL_NONE: {
            this->nsel = 0;
            this->alloc (0);
        } break;
        default:
            ERR_OUT ("Unsupported selection type");
    }

err_out:
    if (hstarts) {
        free (hstarts[0]);
        free (hstarts);
    }
    if (group) { free (group); }

    if (err) { throw "Can not retrieve selections"; }
}

H5VL_log_selections::~H5VL_log_selections () {
    if (dims) { free (dims); }
    if (sels_arr) {
        if (starts[0]) { free (starts[0]); }
        free (starts);
        sels_arr = NULL;
    }
}

H5VL_log_selections &H5VL_log_selections::operator= (H5VL_log_selections &rhs) {
    int i;

    // Deallocate existing start and count array
    if (sels_arr) {
        if (starts[0]) { free (starts[0]); }
        free (starts);
        sels_arr = NULL;
    }

    // Copy dimension sizes
    if (rhs.dims) {
        if (this->ndim < rhs.ndim) {
            this->dims = (hsize_t *)realloc (this->dims, sizeof (hsize_t) * rhs.ndim);
            CHECK_PTR (this->dims)
        }
        memcpy (this->dims, rhs.dims, sizeof (hsize_t) * rhs.ndim);
    } else {
        this->dims = NULL;
    }

    this->nsel = rhs.nsel;
    this->ndim = rhs.ndim;

    if (rhs.sels_arr) {
        this->alloc (nsel);

        // Copy the data
        for (i = 0; i < nsel; i++) {
            memcpy (starts[i], rhs.starts[i], sizeof (hsize_t) * ndim);
            memcpy (counts[i], rhs.counts[i], sizeof (hsize_t) * ndim);
        }
    } else {
        this->starts   = rhs.starts;
        this->counts   = rhs.counts;
        this->sels_arr = NULL;
    }

    return *this;
}

// Assume blocks sorted according to start
bool H5VL_log_selections::operator== (H5VL_log_selections &rhs) {
    int i, j;

    if (ndim != rhs.ndim) { return false; }
    if (nsel != rhs.nsel) { return false; }
    for (i = 0; i < nsel; i++) {
        for (j = 0; j < ndim; j++) {
            if (starts[i][j] != rhs.starts[i][j]) { return false; }
            if (counts[i][j] != rhs.counts[i][j]) { return false; }
        }
    }

    return true;
}

// Should only be called once
void H5VL_log_selections::alloc (int nsel) {
    int i;

    if (nsel) {
        this->starts = sels_arr = (hsize_t **)malloc (sizeof (hsize_t *) * nsel * 2);
        CHECK_PTR (sels_arr)
        this->counts = this->starts + nsel;
        if (ndim) {
            this->starts[0] = (hsize_t *)malloc (sizeof (hsize_t) * nsel * ndim * 2);
            CHECK_PTR (this->starts[0])
        } else {
            this->starts[0] = NULL;
        }
        this->counts[0] = this->starts[0] + nsel * ndim;
        for (i = 1; i < nsel; i++) {
            starts[i] = starts[i - 1] + ndim;
            counts[i] = counts[i - 1] + ndim;
        }
    } else {
        this->starts   = NULL;
        this->counts   = NULL;
        this->sels_arr = NULL;
    }
}

void H5VL_log_selections::convert_to_deep () {
    int i;
    hsize_t **starts_tmp;
    hsize_t **counts_tmp;

    if (!sels_arr) {
        starts_tmp = starts;
        counts_tmp = counts;

        // Allocate new space
        this->alloc (nsel);

        // Copy the data
        for (i = 0; i < nsel; i++) {
            memcpy (starts[i], starts_tmp[i], sizeof (hsize_t) * ndim);
            memcpy (counts[i], counts_tmp[i], sizeof (hsize_t) * ndim);
        }
    }
}

void H5VL_log_selections::get_mpi_type (size_t esize, MPI_Datatype *type) {
    herr_t err = 0;
    int mpierr;
    int i, j;
    bool derived_etype;
    MPI_Aint *offs      = NULL;
    int *lens           = NULL;
    MPI_Datatype *types = NULL;
    MPI_Datatype etype;
    int size[H5S_MAX_RANK], ssize[H5S_MAX_RANK], sstart[H5S_MAX_RANK];
    H5VL_logi_err_finally finally ([&types, &offs, &lens, &derived_etype, &etype, this] () -> void {
        int i;
        if (types != NULL) {
            for (i = 0; i < this->nsel; i++) {
                if (types[i] != MPI_BYTE) { MPI_Type_free (types + i); }
            }
            free (types);
        }
        free (offs);
        free (lens);

        if (derived_etype) { MPI_Type_free (&etype); }
    });

    if (!dims) { RET_ERR ("No dataspace dimension information") }

    etype = H5VL_logi_get_mpi_type_by_size (esize);
    if (etype == MPI_DATATYPE_NULL) {
        mpierr = MPI_Type_contiguous (esize, MPI_BYTE, &etype);
        CHECK_MPIERR
        mpierr = MPI_Type_commit (&etype);
        CHECK_MPIERR
        derived_etype = true;
    } else {
        derived_etype = false;
    }

    for (i = 0; i < ndim; i++) { size[i] = (int)dims[i]; }

    // No selection, return
    if (nsel == 0) {
        *type = MPI_DATATYPE_NULL;
        return;
    }

    lens = (int *)malloc (sizeof (int) * nsel);
    CHECK_PTR (lens)
    offs = (MPI_Aint *)malloc (sizeof (MPI_Aint) * nsel);
    CHECK_PTR (offs)
    types = (MPI_Datatype *)malloc (sizeof (MPI_Datatype) * nsel);
    CHECK_PTR (types)

    // Construct the actual type
    for (i = 0; i < nsel; i++) {
        for (j = 0; j < ndim; j++) {
            ssize[j]  = (int)(counts[i][j]);
            sstart[j] = (int)(starts[i][j]);
        }

        mpierr = H5VL_log_debug_MPI_Type_create_subarray (ndim, size, ssize, sstart, MPI_ORDER_C,
                                                          etype, types + i);
        CHECK_MPIERR
        mpierr = MPI_Type_commit (types + i);
        CHECK_MPIERR
        offs[i] = 0;
        lens[i] = 1;
    }

    mpierr = MPI_Type_create_struct (nsel, lens, offs, types, type);
    CHECK_MPIERR
    mpierr = MPI_Type_commit (type);
    CHECK_MPIERR
}

hsize_t H5VL_log_selections::get_sel_size (int idx) {
    hsize_t ret = 1;
    hsize_t *ptr;

    if (idx < nsel) {
        if (counts) {
            for (ptr = counts[idx]; ptr < counts[idx] + ndim; ptr++) { ret *= *ptr; }
        }
    } else {
        ret = 0;
    }
    return ret;
}

hsize_t H5VL_log_selections::get_sel_size () {
    int i;
    hsize_t ret = 0;
    hsize_t bsize;
    hsize_t *ptr;

    if ((!counts) && nsel) return 1;

    for (i = 0; i < nsel; i++) {
        bsize = 1;
        for (ptr = counts[i]; ptr < counts[i] + ndim; ptr++) { bsize *= *ptr; }
        ret += bsize;
    }

    return ret;
}

void H5VL_log_selections::encode (char *mbuf, MPI_Offset *dsteps, int dimoff) {
    int i;

    if (starts) {
        if (dsteps) {
            for (i = 0; i < nsel; i++) {
                H5VL_logi_sel_encode (ndim - dimoff, dsteps + dimoff, starts[i] + dimoff,
                                      (MPI_Offset *)mbuf);
                mbuf += sizeof (MPI_Offset);
            }
            for (i = 0; i < nsel; i++) {
                H5VL_logi_sel_encode (ndim - dimoff, dsteps + dimoff, counts[i] + dimoff,
                                      (MPI_Offset *)mbuf);
                mbuf += sizeof (MPI_Offset);
            }
        } else {
            for (i = 0; i < nsel; i++) {
                memcpy (mbuf, starts[i] + dimoff, sizeof (hsize_t) * (ndim - dimoff));
                mbuf += sizeof (hsize_t) * (ndim - dimoff);
            }
            for (i = 0; i < nsel; i++) {
                memcpy (mbuf, counts[i] + dimoff, sizeof (hsize_t) * (ndim - dimoff));
                mbuf += sizeof (hsize_t) * (ndim - dimoff);
            }
        }
    }
}
