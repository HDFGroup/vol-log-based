/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <mpi.h>

#include <algorithm>
#include <vector>

#include "H5VL_log_file.hpp"
#include "H5VL_log_filei.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_idx.hpp"
#include "H5VL_logi_nb.hpp"

/*
H5VL_logi_compact_idx_t::H5VL_logi_compact_idx_entry_t::H5VL_logi_compact_idx_entry_t (
        MPI_Offset foff, size_t fsize, int ndim, int nsel)
        : foff (foff), fsize (fsize), nsel (nsel), dsize (0) {
        herr_t err	 = 0;
        this->blocks = malloc (sizeof (hsize_t) * nsel * ndim * 2);
        CHECK_PTR (this->blocks);
err_out:;
        if (err) { throw "OOM"; }
}
*/

H5VL_logi_compact_idx_t::H5VL_logi_compact_idx_entry_t::H5VL_logi_compact_idx_entry_t (
    MPI_Offset foff, size_t fsize, H5VL_logi_compact_idx_entry_t *ref)
    : blocks (ref), nsel (-1), foff (foff), fsize (fsize), dsize (0) {}

H5VL_logi_compact_idx_t::H5VL_logi_compact_idx_entry_t::H5VL_logi_compact_idx_entry_t (
    int ndim, H5VL_logi_metaentry_t &meta)
    : nsel (meta.sels.size ()), foff (meta.hdr.foff), fsize (meta.hdr.fsize), dsize (meta.dsize) {
    int encndim;
    hsize_t *start, *count;

    if (meta.hdr.flag & H5VL_LOGI_META_FLAG_REC) {
        encndim = ndim - 1;
        rec     = meta.sels[0].start[0];
    } else {
        encndim = ndim;
        rec     = -1;
    }
    blocks = malloc (sizeof (hsize_t) * nsel * encndim * 2);

    start = (hsize_t *)(blocks);
    count = start + encndim;
    for (auto &sel : meta.sels) {
        memcpy (start, sel.start, sizeof (hsize_t) * encndim);
        memcpy (count, sel.count, sizeof (hsize_t) * encndim);
        start = count + encndim;
        count = start + encndim;
    }
}

H5VL_logi_compact_idx_t::H5VL_logi_compact_idx_entry_t::~H5VL_logi_compact_idx_entry_t () {
    if (nsel >= 0 && (this->blocks)) { free (this->blocks); }
}

H5VL_logi_compact_idx_t::H5VL_logi_compact_idx_t (H5VL_log_file_t *fp) : H5VL_logi_idx_t (fp) {}

H5VL_logi_compact_idx_t::H5VL_logi_compact_idx_t (H5VL_log_file_t *fp, size_t size)
    : H5VL_logi_idx_t (fp) {
    this->reserve (size);
}

H5VL_logi_compact_idx_t::~H5VL_logi_compact_idx_t () {
    for (auto &idx : idxs) {
        for (auto ent : idx) { delete ent; }
    }
}

void H5VL_logi_compact_idx_t::clear () {
    for (auto &i : this->idxs) { i.clear (); }
}

void H5VL_logi_compact_idx_t::reserve (size_t size) {
    if (this->idxs.size () < size) { this->idxs.resize (size); }
}

void H5VL_logi_compact_idx_t::insert (H5VL_logi_metaentry_t &meta) {
    H5VL_logi_compact_idx_entry_t *entry;

    entry = new H5VL_logi_compact_idx_entry_t (fp->dsets_info[meta.hdr.did]->ndim, meta);

    this->idxs[meta.hdr.did].push_back (entry);
}

