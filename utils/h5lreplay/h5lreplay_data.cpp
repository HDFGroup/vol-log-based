#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
//
#include <hdf5.h>
#include <mpi.h>
//
#include "H5VL_log_dataset.hpp"
#include "H5VL_logi_filter.hpp"
#include "H5VL_logi_util.hpp"
#include "h5lreplay.hpp"
#include "h5lreplay_data.hpp"
#include "h5lreplay_meta.hpp"

typedef struct hidx {
	MPI_Aint foff;
	MPI_Aint moff;
	int len;
	bool operator< (const struct hidx &rhs) const { return foff < rhs.foff; }
} hidx;

herr_t h5lreplay_read_data (MPI_File fin,
							std::vector<dset_info> &dsets,
							std::vector<h5lreplay_idx_t> &reqs) {
	herr_t err = 0;
	int mpierr;
	int i, j;
	size_t bsize;	// Data buffer size of a req
	size_t esize;	// Size of a sel block
	size_t zbsize;	// Size of the decompression buffer
	char *buf;
	std::vector<MPI_Aint> foffs, moffs;
	std::vector<int> lens;
	std::vector<hidx> idxs;
	MPI_Datatype ftype = MPI_DATATYPE_NULL;
	MPI_Datatype mtype = MPI_DATATYPE_NULL;
	MPI_Status stat;

	// Allocate buffer
	zbsize = 0;
	for (auto &reqp : reqs) {
		for (auto &req : reqp.entries) {
			req.bufs.resize (req.sels.size ());
			bsize = 0;
			for (j = 0; j < (int)(req.sels.size ()); j++) {
				esize = dsets[req.hdr.did].esize;
				for (i = 0; i < (int)(dsets[req.hdr.did].ndim); i++) {
					esize *= req.sels[j].count[i];
				}
				req.bufs[j] = (char *)bsize;
				bsize += esize;
			}
			if (zbsize < bsize) { zbsize = bsize; }
			// Comrpessed size can be larger
			if (bsize < (size_t) (req.hdr.fsize)) { bsize = req.hdr.fsize; }
			// Allocate buffer
			buf = (char *)malloc (bsize);
			assert (buf != NULL);
			for (j = 0; j < (int)(req.sels.size ()); j++) {
				req.bufs[j] = buf + (size_t)req.bufs[j];
			}

			// Record off and len for mpi type
			idxs.push_back ({req.hdr.foff, (MPI_Aint)req.bufs[0], (int)req.hdr.fsize});
		}
	}
	// zbuf = (char *)malloc (zbsize);

	// Read the data
	foffs.reserve (idxs.size ());
	moffs.reserve (idxs.size ());
	lens.reserve (idxs.size ());
	std::sort (idxs.begin (), idxs.end ());
	for (auto &idx : idxs) {
		foffs.push_back (idx.foff);
		moffs.push_back (idx.moff);
		lens.push_back (idx.len);
	}
	mpierr =
		MPI_Type_create_hindexed (foffs.size (), lens.data (), foffs.data (), MPI_BYTE, &ftype);
	CHECK_MPIERR
	mpierr =
		MPI_Type_create_hindexed (moffs.size (), lens.data (), moffs.data (), MPI_BYTE, &mtype);
	CHECK_MPIERR
	mpierr = MPI_Type_commit (&ftype);
	CHECK_MPIERR
	mpierr = MPI_Type_commit (&mtype);
	CHECK_MPIERR
	mpierr = MPI_File_set_view (fin, 0, MPI_BYTE, ftype, "native", MPI_INFO_NULL);
	CHECK_MPIERR
	mpierr = MPI_File_read_all (fin, MPI_BOTTOM, 1, mtype, &stat);
	CHECK_MPIERR

	// Unfilter the data
	for (auto &reqp : reqs) {
		for (auto &req : reqp.entries) {
			if (dsets[req.hdr.did].filters.size () > 0) {
				char *buf = NULL;
				int csize = 0;

				err = H5VL_logi_unfilter (dsets[req.hdr.did].filters, req.bufs[0], req.hdr.fsize,
										  (void **)&buf, &csize);
				CHECK_ERR

				memcpy (buf, req.bufs[0], csize);
			}
		}
	}

err_out:;
	if (ftype != MPI_DATATYPE_NULL) { MPI_Type_free (&ftype); }
	if (mtype != MPI_DATATYPE_NULL) { MPI_Type_free (&mtype); }
	return err;
}

