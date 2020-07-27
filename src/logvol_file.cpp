#include "logvol_internal.hpp"

/********************* */
/* Function prototypes */
/********************* */
void *H5VL_log_file_create (
	const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void **req);
void *H5VL_log_file_open (
	const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req);
herr_t H5VL_log_file_get (
	void *file, H5VL_file_get_t get_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_log_file_specific (
	void *file, H5VL_file_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_log_file_optional (
	void *file, H5VL_file_optional_t opt_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_log_file_close (void *file, hid_t dxpl_id, void **req);

const H5VL_file_class_t H5VL_log_file_g {
	H5VL_log_file_create,	/* create */
	H5VL_log_file_open,		/* open */
	H5VL_log_file_get,		/* get */
	H5VL_log_file_specific, /* specific */
	H5VL_log_file_optional, /* optional */
	H5VL_log_file_close		/* close */
};

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_file_create
 *
 * Purpose:     Creates a container using this connector
 *
 * Return:      Success:    Pointer to a file object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_log_file_create (
	const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void **req) {
	herr_t err = 0;
	int mpierr;
	H5VL_log_info_t *info = NULL;
	H5VL_log_file_t *fp	  = NULL;
	H5VL_loc_params_t loc;
	hid_t uvlid, under_fapl_id;
	void *under_vol_info;
	MPI_Comm comm;
	hbool_t po_supported;
	TIMER_START;

#ifdef LOGVOL_VERBOSE_DEBUG
	{
		char vname[3][128];
		ssize_t nsize;

		nsize = H5Iget_name (fcpl_id, vname[0], 128);
		if (nsize == 0) {
			sprintf (vname[0], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[0], "Unknown_Object");
		}
		nsize = H5Iget_name (fapl_id, vname[1], 128);
		if (nsize == 0) {
			sprintf (vname[1], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[1], "Unknown_Object");
		}
		nsize = H5Iget_name (dxpl_id, vname[2], 128);
		if (nsize == 0) {
			sprintf (vname[2], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[2], "Unknown_Object");
		}

		printf ("H5VL_log_file_create(%s, %u, %s, %s, %s, %p)\n", name, flags, vname[0], vname[1],
				vname[2], req);
	}
#endif

	// Try get info about under VOL
	H5Pget_vol_info (fapl_id, (void **)&info);

	if (info) {
		uvlid		   = info->uvlid;
		under_vol_info = info->under_vol_info;
	} else {  // If no under VOL specified, use the native VOL
		htri_t ret;
		ret = H5VLis_connector_registered_by_name ("native");
		if (ret != 1) { RET_ERR ("Native VOL not found") }
		uvlid = H5VLpeek_connector_id_by_name ("native");
		CHECK_ID (uvlid)
		under_vol_info = NULL;
	}

	// Make sure we have mpi enabled
	err = H5Pget_fapl_mpio (fapl_id, &comm, NULL);
	CHECK_ERR

	// Init file obj
	fp			= new H5VL_log_file_t ();
	fp->closing = false;
	fp->refcnt	= 0;
	fp->flag	= flags;
	fp->nldset	= 0;
	fp->ndset	= 0;
	MPI_Comm_dup (comm, &(fp->comm));
	MPI_Comm_rank (comm, &(fp->rank));
	fp->uvlid = uvlid;
	H5Iinc_ref (fp->uvlid);
	fp->nflushed  = 0;
	fp->dxplid	  = dxpl_id;
	fp->type	  = H5I_FILE;
	fp->idxvalid  = false;
	fp->metadirty = false;
	fp->fp		  = fp;
	err			  = H5Pget_nb_buffer_size (fapl_id, &(fp->bsize));
	CHECK_ERR
	err=H5VL_log_filei_pool_init(&(fp->data_buf),fp->bsize);
	CHECK_ERR
	err=H5VL_log_filei_contig_buffer_init(&(fp->meta_buf), 2097152);	// 200 MiB
	CHECK_ERR
#ifdef LOGVOL_PROFILING
	H5VL_log_profile_reset (fp);
#endif

	// Create the file with underlying VOL
	under_fapl_id = H5Pcopy (fapl_id);
	H5Pset_vol (under_fapl_id, uvlid, under_vol_info);
	TIMER_START;
	fp->uo = H5VLfile_create (name, flags, fcpl_id, under_fapl_id, dxpl_id, NULL);
	CHECK_NERR (fp->uo)
	TIMER_STOP (fp, TIMER_H5VL_FILE_CREATE);
	H5Pclose (under_fapl_id);

	// Create LOG group
	loc.obj_type = H5I_FILE;
	loc.type	 = H5VL_OBJECT_BY_SELF;
	fp->lgp = H5VLgroup_create (fp->uo, &loc, fp->uvlid, LOG_GROUP_NAME, H5P_LINK_CREATE_DEFAULT,
								H5P_GROUP_CREATE_DEFAULT, H5P_DEFAULT, dxpl_id, NULL);
	CHECK_NERR (fp->lgp)

	// Att
	err =
		H5VL_logi_add_att (fp, "_ndset", H5T_STD_I32LE, H5T_NATIVE_INT32, 1, &(fp->ndset), dxpl_id);
	CHECK_ERR
	err = H5VL_logi_add_att (fp, "_nldset", H5T_STD_I32LE, H5T_NATIVE_INT32, 1, &(fp->nldset),
							 dxpl_id);
	CHECK_ERR

	// Open the file with MPI
	mpierr = MPI_File_open (fp->comm, name, MPI_MODE_RDWR, MPI_INFO_NULL, &(fp->fh));
	CHECK_MPIERR

	TIMER_STOP (fp, TIMER_FILE_CREATE);

	goto fn_exit;
err_out:;
	if (fp) { delete fp; }
	fp = NULL;
fn_exit:;
	MPI_Comm_free (&comm);

	if (info) { free (info); }

	return (void *)fp;
} /* end H5VL_log_file_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_file_open
 *
 * Purpose:     Opens a container created with this connector
 *
 * Return:      Success:    Pointer to a file object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_log_file_open (
	const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req) {
	herr_t err = 0;
	int mpierr;
	H5VL_log_info_t *info = NULL;
	H5VL_log_file_t *fp	  = NULL;
	H5VL_loc_params_t loc;
	hid_t uvlid, under_fapl_id;
	void *under_vol_info;
	MPI_Comm comm;
	TIMER_START;

#ifdef LOGVOL_VERBOSE_DEBUG
	{
		char vname[3][128];
		ssize_t nsize;

		nsize = H5Iget_name (fapl_id, vname[1], 128);
		if (nsize == 0) {
			sprintf (vname[1], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[1], "Unknown_Object");
		}
		nsize = H5Iget_name (dxpl_id, vname[2], 128);
		if (nsize == 0) {
			sprintf (vname[2], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[2], "Unknown_Object");
		}

		printf ("H5VL_log_file_open(%s, %u, %s, %s, %p)\n", name, flags, vname[0], vname[1], req);
	}
#endif

	// Try get info about under VOL
	H5Pget_vol_info (fapl_id, (void **)&info);

	if (info) {
		uvlid		   = info->uvlid;
		under_vol_info = info->under_vol_info;
	} else {  // If no under VOL specified, use the native VOL
		htri_t ret;
		ret = H5VLis_connector_registered_by_name ("native");
		if (ret != 1) { RET_ERR ("Native VOL not found") }
		uvlid = H5VLpeek_connector_id_by_name ("native");
		CHECK_ID (uvlid)
		under_vol_info = NULL;
		// return NULL;
	}

	// Make sure we have mpi enabled
	err = H5Pget_fapl_mpio (fapl_id, &comm, NULL);
	CHECK_ERR

	// Init file obj
	fp			= new H5VL_log_file_t ();
	fp->closing = false;
	fp->refcnt	= 0;
	fp->flag	= flags;
	MPI_Comm_dup (comm, &(fp->comm));
	MPI_Comm_rank (comm, &(fp->rank));
	fp->uvlid = uvlid;
	H5Iinc_ref (fp->uvlid);
	fp->nflushed  = 0;
	fp->dxplid	  = dxpl_id;
	fp->type	  = H5I_FILE;
	fp->idxvalid  = false;
	fp->metadirty = false;
	fp->fp		  = fp;
	err			  = H5Pget_nb_buffer_size (fapl_id, &(fp->bsize));
	CHECK_ERR
	err=H5VL_log_filei_pool_init(&(fp->data_buf),fp->bsize);
	CHECK_ERR
	err=H5VL_log_filei_contig_buffer_init(&(fp->meta_buf), 2097152);	// 200 MiB
	CHECK_ERR
#ifdef LOGVOL_PROFILING
	H5VL_log_profile_reset (fp);
#endif

	// Create the file with underlying VOL
	under_fapl_id = H5Pcopy (fapl_id);
	H5Pset_vol (under_fapl_id, uvlid, under_vol_info);
	TIMER_START;
	fp->uo = H5VLfile_open (name, flags, under_fapl_id, dxpl_id, NULL);
	CHECK_NERR (fp->uo)
	TIMER_STOP (fp, TIMER_H5VL_FILE_OPEN);
	H5Pclose (under_fapl_id);

	// Create LOG group
	loc.obj_type = H5I_FILE;
	loc.type	 = H5VL_OBJECT_BY_SELF;
	TIMER_START
	fp->lgp = H5VLgroup_open (fp->uo, &loc, fp->uvlid, LOG_GROUP_NAME, H5P_DEFAULT, dxpl_id, NULL);
	CHECK_NERR (fp->lgp)
	TIMER_STOP (fp, TIMER_H5VL_GROUP_OPEN);

	// Att
	err = H5VL_logi_get_att (fp, "_ndset", H5T_NATIVE_INT32, &(fp->ndset), dxpl_id);
	CHECK_ERR
	err = H5VL_logi_get_att (fp, "_nldset", H5T_NATIVE_INT32, &(fp->nldset), dxpl_id);
	CHECK_ERR
	fp->idx.resize (fp->ndset);

	// Open the file with MPI
	mpierr = MPI_File_open (fp->comm, name, MPI_MODE_RDWR, MPI_INFO_NULL, &(fp->fh));
	CHECK_MPIERR

	TIMER_STOP (fp, TIMER_FILE_OPEN);

	goto fn_exit;
err_out:;
	if (fp) { delete fp; }
	fp = NULL;
fn_exit:;
	MPI_Comm_free (&comm);

	if (info) { free (info); }

	return (void *)fp;
} /* end H5VL_log_file_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_file_get
 *
 * Purpose:     Get info about a file
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_file_get (
	void *file, H5VL_file_get_t get_type, hid_t dxpl_id, void **req, va_list arguments) {
	herr_t err = 0;
	H5VL_log_file_t *fp = (H5VL_log_file_t *)file;
	TIMER_START;

#ifdef LOGVOL_VERBOSE_DEBUG
	{
		char vname[2][128];
		ssize_t nsize;

		nsize = H5Iget_name (get_type, vname[0], 128);
		if (nsize == 0) {
			sprintf (vname[0], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[0], "Unknown_Object");
		}
		nsize = H5Iget_name (dxpl_id, vname[1], 128);
		if (nsize == 0) {
			sprintf (vname[1], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[1], "Unknown_Object");
		}

		printf ("H5VL_log_file_get(%p, %s, %s, %p, ...)\n", file, vname[0], vname[1], req);
	}
#endif

	TIMER_START;
	err = H5VLfile_get (fp->uo, fp->uvlid, get_type, dxpl_id, req, arguments);
	CHECK_ERR
	TIMER_STOP (fp, TIMER_H5VL_FILE_GET);

	TIMER_STOP (fp, TIMER_FILE_GET);

err_out:;
	return err;
} /* end H5VL_log_file_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_file_specific
 *
 * Purpose:     Specific operation on file
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_file_specific (
	void *file, H5VL_file_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments) {
	herr_t err = 0;
	H5VL_log_file_t *fp = (H5VL_log_file_t *)file;
	TIMER_START;

#ifdef LOGVOL_VERBOSE_DEBUG
	{
		char vname[2][128];
		ssize_t nsize;

		nsize = H5Iget_name (specific_type, vname[0], 128);
		if (nsize == 0) {
			sprintf (vname[0], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[0], "Unknown_Object");
		}
		nsize = H5Iget_name (dxpl_id, vname[1], 128);
		if (nsize == 0) {
			sprintf (vname[1], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[1], "Unknown_Object");
		}

		printf ("H5VL_log_file_specific(%p, %s, %s, %p, ...)\n", file, vname[0], vname[1], req);
	}
#endif

	switch (specific_type) {
		case H5VL_FILE_IS_ACCESSIBLE:
		case H5VL_FILE_DELETE: {
			hid_t uvlid, under_fapl_id, fapl_id;
			void *under_vol_info;
			H5VL_log_info_t *info = NULL;

			// Try get info about under VOL
			fapl_id = va_arg (arguments, hid_t);
			H5Pget_vol_info (fapl_id, (void **)&info);
			if (info) {
				uvlid		   = info->uvlid;
				under_vol_info = info->under_vol_info;
				free (info);
			} else {  // If no under VOL specified, use the native VOL
				htri_t ret;
				ret = H5VLis_connector_registered_by_name ("native");
				if (ret != 1) { RET_ERR ("Native VOL not found") }
				uvlid = H5VLpeek_connector_id_by_name ("native");
				CHECK_ID (uvlid)
				under_vol_info = NULL;
			}

			/* Call specific of under VOL */
			under_fapl_id = H5Pcopy (fapl_id);
			H5Pset_vol (under_fapl_id, uvlid, under_vol_info);
			TIMER_START;
			err = H5VLfile_specific (NULL, uvlid, specific_type, dxpl_id, req, arguments);
			CHECK_ERR
			TIMER_STOP (fp, TIMER_H5VL_FILE_SPECIFIC);
			H5Pclose (under_fapl_id);
		} break;
		case H5VL_FILE_FLUSH: {
			err = H5VL_log_nb_flush_write_reqs (fp, dxpl_id);
			break;
		} break;
		default:
			RET_ERR ("Unsupported specific_type")
	} /* end select */

	TIMER_STOP (fp, TIMER_FILE_SPECIFIC);
err_out:;
	return err;
} /* end H5VL_log_file_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_file_optional
 *
 * Purpose:     Perform a connector-specific operation on a file
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_file_optional (
	void *file, H5VL_file_optional_t opt_type, hid_t dxpl_id, void **req, va_list arguments) {
	herr_t err = 0;
	H5VL_log_file_t *fp = (H5VL_log_file_t *)file;
	TIMER_START;

#ifdef LOGVOL_VERBOSE_DEBUG
	{
		char vname[2][128];
		ssize_t nsize;

		nsize = H5Iget_name (opt_type, vname[0], 128);
		if (nsize == 0) {
			sprintf (vname[0], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[0], "Unknown_Object");
		}
		nsize = H5Iget_name (dxpl_id, vname[1], 128);
		if (nsize == 0) {
			sprintf (vname[1], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[1], "Unknown_Object");
		}

		printf ("H5VL_log_file_optional(%p, %s, %s, %p, ...)\n", file, vname[0], vname[1], req);
	}
#endif
	TIMER_START;
	err = H5VLfile_optional (fp->uo, fp->uvlid, opt_type, dxpl_id, req, arguments);
	CHECK_ERR
	TIMER_STOP (fp, TIMER_H5VL_FILE_OPTIONAL);
	

	TIMER_STOP (fp, TIMER_FILE_OPTIONAL);
err_out:;
	return err;
} /* end H5VL_log_file_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_file_close
 *
 * Purpose:     Closes a file.
 *
 * Return:      Success:    0
 *              Failure:    -1, file not closed.
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_file_close (void *file, hid_t dxpl_id, void **req) {
	herr_t err = 0;
	int mpierr;
	H5VL_log_file_t *fp = (H5VL_log_file_t *)file;
	TIMER_START;

#ifdef LOGVOL_VERBOSE_DEBUG
	{
		char vname[1][128];
		ssize_t nsize;

		nsize = H5Iget_name (dxpl_id, vname[0], 128);
		if (nsize == 0) {
			sprintf (vname[0], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[0], "Unknown_Object");
		}

		printf ("H5VL_log_file_close(%p, %s, %p, ...)\n", file, vname[0], req);
	}
#endif

	if (fp->flag != H5F_ACC_RDONLY) {
		// Flush write requests
		if (fp->wreqs.size () > fp->nflushed) {
			err = H5VL_log_nb_flush_write_reqs (fp, dxpl_id);
			CHECK_ERR
		}

		// Generate metadata table
		err = H5VL_log_filei_metaflush (fp);
		CHECK_ERR

		// Att
		err = H5VL_logi_put_att (fp, "_ndset", H5T_NATIVE_INT32, &(fp->ndset), dxpl_id);
		CHECK_ERR
	}

	// Close log group
	TIMER_START
	err = H5VLgroup_close (fp->lgp, fp->uvlid, dxpl_id, req);
	CHECK_ERR
	TIMER_STOP (fp, TIMER_H5VL_GROUP_CLOSE);

	// Close the file with MPI
	mpierr = MPI_File_close (&(fp->fh));
	CHECK_MPIERR

	H5VL_log_filei_contig_buffer_free(&(fp->meta_buf));

	// Close the file with under VOL
	TIMER_START;
	err = H5VLfile_close (fp->uo, fp->uvlid, dxpl_id, req);
	CHECK_ERR
	TIMER_STOP (fp, TIMER_H5VL_FILE_CLOSE);

	TIMER_STOP (fp, TIMER_FILE_CLOSE);
#ifdef LOGVOL_PROFILING
	H5VL_log_profile_print (fp);
#endif

	// Clean up
	MPI_Comm_free (&(fp->comm));
	H5Idec_ref (fp->uvlid);
	delete fp;

err_out:
	return err;
} /* end H5VL_log_file_close() */
