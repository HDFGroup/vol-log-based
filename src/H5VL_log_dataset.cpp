#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <assert.h>

#include "H5VL_log.h"
#include "H5VL_log_dataset.hpp"
#include "H5VL_log_filei.hpp"
#include "H5VL_log_req.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_dataspace.hpp"
#include "H5VL_logi_filter.hpp"
#include "H5VL_logi_util.hpp"
#include "H5VL_logi_wrapper.hpp"
#include "H5VL_logi_zip.hpp"

/********************* */
/* Function prototypes */
/********************* */

const H5VL_dataset_class_t H5VL_log_dataset_g {
	H5VL_log_dataset_create,   /* create       */
	H5VL_log_dataset_open,	   /* open         */
	H5VL_log_dataset_read,	   /* read         */
	H5VL_log_dataset_write,	   /* write        */
	H5VL_log_dataset_get,	   /* get          */
	H5VL_log_dataset_specific, /* specific     */
	H5VL_log_dataset_optional, /* optional     */
	H5VL_log_dataset_close	   /* close        */
};

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_create
 *
 * Purpose:     Creates a dataset in a container
 *
 * Return:      Success:    Pointer to a dataset object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_log_dataset_create (void *obj,
							   const H5VL_loc_params_t *loc_params,
							   const char *name,
							   hid_t lcpl_id,
							   hid_t type_id,
							   hid_t space_id,
							   hid_t dcpl_id,
							   hid_t dapl_id,
							   hid_t dxpl_id,
							   void **req) {
	herr_t err = 0;
	int i;
	H5VL_log_obj_t *op	= (H5VL_log_obj_t *)obj;
	H5VL_log_dset_t *dp = NULL;
	H5VL_loc_params_t locp;
	hid_t sid = -1;
	void *ap;
	int ndim, nfilter;
	H5VL_log_req_t *rp;
	void **ureqp, *ureq;
	H5VL_LOGI_PROFILING_TIMER_START;

	sid = H5Screate (H5S_SCALAR);
	CHECK_ID (sid);
	err = H5Pset_layout (dcpl_id, H5D_CONTIGUOUS);
	CHECK_ERR

	dp = new H5VL_log_dset_t (op, H5I_DATASET);

	if (req) {
		rp	  = new H5VL_log_req_t ();
		ureqp = &ureq;
	} else {
		ureqp = NULL;
	}

	H5VL_LOGI_PROFILING_TIMER_START;
	dp->uo = H5VLdataset_create (op->uo, loc_params, op->uvlid, name, lcpl_id, type_id, sid,
								 dcpl_id, dapl_id, dxpl_id, ureqp);
	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VLDATASET_CREATE);
	CHECK_PTR (dp->uo)
	if (req) { rp->append (ureq); }

	dp->dtype = H5Tcopy (type_id);
	CHECK_ID (dp->dtype)
	dp->esize = H5Tget_size (type_id);
	CHECK_ID (dp->esize)

	// NOTE: I don't know if it work for scalar dataset, can we create zero sized attr?
	ndim = H5Sget_simple_extent_dims (space_id, dp->dims, dp->mdims);
	CHECK_ID (ndim)
	dp->ndim = (hsize_t)ndim;

	dp->id = (dp->fp->ndset)++;

	// Record metadata in fp
	dp->fp->idx.resize (dp->fp->ndset);
	dp->fp->mreqs.resize (dp->fp->ndset);
	dp->fp->mreqs[dp->id] = new H5VL_log_merged_wreq_t (dp, 1);
	dp->fp->dsets.resize (dp->fp->ndset);
	dp->fp->dsets[dp->id] = *dp;
	// Dstep for encoding selection
	if (dp->fp->config & H5VL_FILEI_CONFIG_SEL_ENCODE) {
		dp->dsteps[dp->ndim - 1] = 1;
		for (i = dp->ndim - 2; i > -1; i--) { dp->dsteps[i] = dp->dsteps[i + 1] * dp->dims[i + 1]; }
	}

	// Atts
	err = H5VL_logi_add_att (dp, "_dims", H5T_STD_I64LE, H5T_NATIVE_INT64, dp->ndim, dp->dims,
							 dxpl_id, ureqp);
	CHECK_ERR
	if (req) { rp->append (ureq); }
	err = H5VL_logi_add_att (dp, "_mdims", H5T_STD_I64LE, H5T_NATIVE_INT64, dp->ndim, dp->mdims,
							 dxpl_id, ureqp);
	CHECK_ERR
	if (req) { rp->append (ureq); }

	err = H5VL_logi_add_att (dp, "_ID", H5T_STD_I32LE, H5T_NATIVE_INT32, 1, &(dp->id), dxpl_id,
							 ureqp);
	CHECK_ERR
	if (req) {
		rp->append (ureq);
		*req = rp;
	}

	// Filters
	nfilter = H5Pget_nfilters (dcpl_id);
	CHECK_ID (nfilter);
	dp->filters.resize (nfilter);
	for (i = 0; i < nfilter; i++) {
		dp->filters[i].id = H5Pget_filter2 (dcpl_id, (unsigned int)i, &(dp->filters[i].flags),
											&(dp->filters[i].cd_nelmts), dp->filters[i].cd_values,
											LOGVOL_FILTER_NAME_MAX, dp->filters[i].name,
											&(dp->filters[i].filter_config));
		CHECK_ID (dp->filters[i].id);
	}

	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_CREATE);

	goto fn_exit;
