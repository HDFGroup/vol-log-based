#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "H5VL_log.h"
#include "H5VL_log_dataset.hpp"
#include "H5VL_log_dataseti.hpp"
#include "H5VL_log_filei.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_idx.hpp"
#include "H5VL_logi_util.hpp"
#include "H5VL_logi_wrapper.hpp"

static bool interleve (int ndim, int *sa, int *ca, int *sb) {
	int i;

	for (i = 0; i < ndim; i++) {
		if (sa[i] + ca[i] < sb[i]) {
			return false;
		} else if (sa[i] + ca[i] > sb[i]) {
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

// Assume no overlaping read
herr_t H5VL_log_dataset_readi_gen_rtypes (std::vector<H5VL_log_idx_search_ret_t> blocks,
										  MPI_Datatype *ftype,
										  MPI_Datatype *mtype,
										  std::vector<H5VL_log_copy_ctx> &overlaps) {
	herr_t err = 0;
	int mpierr;
	int i, j, k, l;
	int nblock = blocks.size ();
	std::vector<bool> newgroup (nblock, 0);
	int nt, nrow, old_nt;
	int *lens;
	MPI_Aint *foffs = NULL, *moffs = NULL;
	MPI_Datatype *ftypes = NULL, *mtypes = NULL, etype = MPI_DATATYPE_NULL;
	MPI_Offset fssize[H5S_MAX_RANK], mssize[H5S_MAX_RANK];
	MPI_Offset ctr[H5S_MAX_RANK];
	H5VL_log_copy_ctx ctx;

	if (!nblock) {
		*ftype = *mtype = MPI_DATATYPE_NULL;
		return 0;
	}

	std::sort (blocks.begin (), blocks.end ());

	for (i = 0; i < nblock - 1; i++) {
		if (blocks[i].zbuf) {
			newgroup[i] = true;
		} else if ((blocks[i].foff == blocks[i + 1].foff) &&
				   interleve (blocks[i].info->ndim,  blocks[i].dstart, blocks[i].count,
							  blocks[i + 1].dstart)) {
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
					for (k = 0; k < blocks[i].info->ndim - 1; k++) { nrow *= blocks[j].count[k]; }
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
						blocks[i].info->ndim, blocks[j].dsize, blocks[j].count, blocks[j].dstart,
						MPI_ORDER_C, etype, ftypes + nt);
					CHECK_MPIERR
					mpierr = H5VL_log_debug_MPI_Type_create_subarray (
						blocks[i].info->ndim, blocks[j].msize, blocks[j].count, blocks[j].mstart,
						MPI_ORDER_C, etype, mtypes + nt);

					CHECK_MPIERR
					mpierr = MPI_Type_commit (ftypes + nt);
					CHECK_MPIERR
					mpierr = MPI_Type_commit (mtypes + nt);
					CHECK_MPIERR

					if (k) {
						mpierr = MPI_Type_free (&etype);
						CHECK_MPIERR
					}

					foffs[nt] = blocks[j].foff;
					moffs[nt] = (MPI_Offset)(blocks[j].xbuf);
					lens[nt]  = 1;
					nt++;
				}

				j++;
			} else {
				old_nt = nt;
				for (; j <= i; j++) {  // Breakdown each interleaving blocks
					fssize[blocks[i].info->ndim - 1] = 1;
					mssize[blocks[i].info->ndim - 1] = 1;
					for (k = blocks[i].info->ndim - 2; k > -1; k--) {
						fssize[k] = fssize[k + 1] * blocks[j].dsize[k + 1];
						mssize[k] = mssize[k + 1] * blocks[j].msize[k + 1];
					}

					memset (ctr, 0, sizeof (MPI_Offset) * blocks[i].info->ndim);
					while (ctr[0] < blocks[j].count[0]) {  // Foreach row
						lens[nt]  = blocks[j].count[blocks[i].info->ndim - 1] * blocks[j].info->esize;
						foffs[nt] = blocks[j].foff;
						moffs[nt] = (MPI_Offset)(blocks[j].xbuf);
						for (k = 0; k < blocks[i].info->ndim; k++) {	// Calculate offset
							foffs[nt] += fssize[k] * (blocks[j].dstart[k] + ctr[k]);
							moffs[nt] += mssize[k] * (blocks[j].mstart[k] + ctr[k]);
						}
						ftypes[nt] = MPI_BYTE;
						mtypes[nt] = MPI_BYTE;
						nt++;

						ctr[blocks[i].info->ndim - 2]++;	// Move to next position
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
							ctx.dst	 = (char *)moffs[l];
							ctx.size = std::min ((size_t)lens[l],
												 (size_t) (foffs[k] - foffs[l] + lens[k]));
							ctx.src	 = (char *)(moffs[k] - ctx.size + lens[k]);
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

	mpierr = MPI_Type_struct (nt, lens, foffs, ftypes, ftype);
	CHECK_MPIERR
	mpierr = MPI_Type_struct (nt, lens, moffs, mtypes, mtype);
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

	H5VL_log_free (foffs) H5VL_log_free (moffs) H5VL_log_free (lens)

		return err;
}

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
void *H5VL_log_dataseti_open_with_uo (void *obj,
									  void *uo,
									  const H5VL_loc_params_t *loc_params,
									  hid_t dxpl_id) {
	herr_t err			= 0;
	H5VL_log_obj_t *op	= (H5VL_log_obj_t *)obj;
	H5VL_log_dset_t *dp = NULL;
	H5VL_loc_params_t locp;
	va_list args;
	void *ap;
	int ndim;
	H5VL_LOGI_PROFILING_TIMER_START;

	dp = new H5VL_log_dset_t (op, H5I_DATASET, uo);
	CHECK_PTR (dp)

	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VL_logi_dataset_get_wrapper (dp->uo, dp->uvlid, H5VL_DATASET_GET_TYPE, dxpl_id, NULL,
										 &(dp->dtype));
	CHECK_ERR
	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VLDATASET_GET);

	dp->esize = H5Tget_size (dp->dtype);
	CHECK_ID (dp->esize)

	// Atts
	err = H5VL_logi_get_att_ex (dp, "_dims", H5T_NATIVE_INT64, &(dp->ndim), dp->dims, dxpl_id);
	CHECK_ERR
	err = H5VL_logi_get_att (dp, "_mdims", H5T_NATIVE_INT64, dp->mdims, dxpl_id);
	CHECK_ERR
	err = H5VL_logi_get_att (dp, "_ID", H5T_NATIVE_INT32, &(dp->id), dxpl_id);
	CHECK_ERR

	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASETI_OPEN_WITH_UO);

	goto fn_exit;
err_out:;
	if (dp) delete dp;
	dp = NULL;
fn_exit:;
	return (void *)dp;
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
	herr_t err			= 0;
	H5VL_log_dset_t *dp = NULL;
	H5VL_loc_params_t locp;
	va_list args;
	void *ap;
	int ndim;
	H5VL_LOGI_PROFILING_TIMER_START;

	dp = new H5VL_log_dset_t (cp, H5I_DATASET, uo);
	CHECK_PTR (dp)

	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VL_logi_dataset_get_wrapper (dp->uo, dp->uvlid, H5VL_DATASET_GET_TYPE,
										 H5P_DATASET_XFER_DEFAULT, NULL, &(dp->dtype));
	CHECK_ERR
	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VLDATASET_GET);
	dp->esize = H5Tget_size (dp->dtype);
	CHECK_ID (dp->esize)

	// Atts
	err = H5VL_logi_get_att_ex (dp, "_dims", H5T_NATIVE_INT64, &(dp->ndim), dp->dims,
								H5P_DATASET_XFER_DEFAULT);
	CHECK_ERR
	err = H5VL_logi_get_att (dp, "_mdims", H5T_NATIVE_INT64, dp->mdims, H5P_DATASET_XFER_DEFAULT);
	CHECK_ERR
	err = H5VL_logi_get_att (dp, "_ID", H5T_NATIVE_INT32, &(dp->id), H5P_DATASET_XFER_DEFAULT);
	CHECK_ERR

	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASETI_WRAP);

	goto fn_exit;
err_out:;
	if (dp) delete dp;
	dp = NULL;
fn_exit:;
	return (void *)dp;
} /* end H5VL_log_dataset_open() */

/*
herr_t H5VL_log_dataseti_writen (hid_t did,
				  hid_t mem_type_id,
				  int n,
				  MPI_Offset **starts,
				  MPI_Offset **counts,
				  hid_t dxplid,
				  void *buf) {
	herr_t err			= 0;
	H5VL_log_dset_t *dp = NULL;
	int i, j, k, l;
	size_t esize;
	H5VL_log_wreq_t r;
	htri_t eqtype;
	H5VL_log_req_type_t rtype;
	MPI_Datatype ptype = MPI_DATATYPE_NULL;
	H5VL_LOGI_PROFILING_TIMER_START;

	H5VL_LOGI_PROFILING_TIMER_START;

	// Get VOL object
	dp = (H5VL_log_dset_t *)H5VLobject (did);
	CHECK_PTR (dp)

	// Sanity check
	if (!buf) ERR_OUT ("user buffer can't be NULL");

	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE_INIT);

	H5VL_LOGI_PROFILING_TIMER_START;
	// Setting metadata;
	r.hdr.did = dp->id;
	// r.info->ndim	= dp->ndim;
	r.ldid	= -1;
	r.ldoff = 0;
	r.ubuf	= (char *)buf;
	r.rsize = 0;  // Nomber of elements in record
	r.hdr.flag	= 0;

	// Gather starts and counts
	r.sels.resize (n);
	for (i = 0; i < n; i++) {
		r.sels[i].size = 1;
		for (j = 0; j < dp->ndim; j++) {
			r.sels[i].start[j] = starts[i][j];
			r.sels[i].count[j] = counts[i][j];
			r.sels[i].size *= r.sels[i].count[j];
		}
		r.rsize += r.sels[i].size;
		r.sels[i].size *= dp->esize;
	}
	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE_START_COUNT);

	// Non-blocking?
	err = H5Pget_nonblocking (dxplid, &rtype);
	CHECK_ERR

	// Need convert?
	eqtype = H5Tequal (dp->dtype, mem_type_id);

	// Can reuse user buffer
	if (rtype == H5VL_LOG_REQ_NONBLOCKING && eqtype > 0) {
		r.xbuf = r.ubuf;
	} else {  // Need internal buffer
		H5VL_LOGI_PROFILING_TIMER_START;
		// Get element size
		esize = H5Tget_size (mem_type_id);
		CHECK_ID (esize)

		// HDF5 type conversion is in place, allocate for whatever larger
		err = H5VL_log_filei_balloc (dp->fp, r.rsize * std::max (esize, (size_t) (dp->esize)),
									 (void **)(&(r.xbuf)));
		// err = H5VL_log_filei_pool_alloc (&(dp->fp->data_buf),
		//								 r.rsize * std::max (esize, (size_t) (dp->esize)),
		//								 (void **)(&(r.xbuf)));
		// CHECK_ERR

		// Copy data
		memcpy (r.xbuf, r.ubuf, r.rsize * esize);

		H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE_PACK);

		H5VL_LOGI_PROFILING_TIMER_START;
		// Need convert
		if (eqtype == 0) {
			void *bg = NULL;

			if (H5Tget_class (dp->dtype) == H5T_COMPOUND) bg = malloc (r.rsize * dp->esize);
			err = H5Tconvert (mem_type_id, dp->dtype, r.rsize, r.xbuf, bg, dxplid);
			free (bg);
		}
		H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE_CONVERT);
	}

	H5VL_LOGI_PROFILING_TIMER_START;
	// Convert request size to number of bytes to be used by later routines
	r.rsize *= dp->esize;

	// Put request in queue
	dp->fp->wreqs.push_back (r);
	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE_FINALIZE);

	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE);
err_out:;
	if (err) {
		// if (r.xbuf != r.ubuf) H5VL_log_filei_bfree (dp->fp, r.xbuf);
	}
	H5VL_log_type_free (ptype);

	return err;
}
*/