bool h5lreplay_write_req::operator< (const h5lreplay_write_req &rhs) const {
	int i;

	if (info->foff < rhs.info->foff)
		return true;
	else if (info->foff > rhs.info->foff)
		return false;

	for (i = 0; i < (int)(info->ndim); i++) {
		if (start[i] < rhs.start[i])
			return true;
		else if (start[i] > rhs.start[i])
			return false;
	}

	return false;
}

herr_t h5lreplay_write_data (MPI_File fout,
							 std::vector<dset_info> &dsets,
							 std::vector<h5lreplay_idx_t> &reqs) {
	herr_t err = 0;
	int mpierr;
	int i, j, k;
	// hid_t dsid = -1;
	// hid_t msid = -1;
	std::vector<h5lreplay_write_req> blocks;  // H5Dwrite reqeusts
	// hsize_t one[H5S_MAX_RANK];
	// hsize_t zero = 0;
	// hsize_t msize;
	MPI_Datatype ftype;	 // File type for writing
	MPI_Datatype mtype;	 // Memory type for writing
	MPI_Status stat;

	/*
	one[0] = INT_MAX;
	msid   = H5Screate_simple (1, one, one);

	for (i = 0; i < H5S_MAX_RANK; i++) { one[i] = 1; }
	*/

	for (i = 0; i < (int)(dsets.size ()); i++) {
		// dsid = H5Dget_space (dsets[i].did);
		for (auto &req : reqs[i].entries) {
			for (k = 0; k < (int)(req.sels.size ()); k++) {
				// Code to replay as is
				/*
				msize = 1;
				for (j = 0; j < (int)(dsets[req.hdr.did].ndim); j++) {
					msize *= req.sels[k].count[j];
				}
				err = H5Sselect_hyperslab (msid, H5S_SELECT_SET, &zero, NULL, one, &msize);
				CHECK_ERR
				err = H5Sselect_hyperslab (dsid, H5S_SELECT_SET, req.sels[k].start, NULL, one,
										   req.sels[k].count);
				CHECK_ERR
				err = H5Dwrite (dsets[i].did, dsets[i].dtype, msid, dsid, H5P_DEFAULT, req.bufs[k]);
				CHECK_ERR
				*/

				blocks.push_back ({req.bufs[k], req.sels[k].start, req.sels[k].count, &(dsets[i])});
			}
		}
		// H5Sclose (dsid);
		// dsid = -1;
	}

	err = h5lreplay_write_data_gen_types (blocks, &ftype, &mtype);
	CHECK_ERR

	mpierr = MPI_Type_commit (&mtype);
	CHECK_MPIERR
	mpierr = MPI_Type_commit (&ftype);
	CHECK_MPIERR

	mpierr = MPI_File_set_view (fout, 0, MPI_BYTE, ftype, "native", MPI_INFO_NULL);
	CHECK_MPIERR

	mpierr = MPI_File_write_at_all (fout, 0, MPI_BOTTOM, 1, mtype, &stat);
	CHECK_MPIERR
err_out:;
	if (mtype != MPI_DATATYPE_NULL) MPI_Type_free (&mtype);
	if (ftype != MPI_DATATYPE_NULL) MPI_Type_free (&ftype);
	// if (dsid >= 0) { H5Sclose (dsid); }
	// if (msid >= 0) { H5Sclose (msid); }
	return err;
}