err_out:;
	if (dp) {
		delete dp;
		dp = NULL;
	}
fn_exit:;
	return (void *)dp;
} /* end H5VL_log_dataset_create() */

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
void *H5VL_log_dataset_open (void *obj,
							 const H5VL_loc_params_t *loc_params,
							 const char *name,
							 hid_t dapl_id,
							 hid_t dxpl_id,
							 void **req) {
	herr_t err = 0;
	int i;
	H5VL_log_obj_t *op	= (H5VL_log_obj_t *)obj;
	H5VL_log_dset_t *dp = NULL;
	H5VL_loc_params_t locp;
	va_list args;
	int nfilter;
	hid_t dcpl_id = -1;
	void *ap;
	H5VL_log_req_t *rp;
	void **ureqp, *ureq;
	H5VL_LOGI_PROFILING_TIMER_START;

	dp = new H5VL_log_dset_t (op, H5I_DATASET);

	if (req) {
		rp	  = new H5VL_log_req_t ();
		ureqp = &ureq;
	} else {
		ureqp = NULL;
	}

	H5VL_LOGI_PROFILING_TIMER_START;
	dp->uo = H5VLdataset_open (op->uo, loc_params, op->uvlid, name, dapl_id, dxpl_id, NULL);
	CHECK_PTR (dp->uo);
	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VLDATASET_OPEN);

	dp->dtype = H5VL_logi_dataset_get_type (dp->uo, dp->uvlid, dxpl_id);
	CHECK_ID (dp->dtype)
	if (req) { rp->append (ureq); }

	dp->esize = H5Tget_size (dp->dtype);
	CHECK_ID (dp->esize)

	// Atts
	err = H5VL_logi_get_att_ex (dp, "_dims", H5T_NATIVE_INT64, &(dp->ndim), dp->dims, dxpl_id);
	CHECK_ERR
	if (req) { rp->append (ureq); }
	err = H5VL_logi_get_att (dp, "_mdims", H5T_NATIVE_INT64, dp->mdims, dxpl_id);
	CHECK_ERR
	if (req) { rp->append (ureq); }
	err = H5VL_logi_get_att (dp, "_ID", H5T_NATIVE_INT32, &(dp->id), dxpl_id);
	CHECK_ERR
	if (req) {
		rp->append (ureq);
		*req = rp;
	}

	// Dstep for encoding selection
	if (dp->fp->config & H5VL_FILEI_CONFIG_SEL_ENCODE) {
		dp->dsteps[dp->ndim - 1] = 1;
		for (i = dp->ndim - 2; i > -1; i--) { dp->dsteps[i] = dp->dsteps[i + 1] * dp->dims[i + 1]; }
	}

	// Record metadata in fp
	dp->fp->idx.resize (dp->fp->ndset);
	dp->fp->dsets.resize (dp->fp->ndset);
	dp->fp->mreqs[dp->id] = new H5VL_log_merged_wreq_t (dp, 1);
	dp->fp->dsets[dp->id] = *dp;

	// Filters
	dcpl_id = H5VL_logi_dataset_get_dcpl (dp->uo, dp->uvlid, dxpl_id);
	CHECK_ID (dcpl_id)
	nfilter = H5Pget_nfilters (dcpl_id);
	CHECK_ID (nfilter);
	dp->filters.resize (nfilter);
	for (i = 0; i < nfilter; i++) {
		dp->filters[i].id = H5Pget_filter2 (dcpl_id, (unsigned int)i, &(dp->filters[i].flags),
											&(dp->filters[i].cd_nelmts), dp->filters[i].cd_values,
											LOGVOL_FILTER_NAME_MAX, dp->filters[i].name,
											&(dp->filters[i].filter_config));
		CHECK_ID (dp->filters[i].id);
	}

	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_OPEN);

	goto fn_exit;