void H5VL_logi_compact_idx_t::parse_block (char *block, size_t size) {
    char *bufp = block;                                        // Buffer for raw metadata
    H5VL_logi_compact_idx_entry_t *centry;                     // Buffer of decoded metadata entry
    H5VL_logi_metaentry_t entry;                               // Buffer of decoded metadata entry
    std::map<char *, H5VL_logi_compact_idx_entry_t *> bcache;  // Cache for linked metadata entry

    if (fp->config & H5VL_FILEI_CONFIG_METADATA_SHARE) {  // Need to maintina cache if file contains
                                                          // referenced metadata entries
        while (bufp < block + size) {
            H5VL_logi_meta_hdr *hdr_tmp = (H5VL_logi_meta_hdr *)bufp;

            // Skip the search if dataset is unlinked
            if (fp->dsets_info[hdr_tmp->did]) {
#ifdef WORDS_BIGENDIAN
                H5VL_logi_lreverse ((uint32_t *)bufp,
                                    (uint32_t *)(bufp + sizeof (H5VL_logi_meta_hdr)));
#endif
                // Have to parse all entries for reference purpose
                if (hdr_tmp->flag & H5VL_LOGI_META_FLAG_SEL_REF) {
                    MPI_Offset rec, roff;

                    // Check if it is a record entry
                    if (hdr_tmp->flag & H5VL_LOGI_META_FLAG_REC) {
                        rec  = ((MPI_Offset *)(hdr_tmp + 1))[0];
                        roff = ((MPI_Offset *)(hdr_tmp + 1))[1];
#ifdef WORDS_BIGENDIAN
                        H5VL_logi_llreverse ((uint64_t *)(&rec));
#endif
                    } else {
                        roff = ((MPI_Offset *)(hdr_tmp + 1))[0];
                    }
#ifdef WORDS_BIGENDIAN
                    H5VL_logi_llreverse ((uint64_t *)(&roff));
#endif
                    centry      = new H5VL_logi_compact_idx_entry_t (hdr_tmp->foff, hdr_tmp->fsize,
                                                                bcache[bufp + roff]);
                    centry->rec = (hssize_t)rec;
                } else {
                    H5VL_logi_metaentry_decode (*(fp->dsets_info[hdr_tmp->did]), bufp, entry);

                    centry = new H5VL_logi_compact_idx_entry_t (fp->dsets_info[hdr_tmp->did]->ndim,
                                                                entry);

                    // Insert to cache
                    bcache[bufp] = centry;
                }
                // Insert to the index
                this->idxs[hdr_tmp->did].push_back (centry);
            }
            bufp += hdr_tmp->meta_size;
        }
    } else {
        while (bufp < block + size) {
            H5VL_logi_meta_hdr *hdr_tmp = (H5VL_logi_meta_hdr *)bufp;

            // Skip the search if dataset is unlinked
            if (fp->dsets_info[hdr_tmp->did]) {
#ifdef WORDS_BIGENDIAN
                H5VL_logi_lreverse ((uint32_t *)bufp,
                                    (uint32_t *)(bufp + sizeof (H5VL_logi_meta_hdr)));
#endif

                H5VL_logi_metaentry_decode (*(fp->dsets_info[hdr_tmp->did]), bufp, entry);

                centry =
                    new H5VL_logi_compact_idx_entry_t (fp->dsets_info[hdr_tmp->did]->ndim, entry);

                // Insert to the index
                this->idxs[hdr_tmp->did].push_back (centry);
            }
            bufp += hdr_tmp->meta_size;
        }
    }
}

static bool intersect (
    int ndim, hsize_t *sa, hsize_t *ca, hsize_t *sb, hsize_t *cb, hsize_t *so, hsize_t *co) {
    int i;

    for (i = 0; i < ndim; i++) {
        so[i] = std::max (sa[i], sb[i]);
        co[i] = std::min (sa[i] + ca[i], sb[i] + cb[i]);
        if (co[i] <= so[i]) return false;
        co[i] -= so[i];
    }

    return true;
}