static bool interleve (int ndim, hsize_t *sa, hsize_t *ca, hsize_t *sb) {
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
		to	  = oa[A]; \
		oa[A] = oa[B]; \
		oa[B] = to;    \
		to	  = ob[A]; \
		ob[A] = ob[B]; \
		ob[B] = to;    \
		tl	  = l[A];  \
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
// Assume no overlaping write
herr_t h5lreplay_write_data_gen_types (std::vector<h5lreplay_write_req> blocks,
									   MPI_Datatype *ftype,
									   MPI_Datatype *mtype) {
	herr_t err = 0;
	int mpierr;
	int32_t i, j, k, l;
	int nblock = blocks.size ();  // Number of place to read
	std::vector<bool> newgroup (nblock,
								0);	 // Whether the current block interleave with previous block
	int nt;							 // Number of sub-types in ftype and mtype
	int nrow;	 // Number of contiguous sections after breaking down a set of interleaving blocks
	int old_nt;	 // Number of elements in foffs, moffs, and lens that has been sorted by foffs
	int *lens;	 // array_of_blocklengths in MPI_Type_create_struct for ftype and mtype
	MPI_Aint *foffs		 = NULL;			   // array_of_displacements in ftype
	MPI_Aint *moffs		 = NULL;			   // array_of_displacements in mtype
	MPI_Datatype *ftypes = NULL;			   // array_of_types in ftype
	MPI_Datatype *mtypes = NULL;			   // array_of_types in mtype
	MPI_Datatype etype	 = MPI_DATATYPE_NULL;  // element type for each ftypes and mtypes
	MPI_Offset ctr[H5S_MAX_RANK];  // Logical position of the current contiguous section in the
								   // dataspace of the block being broken down
	int tsize[H5S_MAX_RANK];	   // MPI_Type_create_subarray size
	int tssize[H5S_MAX_RANK];	   // MPI_Type_create_subarray ssize
	int tstart[H5S_MAX_RANK];	   // MPI_Type_create_subarray start
	int nelem;					   // Number of elements selected in a block
	char *bufp;					   // Memory buffer corresponding to ctr

	if (!nblock) {
		*ftype = *mtype = MPI_DATATYPE_NULL;
		return 0;
	}

	std::sort (blocks.begin (), blocks.end ());

	for (i = 0; i < nblock - 1; i++) {
		if ((blocks[i].info->foff == blocks[i + 1].info->foff) &&
			interleve (blocks[i].info->ndim, blocks[i].start, blocks[i].count,
					   blocks[i + 1].start)) {
			newgroup[i] = false;
		} else {
			newgroup[i] = true;
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

				if (blocks[i].info->ndim) {
					nelem = 1;
					for (k = 0; k < blocks[i].info->ndim; k++) {
						tsize[k]  = (int)(blocks[j].info->dims[k]);
						tssize[k] = (int)(blocks[j].count[k]);
						tstart[k] = (int)(blocks[j].start[k]);
						nelem *= blocks[j].count[k];
					}

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

					mpierr = H5VL_log_debug_MPI_Type_create_subarray (blocks[i].info->ndim, tsize,
																	  tssize, tstart, MPI_ORDER_C,
																	  etype, ftypes + nt);
					CHECK_MPIERR
					mpierr = MPI_Type_commit (ftypes + nt);
					CHECK_MPIERR

					mpierr = MPI_Type_contiguous (nelem, etype, mtypes + nt);
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

				foffs[nt] = blocks[j].info->foff;
				moffs[nt] = (MPI_Offset) (blocks[j].buf);
				nt++;

				j++;
			} else {
				old_nt = nt;
				for (; j <= i; j++) {  // Breakdown each interleaving blocks
					bufp = blocks[j].buf;
					memset (ctr, 0, sizeof (MPI_Offset) * blocks[i].info->ndim);
					while (ctr[0] < blocks[j].count[0]) {  // Foreach row
						if (blocks[i].info->ndim) {
							lens[nt] =
								blocks[j].count[blocks[j].info->ndim - 1] * blocks[j].info->esize;
						} else {
							lens[nt] = blocks[j].info->esize;
						}
						foffs[nt] = blocks[j].info->foff;
						moffs[nt] = (MPI_Offset) (blocks[j].buf);
						for (k = 0; k < (int32_t) (blocks[i].info->ndim);
							 k++) {	 // Calculate offset
							foffs[nt] += blocks[j].info->dsteps[k] * (blocks[j].start[k] + ctr[k]);
						}
						moffs[nt] += (MPI_Aint)bufp;
						bufp += lens[nt];
						ftypes[nt] = MPI_BYTE;
						mtypes[nt] = MPI_BYTE;
						nt++;

						if (blocks[i].info->ndim < 2) break;  // Special case for < 2-D
						ctr[blocks[i].info->ndim - 2]++;	  // Move to next position
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

				// Should there be overlapping write, we discard overlapping part
				for (k = old_nt; k < nt; k++) {
					for (l = k + 1; l < nt; l++) {
						if (foffs[k] + lens[k] > foffs[l]) {  // Adjust for overlap
							// Trim off the later one
							auto osize = std::min ((size_t)lens[l],
												   (size_t) (foffs[k] - foffs[l] + lens[k]));
							foffs[l] += osize;
							moffs[l] += osize;
							lens[l] -= osize;
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

err_out:

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

	return err;
}