err_out:;
	if (dp) delete dp;
	dp = NULL;
fn_exit:;
	if (dcpl_id >= 0) { H5Pclose (dcpl_id); }
	return (void *)dp;
} /* end H5VL_log_dataset_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_read
 *
 * Purpose:     Reads data elements from a dataset into a buffer.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_dataset_read (void *dset,
							  hid_t mem_type_id,
							  hid_t mem_space_id,
							  hid_t file_space_id,
							  hid_t plist_id,
							  void *buf,
							  void **req) {
	herr_t err = 0;
	int i, j;
	int n;
	size_t esize;
	htri_t eqtype;
	char *bufp = (char *)buf;
	H5VL_log_rreq_t r;
	H5S_sel_type stype, mstype;
	H5VL_log_req_type_t rtype;
	H5VL_log_multisel_arg_t arg;
	H5VL_log_dset_t *dp = (H5VL_log_dset_t *)dset;
	H5VL_log_req_t *rp;
	void **ureqp, *ureq;
	H5VL_LOGI_PROFILING_TIMER_START;

	H5VL_LOGI_PROFILING_TIMER_START;

	// Sanity check
	if (stype == H5S_SEL_NONE) goto err_out;
	if (!buf) ERR_OUT ("user buffer can't be NULL");
	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_READ_INIT);

	// Varn ?
	err = H5Pget_multisel (plist_id, &arg);
	CHECK_ERR

	// Gather starts and counts
	if (arg.n) {
		r.sels = new H5VL_log_selections (dp->ndim, arg.n, arg.starts, arg.counts);

		mstype = H5S_SEL_ALL;
	} else {
		H5VL_LOGI_PROFILING_TIMER_START;
		r.sels = new H5VL_log_selections (file_space_id);
		H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOGI_GET_DATASPACE_SELECTION);

		// H5S_All means using file space
		if (mem_space_id == H5S_ALL) mem_space_id = file_space_id;

		// Check mem space selection
		if (mem_space_id == H5S_ALL)
			mstype = H5S_SEL_ALL;
		else if (mem_space_id == H5S_CONTIG)
			mstype = H5S_SEL_ALL;
		else
			mstype = H5Sget_select_type (mem_space_id);
	}

	// Setting metadata;
	r.info	  = &(dp->fp->dsets[dp->id]);
	r.hdr.did = dp->id;
	r.ndim	  = dp->ndim;
	r.ubuf	  = (char *)buf;
	r.ptype	  = MPI_DATATYPE_NULL;
	r.dtype	  = -1;
	r.mtype	  = -1;
	r.esize	  = dp->esize;
	r.rsize	  = 0;	// Nomber of elements in record

	// Non-blocking?
	err = H5Pget_nonblocking (plist_id, &rtype);
	CHECK_ERR

	// Need convert?
	eqtype = H5Tequal (dp->dtype, mem_type_id);
	CHECK_ID (eqtype);

	// Can reuse user buffer
	if (eqtype > 0 && mstype == H5S_SEL_ALL) {
		r.xbuf = r.ubuf;
	} else {  // Need internal buffer
		// Get element size
		esize = H5Tget_size (mem_type_id);
		CHECK_ID (esize)

		// HDF5 type conversion is in place, allocate for whatever larger
		err = H5VL_log_filei_balloc (dp->fp, r.rsize * std::max (esize, (size_t) (dp->esize)),
									 (void **)(&(r.xbuf)));
		CHECK_ERR

		// Need packing
		if (mstype != H5S_SEL_ALL) {
			H5VL_LOGI_PROFILING_TIMER_START;
			err = H5VL_log_selections (mem_space_id).get_mpi_type (esize, &(r.ptype));
			CHECK_ERR
			H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOGI_GET_DATASPACE_SEL_TYPE);
		}

		// Need convert
		if (eqtype == 0) {
			r.dtype = H5Tcopy (dp->dtype);
			CHECK_ID (r.dtype)
			r.mtype = H5Tcopy (mem_type_id);
			CHECK_ID (r.mtype)
		}
	}

	// Flush it immediately if blocking, otherwise place into queue
	if (rtype != H5VL_LOG_REQ_NONBLOCKING) {
		err = H5VL_log_nb_flush_read_reqs (dp->fp, std::vector<H5VL_log_rreq_t> (1, r), plist_id);
		CHECK_ERR
	} else {
		dp->fp->rreqs.push_back (r);
	}

	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_READ);
err_out:;

	return err;
} /* end H5VL_log_dataset_read() */

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
herr_t H5VL_log_dataset_write (void *dset,
							   hid_t mem_type_id,
							   hid_t mem_space_id,
							   hid_t file_space_id,
							   hid_t plist_id,
							   const void *buf,
							   void **req) {
	herr_t err			= 0;
	H5VL_log_dset_t *dp = (H5VL_log_dset_t *)dset;
	int i;
	size_t esize;							 // Element size of the memory type
	size_t ssize;							 // Size of a selection block
	H5VL_log_wreq_t *r;						 // Request obj
	H5VL_log_req_data_block_t db;			 // Request data
	htri_t eqtype;							 // user buffer type equals dataset type?
	H5S_sel_type stype;						 // Dataset sselection type
	H5S_sel_type mstype;					 // Memory space selection type
	H5VL_log_req_type_t rtype;				 // Whether req is nonblocking
	MPI_Datatype ptype = MPI_DATATYPE_NULL;	 // Packing type for non-contiguous memory buffer
	H5VL_log_multisel_arg_t arg;			 // Varn parameters
	H5VL_log_req_t *rp;						 // Request obj
	H5VL_log_selections *dsel = NULL;		 // Selection blocks
	H5VL_LOGI_PROFILING_TIMER_START;

	H5VL_LOGI_PROFILING_TIMER_START;

	// Varn ?
	err = H5Pget_multisel (plist_id, &arg);
	CHECK_ERR

	// Get selections
	if (arg.n) {
		dsel = new H5VL_log_selections (dp->ndim, arg.n, arg.starts, arg.counts);

		mstype = H5S_SEL_ALL;
	} else {
		H5VL_LOGI_PROFILING_TIMER_START;
		dsel = new H5VL_log_selections (file_space_id);
		H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOGI_GET_DATASPACE_SELECTION);

		// H5S_All means using file space
		if (mem_space_id == H5S_ALL) mem_space_id = file_space_id;

		// Check mem space selection
		if (mem_space_id == H5S_ALL)
			mstype = H5S_SEL_ALL;
		else if (mem_space_id == H5S_CONTIG)
			mstype = H5S_SEL_ALL;
		else
			mstype = H5Sget_select_type (mem_space_id);
	}

	// Sanity check
	if (dsel->nsel == 0) goto err_out;	// No elements selected
	if (!buf) ERR_OUT ("user buffer can't be NULL");
	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE_INIT);

	if (dp->fp->flag ^ H5VL_FILEI_CONFIG_METADATA_MERGE) {
		H5VL_LOGI_PROFILING_TIMER_START;

		r = new H5VL_log_wreq_t ();

		// Setting metadata;
		r->hdr.did = dp->id;
		db.ubuf	   = (char *)buf;
		r->nsel	   = dsel->nsel;
		db.size	   = dsel->get_sel_size ();	 // Number of data elements in the record

		H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE_START_COUNT);

		// Encode and pack selections
		H5VL_LOGI_PROFILING_TIMER_START

		// Flags
		r->hdr.flag = 0;
		if (r->nsel > 1) {
			if ((dp->ndim > 1) && (dp->fp->config & H5VL_FILEI_CONFIG_SEL_ENCODE)) {
				r->hdr.flag |= H5VL_LOGI_META_FLAG_SEL_ENCODE;
			}
			r->hdr.flag |= H5VL_LOGI_META_FLAG_MUL_SEL;

			if (dp->fp->config & H5VL_FILEI_CONFIG_SEL_DEFLATE) {
				r->hdr.flag |= H5VL_LOGI_META_FLAG_SEL_DEFLATE;
			}
		}

		// Selection buffer
		r->hdr.meta_size = sizeof (H5VL_logi_meta_hdr);	 // Header
		if (r->hdr.flag & H5VL_LOGI_META_FLAG_MUL_SEL) {
			r->hdr.meta_size += sizeof (int);  // N
		}
		if (r->hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
			r->hdr.meta_size +=
				sizeof (MPI_Offset) * (dp->ndim - 1 + r->nsel * 2) + sizeof (MPI_Offset) * 2;
		} else {
			r->hdr.meta_size +=
				sizeof (MPI_Offset) * (dp->ndim * r->nsel * 2) + sizeof (MPI_Offset) * 2;
		}
		r->meta_buf = (char *)malloc (r->hdr.meta_size);

		err = H5VL_logi_metaentry_encode (*dp, r->hdr, dsel->nsel, dsel->starts, dsel->counts,
										  r->meta_buf);
		CHECK_ERR

		H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE_ENCODE);
	}

	// Non-blocking?
	err = H5Pget_nonblocking (plist_id, &rtype);
	CHECK_ERR

	// Need convert?
	eqtype = H5Tequal (dp->dtype, mem_type_id);

	// Can reuse user buffer
	if (rtype == H5VL_LOG_REQ_NONBLOCKING && eqtype > 0 && mstype == H5S_SEL_ALL) {
		db.xbuf = db.ubuf;
	} else {  // Need internal buffer
		H5VL_LOGI_PROFILING_TIMER_START;
		// Get element size
		esize = H5Tget_size (mem_type_id);
		CHECK_ID (esize)

		// HDF5 type conversion is in place, allocate for whatever larger
		err = H5VL_log_filei_balloc (dp->fp, db.size * std::max (esize, (size_t) (dp->esize)),
									 (void **)(&(db.xbuf)));
		// err = H5VL_log_filei_pool_alloc (&(dp->fp->data_buf),
		//								 db.size * std::max (esize, (size_t) (dp->esize)),
		//								 (void **)(&(r->xbuf)));
		// CHECK_ERR

		// Need packing
		if (mstype != H5S_SEL_ALL) {
			i = 0;

			H5VL_LOGI_PROFILING_TIMER_START
			err = H5VL_log_selections (mem_space_id).get_mpi_type (esize, &ptype);
			CHECK_ERR
			H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOGI_GET_DATASPACE_SEL_TYPE);

			MPI_Pack (db.ubuf, 1, ptype, db.xbuf, db.size * esize, &i, dp->fp->comm);

			LOG_VOL_ASSERT (i == db.size * esize)
		} else {
			memcpy (db.xbuf, db.ubuf, db.size * esize);
		}
		H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE_PACK);

		H5VL_LOGI_PROFILING_TIMER_START;
		// Need convert
		if (eqtype == 0) {
			void *bg = NULL;

			if (H5Tget_class (dp->dtype) == H5T_COMPOUND) bg = malloc (db.size * dp->esize);

			err = H5Tconvert (mem_type_id, dp->dtype, db.size, db.xbuf, bg, plist_id);
			free (bg);
		}
		H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE_CONVERT);
	}

	// Convert request size to number of bytes to be used by later routines
	db.size *= dp->esize;

	// Filtering
	H5VL_LOGI_PROFILING_TIMER_START;
	if (dp->filters.size ()) {
		char *buf = NULL;
		int csize = 0;

		err = H5VL_logi_filter (dp->filters, db.xbuf, db.size, (void **)&buf, &csize);
		CHECK_ERR

		if (db.xbuf != db.ubuf) {
			err = H5VL_log_filei_bfree (dp->fp, &(db.xbuf));
			CHECK_ERR
		}

		db.xbuf = buf;
		db.size = csize;
	}
	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE_FILTER);

	H5VL_LOGI_PROFILING_TIMER_START;

	// Put request in queue
	if (dp->fp->flag ^ H5VL_FILEI_CONFIG_METADATA_MERGE) {
		err = dp->fp->mreqs[dp->id]->append (dp, db, dsel->nsel, dsel->starts, dsel->counts);
		CHECK_ERR
	} else {
		r->rsize = db.size;
		r->dbufs.push_back (db);
		dp->fp->wreqs.push_back (r);
	}
	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE_FINALIZE);

	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_WRITE);