void H5VL_logi_compact_idx_t::search (H5VL_log_rreq_t *req,
                                      std::vector<H5VL_log_idx_search_ret_t> &ret) {
    int i, j, k;
    int nsel;
    MPI_Offset doff = 0;
    hsize_t bsize;
    size_t soff;
    hsize_t *start, *count;
    hsize_t os[H5S_MAX_RANK], oc[H5S_MAX_RANK];
    H5VL_log_idx_search_ret_t cur;

    // Skip the search if dataset is unlinked
    if (!(fp->dsets_info[req->hdr.did])) { return; }

    soff = 0;
    for (i = 0; i < req->sels->nsel; i++) {
        for (auto ent : this->idxs[req->hdr.did]) {
            if (ent->nsel == -1) {
                nsel  = ((H5VL_logi_compact_idx_entry_t *)(ent->blocks))->nsel;
                start = (hsize_t *)(((H5VL_logi_compact_idx_entry_t *)(ent->blocks))->blocks);
            } else {
                nsel  = ent->nsel;
                start = (hsize_t *)(ent->blocks);
            }

            if (ent->rec >= 0) {
                cur.dstart[0] = 0;
                cur.dsize[0]  = 1;
                cur.mstart[0] = ent->rec - req->sels->starts[i][0];
                cur.msize[0]  = req->sels->counts[i][0];
                cur.count[0]  = 1;

                if ((cur.mstart[0] < 0) || (cur.mstart[0] >= cur.msize[0])) { continue; }

                count = start + req->ndim - 1;
                doff  = 0;
                while (nsel--) {
                    if (intersect (req->ndim - 1, start, count, req->sels->starts[i] + 1,
                                   req->sels->counts[i] + 1, os, oc)) {
                        for (j = 1; j < req->ndim; j++) {
                            cur.dstart[j] = os[j - 1] - start[j - 1];
                            cur.dsize[j]  = count[j - 1];
                            cur.mstart[j] = os[j - 1] - req->sels->starts[i][j];
                            cur.msize[j]  = req->sels->counts[i][j];
                            cur.count[j]  = oc[j - 1];
                        }
                        cur.info  = req->info;
                        cur.foff  = ent->foff;
                        cur.fsize = ent->fsize;
                        cur.doff  = doff;
                        cur.xsize = ent->dsize;
                        cur.xbuf  = req->xbuf + soff;
                        ret.push_back (cur);
                    }

                    // Calculate doff on the fly
                    bsize = fp->dsets_info[req->hdr.did]->esize;
                    for (k = 0; k < req->ndim - 1; k++) { bsize *= count[k]; }
                    doff += (MPI_Offset)bsize;

                    // Advance start and ount
                    start = count + req->ndim - 1;
                    count = start + req->ndim - 1;
                }
            } else {
                count = start + req->ndim;
                doff  = 0;
                while (nsel--) {
                    if (intersect (req->ndim, start, count, req->sels->starts[i],
                                   req->sels->counts[i], os, oc)) {
                        for (j = 0; j < req->ndim; j++) {
                            cur.dstart[j] = os[j] - start[j];
                            cur.dsize[j]  = count[j];
                            cur.mstart[j] = os[j] - req->sels->starts[i][j];
                            cur.msize[j]  = req->sels->counts[i][j];
                            cur.count[j]  = oc[j];
                        }
                        cur.info  = req->info;
                        cur.foff  = ent->foff;
                        cur.fsize = ent->fsize;
                        cur.doff  = doff;
                        cur.xsize = ent->dsize;
                        cur.xbuf  = req->xbuf + soff;
                        ret.push_back (cur);
                    }

                    // Calculate doff on the fly
                    bsize = fp->dsets_info[req->hdr.did]->esize;
                    for (k = 0; k < req->ndim; k++) { bsize *= count[k]; }
                    doff += (MPI_Offset)bsize;

                    // Advance start and ount
                    start = count + req->ndim;
                    count = start + req->ndim;
                }
            }
        }
        soff += req->sels->get_sel_size (i) * req->esize;
    }
}