err_out:;
	if (err) {
		// if (r->xbuf != r->ubuf) H5VL_log_filei_bfree (dp->fp, r->xbuf);
	}
	if (dsel) { delete dsel; }
	H5VL_log_type_free (ptype);

	return err;
} /* end H5VL_log_dataset_write() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_get
 *
 * Purpose:     Gets information about a dataset
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_dataset_get (void *dset, H5VL_dataset_get_args_t *args, hid_t dxpl_id, void **req) {
	H5VL_log_dset_t *dp = (H5VL_log_dset_t *)dset;
	herr_t err			= 0;
	H5VL_log_req_t *rp;
	void **ureqp, *ureq;
	H5VL_LOGI_PROFILING_TIMER_START;

	switch (args->op_type) {
		/* H5Dget_space */
		case H5VL_DATASET_GET_SPACE: {
			args->args.get_space.space_id = H5Screate_simple (dp->ndim, dp->dims, dp->mdims);
			break;
		}

		/* H5Dget_space_status */
		case H5VL_DATASET_GET_SPACE_STATUS: {
			err = -1;
			ERR_OUT ("H5VL_DATASET_GET_SPACE_STATUS not supported")
			break;
		}

		/* H5Dargs->get_type */
		case H5VL_DATASET_GET_TYPE: {
			args->args.get_type.type_id = H5Tcopy (dp->dtype);
			break;
		}

		/* H5Dget_create_plist */
		case H5VL_DATASET_GET_DCPL: {
			err = -1;
			ERR_OUT ("H5VL_DATASET_GET_DCPL not supported")
			break;
		}

		/* H5Dget_access_plist */
		case H5VL_DATASET_GET_DAPL: {
			err = -1;
			ERR_OUT ("H5VL_DATASET_GET_DAPL not supported")
			break;
		}

		/* H5Dget_storage_size */
		case H5VL_DATASET_GET_STORAGE_SIZE: {
			err = -1;
			ERR_OUT ("H5VL_DATASET_GET_STORAGE_SIZE not supported")
			break;
		}
		default:
			ERR_OUT ("Unrecognized op type")
	} /* end switch */

	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_GET);
err_out:;
	return err;
} /* end H5VL_log_dataset_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_specific
 *
 * Purpose:     Specific operation on a dataset
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_dataset_specific (void *obj,
								  H5VL_dataset_specific_args_t *args,
								  hid_t dxpl_id,
								  void **req) {
	H5VL_log_dset_t *dp = (H5VL_log_dset_t *)obj;
	herr_t err			= 0;
	H5VL_log_req_t *rp;
	void **ureqp, *ureq;
	H5VL_LOGI_PROFILING_TIMER_START;

	switch (args->op_type) {
		case H5VL_DATASET_SET_EXTENT: { /* H5Dset_extent */
			int i;
			const hsize_t *new_sizes = args->args.set_extent.size;

			// Adjust dim
			for (i = 0; i < dp->ndim; i++) {
				if (new_sizes[i] < 0) {
					err = -1;
					ERR_OUT ("size cannot be negative")
				}
				if (dp->mdims[i] != H5S_UNLIMITED && new_sizes[i] > dp->mdims[i]) {
					err = -1;
					ERR_OUT ("size cannot exceed max size")
				}
				dp->dims[i] = dp->fp->dsets[dp->id].dims[i] = new_sizes[i];
			}

			// Recalculate dsteps if needed
			if (dp->fp->config & H5VL_FILEI_CONFIG_SEL_ENCODE) {
				dp->dsteps[dp->ndim - 1] = 1;
				for (i = dp->ndim - 2; i > -1; i--) {
					dp->dsteps[i] = dp->dsteps[i + 1] * dp->dims[i + 1];
				}

				// Flush merged request as dstep may be changed
				if (dp->fp->mreqs[dp->id] && (dp->fp->mreqs[dp->id]->nsel > 0)) {
					dp->fp->wreqs.push_back (dp->fp->mreqs[dp->id]);
					dp->fp->mreqs[dp->id] = new H5VL_log_merged_wreq_t (dp, 1);
				}
			}

			break;
		}
		default:
			H5VL_LOGI_PROFILING_TIMER_START;
			err = H5VLdataset_specific (dp->uo, dp->uvlid, args, dxpl_id, req);
			H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VLDATASET_SPECIFIC);
	}

	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_SPECIFIC);
err_out:;
	return err;
} /* end H5VL_log_dataset_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_optional
 *
 * Purpose:     Perform a connector-specific operation on a dataset
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_dataset_optional (void *obj,
								  H5VL_optional_args_t *args,
								  hid_t dxpl_id,
								  void **req) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
	herr_t err		   = 0;
	H5VL_log_req_t *rp;
	void **ureqp, *ureq;
	H5VL_LOGI_PROFILING_TIMER_START;

	if (req) {
		rp	  = new H5VL_log_req_t ();
		ureqp = &ureq;
	} else {
		ureqp = NULL;
	}

	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VLdataset_optional (op->uo, op->uvlid, args, dxpl_id, req);
	H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VLDATASET_OPTIONAL);

	if (req) {
		rp->append (ureq);
		*req = rp;
	}

	H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VL_LOG_DATASET_OPTIONAL);
	return err;
} /* end H5VL_log_dataset_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_close
 *
 * Purpose:     Closes a dataset.
 *
 * Return:      Success:    0
 *              Failure:    -1, dataset not closed.
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_dataset_close (void *dset, hid_t dxpl_id, void **req) {
	herr_t err			= 0;
	H5VL_log_dset_t *dp = (H5VL_log_dset_t *)dset;
	H5VL_LOGI_PROFILING_TIMER_START;

	// Flush merged request
	if (dp->fp->mreqs[dp->id] && (dp->fp->mreqs[dp->id]->nsel > 0)) {
		dp->fp->wreqs.push_back (dp->fp->mreqs[dp->id]);
	}

	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VLdataset_close (dp->uo, dp->uvlid, dxpl_id, NULL);
	CHECK_ERR
	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VLDATASET_CLOSE);

	H5Tclose (dp->dtype);

	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_CLOSE);

	delete dp;

err_out:;

	return err;
} /* end H5VL_log_dataset_close() */