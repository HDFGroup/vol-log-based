#include "logvol_internal.hpp"

/* Characteristics of the pass-through VOL connector */
#define H5VL_log_NAME	 "LOG"
#define H5VL_log_VALUE	 1026 /* VOL connector ID */
#define H5VL_log_VERSION 1111

/********************* */
/* Function prototypes */
/********************* */
herr_t H5VL_log_init (hid_t vipl_id);
herr_t H5VL_log_obj_term (void);
void *H5VL_log_info_copy (const void *info);
herr_t H5VL_log_info_cmp (int *cmp_value, const void *info1, const void *info2);
herr_t H5VL_log_info_free (void *info);
herr_t H5VL_log_info_to_str (const void *info, char **str);
herr_t H5VL_log_str_to_info (const char *str, void **info);
void *H5VL_log_get_object (const void *obj);
herr_t H5VL_log_get_wrap_ctx (const void *obj, void **wrap_ctx);
herr_t H5VL_log_free_wrap_ctx (void *obj);
void *H5VL_log_wrap_object (void *obj, H5I_type_t obj_type, void *wrap_ctx);
void *H5VL_log_unwrap_object (void *wrap_ctx);
herr_t H5VL_log_optional (void *obj, int op_type, hid_t dxpl_id, void **req, va_list arguments);

/*******************/
/* Local variables */
/*******************/

/* PNC VOL connector class struct */
const H5VL_class_t H5VL_log_g = {H5VL_log_VERSION,					 /* version      */
								 (H5VL_class_value_t)H5VL_log_VALUE, /* value        */
								 H5VL_log_NAME,						 /* name         */
								 0,									 /* capability flags */
								 H5VL_log_init,						 /* initialize   */
								 H5VL_log_obj_term,					 /* terminate    */
								 {
									 sizeof (H5VL_log_info_t), /* info size    */
									 H5VL_log_info_copy,	   /* info copy    */
									 H5VL_log_info_cmp,		   /* info compare */
									 H5VL_log_info_free,	   /* info free    */
									 H5VL_log_info_to_str,	   /* info to str  */
									 H5VL_log_str_to_info,	   /* str to info  */
								 },
								 {
									 H5VL_log_get_object,	 /* get_object   */
									 H5VL_log_get_wrap_ctx,	 /* get_wrap_ctx */
									 H5VL_log_wrap_object,	 /* wrap_object  */
									 H5VL_log_unwrap_object, /* wrap_object  */
									 H5VL_log_free_wrap_ctx, /* free_wrap_ctx */
								 },
								 H5VL_log_attr_g,
								 H5VL_log_dataset_g,
								 H5VL_log_datatype_g,
								 H5VL_log_file_g,  /* file_cls */
								 H5VL_log_group_g, /* group_cls */
								 H5VL_log_link_g,
								 H5VL_log_object_g,
								 H5VL_log_introspect_g,
								 {
									 /* request_cls */
									 NULL, /* wait */
									 NULL, /* notify */
									 NULL, /* cancel */
									 NULL, /* specific */
									 NULL, /* optional */
									 NULL  /* free */
								 },
								 H5VL_log_blob_g,
								 H5VL_log_token_g,
								 H5VL_log_optional};

/* The connector identification number, initialized at runtime */
hid_t H5VL_LOG_g = H5I_INVALID_HID;

H5PL_type_t H5PLget_plugin_type (void) { return H5PL_TYPE_VOL; }
const void *H5PLget_plugin_info (void) { return &H5VL_log_g; }

int mpi_inited;

/*-------------------------------------------------------------------------
 * Function:    H5Pset_vol_log
 *
 * Purpose:     Set to use the log VOL in a file access property list
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5Pset_vol_log (hid_t fapl_id) {
	herr_t err = 0;
	hid_t logvlid;
	H5VL_log_info_t log_vol_info;

	// Register the VOL if not yet registered
	logvlid = H5VL_log_register ();
	CHECK_ID (logvlid)

	err = H5Pget_vol_id (fapl_id, &(log_vol_info.uvlid));
	CHECK_ERR
	err = H5Pget_vol_info (fapl_id, &(log_vol_info.under_vol_info));
	CHECK_ERR

	err = H5Pset_vol (fapl_id, logvlid, &log_vol_info);
	CHECK_ERR

err_out:
	return err;
}

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_init
 *
 * Purpose:     Initialize this VOL connector, performing any necessary
 *      operations for the connector that will apply to all containers
 *              accessed with the connector.
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_init (hid_t vipl_id) {
	herr_t err = 0;
	int mpierr;
	H5VL_log_req_type_t blocking = H5VL_LOG_REQ_BLOCKING;
	ssize_t infty				 = LOG_VOL_BSIZE_UNLIMITED;
	htri_t exist;

	mpierr = MPI_Initialized (&mpi_inited);
	CHECK_MPIERR
	if (!mpi_inited) { MPI_Init (NULL, NULL); }

	/* SID no longer recognized at this stage, move to file close
	if(H5VL_log_dataspace_contig==H5I_INVALID_HID){
		H5VL_log_dataspace_contig = H5Screate(H5S_SCALAR);
		CHECK_ID(H5VL_log_dataspace_contig);
	}
	*/

	/* Default pclass should not be changed, insert property to plist instead
	exist = H5Pexist (H5P_DATASET_XFER, "nonblocking");
	CHECK_ID (exist)
	if (!exist) {
		err = H5Pregister2 (H5P_DATASET_XFER, "nonblocking", sizeof (H5VL_log_req_type_t),
							&blocking, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		CHECK_ERR
	}

	exist = H5Pexist (H5P_FILE_ACCESS, "nb_buffer_size");
	CHECK_ID (exist)
	if (!exist) {
		err = H5Pregister2 (H5P_FILE_ACCESS, "nb_buffer_size", sizeof (ssize_t), &infty, NULL, NULL,
							NULL, NULL, NULL, NULL, NULL);
		CHECK_ERR
	}
	*/

err_out:;
	return err;
} /* end H5VL_log_init() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_log_obj_term
 *
 * Purpose:     Terminate this VOL connector, performing any necessary
 *      operations for the connector that release connector-wide
 *      resources (usually created / initialized with the 'init'
 *      callback).
 *
 * Return:  Success:    0
 *      Failure:    (Can't fail)
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_log_obj_term (void) {
	herr_t err = 0;
	int mpierr;

	/* SID no longer recognized at this stage, move to file close
	if(H5VL_log_dataspace_contig!=H5I_INVALID_HID){
		H5VL_log_dataspace_contig=H5I_INVALID_HID;
	}
	*/

	if (!mpi_inited) {
		mpierr = MPI_Initialized (&mpi_inited);
		CHECK_MPIERR
		if (mpi_inited) { MPI_Finalize (); }
	}

err_out:;
	return err;
} /* end H5VL_log_obj_term() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_log_info_copy
 *
 * Purpose:     Duplicate the connector's info object.
 *
 * Returns: Success:    New connector info object
 *      Failure:    NULL
 *
 *---------------------------------------------------------------------------
 */
void *H5VL_log_info_copy (const void *_info) {
	const H5VL_log_info_t *info = (const H5VL_log_info_t *)_info;
	H5VL_log_info_t *new_info;

	/* Allocate new VOL info struct for the PNC connector */
	new_info = (H5VL_log_info_t *)calloc (1, sizeof (H5VL_log_info_t));

	new_info->uvlid = info->uvlid;
	H5Iinc_ref (new_info->uvlid);

	if (info->under_vol_info)
		H5VLcopy_connector_info (new_info->uvlid, &(new_info->under_vol_info),
								 info->under_vol_info);

	return (new_info);
} /* end H5VL_log_info_copy() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_log_info_cmp
 *
 * Purpose:     Compare two of the connector's info objects, setting *cmp_value,
 *      following the same rules as strcmp().
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_log_info_cmp (int *cmp_value, const void *_info1, const void *_info2) {
	const H5VL_log_info_t *info1 = (const H5VL_log_info_t *)_info1;
	const H5VL_log_info_t *info2 = (const H5VL_log_info_t *)_info2;

	/* Sanity checks */
	assert (info1);
	assert (info2);

	/* Compare under VOL connector classes */
	H5VLcmp_connector_cls (cmp_value, info1->uvlid, info2->uvlid);
	if (*cmp_value == 0) {
		/* Compare under VOL connector info objects */
		H5VLcmp_connector_info (cmp_value, info1->uvlid, info1->under_vol_info,
								info2->under_vol_info);
	}

	return (0);
} /* end H5VL_log_info_cmp() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_log_info_free
 *
 * Purpose:     Release an info object for the connector.
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_log_info_free (void *_info) {
	H5VL_log_info_t *info = (H5VL_log_info_t *)_info;
	hid_t err_id;

#ifdef LOGVOL_VERBOSE_DEBUG
	printf ("H5VL_log_info_free(%p)\n", _info);
#endif

	/* Release MPI_Info */
	// MPI_Comm_free(&(info->comm));

	err_id = H5Eget_current_stack ();

	/* Release underlying VOL ID and info */
	if (info->uvlid) H5VLfree_connector_info (info->uvlid, info->under_vol_info);
	H5Idec_ref (info->uvlid);

	H5Eset_current_stack (err_id);

	/* Free PNC info object itself */
	free (info);

	return (0);
} /* end H5VL_log_info_free() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_log_info_to_str
 *
 * Purpose:     Serialize an info object for this connector into a string
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_log_info_to_str (const void *_info, char **str) {
	const H5VL_log_info_t *info	   = (const H5VL_log_info_t *)_info;
	H5VL_class_value_t under_value = (H5VL_class_value_t)-1;
	char *under_vol_string		   = NULL;
	size_t under_vol_str_len	   = 0;

#ifdef LOGVOL_VERBOSE_DEBUG
	printf ("H5VL_log_info_to_str(%p, %p)\n", _info, str);
#endif

	/* Get value and string for underlying VOL connector */
	H5VLget_value (info->uvlid, &under_value);
	H5VLconnector_info_to_str (info->under_vol_info, info->uvlid, &under_vol_string);

	/* Determine length of underlying VOL info string */
	if (under_vol_string) under_vol_str_len = strlen (under_vol_string);

	/* Allocate space for our info */
	*str = (char *)H5allocate_memory (32 + under_vol_str_len, (hbool_t)0);
	assert (*str);

	/* Encode our info
	 * Normally we'd use snprintf() here for a little extra safety, but that
	 * call had problems on Windows until recently. So, to be as platform-independent
	 * as we can, we're using sprintf() instead.
	 */
	sprintf (*str, "under_vol=%u;under_info={%s}", (unsigned)under_value,
			 (under_vol_string ? under_vol_string : ""));

	return (0);
} /* end H5VL_log_info_to_str() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_log_str_to_info
 *
 * Purpose:     Deserialize a string into an info object for this connector.
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_log_str_to_info (const char *str, void **_info) {
	H5VL_log_info_t *info = (H5VL_log_info_t *)_info;
	unsigned under_vol_value;
	const char *under_vol_info_start, *under_vol_info_end;
	hid_t uvlid;
	void *under_vol_info = NULL;

#ifdef LOGVOL_VERBOSE_DEBUG
	printf ("H5VL_log_str_to_info(%s, %p)\n", str, _info);
#endif

	/* Retrieve the underlying VOL connector value and info */
	sscanf (str, "under_vol=%u;", &under_vol_value);
	uvlid = H5VLregister_connector_by_value ((H5VL_class_value_t)under_vol_value, H5P_DEFAULT);
	under_vol_info_start = strchr (str, '{');
	under_vol_info_end	 = strrchr (str, '}');
	assert (under_vol_info_end > under_vol_info_start);
	if (under_vol_info_end != (under_vol_info_start + 1)) {
		char *under_vol_info_str;

		under_vol_info_str = (char *)malloc ((size_t) (under_vol_info_end - under_vol_info_start));
		memcpy (under_vol_info_str, under_vol_info_start + 1,
				(size_t) ((under_vol_info_end - under_vol_info_start) - 1));
		*(under_vol_info_str + (under_vol_info_end - under_vol_info_start)) = '\0';

		H5VLconnector_str_to_info (under_vol_info_str, uvlid, &under_vol_info);

		free (under_vol_info_str);
	} /* end else */

	/* Allocate new pass-through VOL connector info and set its fields */
	info				 = (H5VL_log_info_t *)calloc (1, sizeof (H5VL_log_info_t));
	info->uvlid			 = uvlid;
	info->under_vol_info = under_vol_info;

	/* Set return value */
	*_info = info;

	return 0;
} /* end H5VL_log_str_to_info() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_log_get_object
 *
 * Purpose:     Retrieve the 'data' for a VOL object.
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
void *H5VL_log_get_object (const void *obj) {
	const H5VL_log_obj_t *op = (const H5VL_log_obj_t *)obj;
#ifdef LOGVOL_VERBOSE_DEBUG
	printf ("H5VL_log_get_object(%p)\n", obj);
#endif
	return H5VLget_object (op->uo, op->uvlid);
} /* end H5VL_log_get_object() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_log_get_wrap_ctx
 *
 * Purpose:     Retrieve a "wrapper context" for an object
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_log_get_wrap_ctx (const void *obj, void **wrap_ctx) {
	herr_t err				 = 0;
	const H5VL_log_obj_t *op = (const H5VL_log_obj_t *)obj;
	H5VL_log_wrap_ctx_t *ctx;

#ifdef LOGVOL_VERBOSE_DEBUG
	printf ("H5VL_log_get_wrap_ctx(%p,%p)\n", obj, wrap_ctx);
#endif

	/* Allocate new VOL object wrapping context for the pass through connector */
	ctx = (H5VL_log_wrap_ctx_t *)calloc (1, sizeof (H5VL_log_wrap_ctx_t));

	/* Increment reference count on underlying VOL ID, and copy the VOL info */
	ctx->uvlid = op->uvlid;
	H5Iinc_ref (ctx->uvlid);
	err = H5VLget_wrap_ctx (op->uo, op->uvlid, &(ctx->uctx));
	CHECK_ERR

	/* Set wrap context to return */
	*wrap_ctx = ctx;

err_out:;
	return err;

} /* end H5VL_log_get_wrap_ctx() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_log_wrap_object
 *
 * Purpose:     Use a "wrapper context" to wrap a data object
 *
 * Return:      Success:    Pointer to wrapped object
 *              Failure:    NULL
 *
 *---------------------------------------------------------------------------
 */
void *H5VL_log_wrap_object (void *obj, H5I_type_t type, void *_wrap_ctx) {
	H5VL_log_wrap_ctx_t *ctx = (H5VL_log_wrap_ctx_t *)_wrap_ctx;
	H5VL_log_obj_t *wop		 = NULL;
	void *uo;

#ifdef LOGVOL_VERBOSE_DEBUG
	{
		char vname[128];
		ssize_t nsize;

		nsize = H5Iget_name (type, vname, 128);
		if (nsize == 0) {
			sprintf (vname, "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname, "Unknown_Object");
		}

		printf ("H5VL_log_wrap_object(%p, %s, %p)\n", obj, vname, _wrap_ctx);
	}
#endif

	/* Wrap the object with the underlying VOL */
	uo = H5VLwrap_object (obj, type, ctx->uvlid, ctx->uctx);
	if (uo) {
		wop		   = (H5VL_log_obj_t *)malloc (sizeof (H5VL_log_obj_t));
		wop->uo	   = uo;
		wop->uvlid = ctx->uvlid;
		H5Iinc_ref (wop->uvlid);
	} else
		wop = NULL;

	return wop;
} /* end H5VL_log_wrap_object() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_log_wrap_object
 *
 * Purpose:     Use a "wrapper context" to wrap a data object
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
void *H5VL_log_unwrap_object (void *obj) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
	void *uo;

#ifdef LOGVOL_VERBOSE_DEBUG
	printf ("H5VL_log_unwrap_object(%p)\n", obj);
#endif

	/* Unrap the object with the underlying VOL */
	uo = H5VLunwrap_object (op->uo, op->uvlid);

	if (uo) {
		H5Idec_ref (op->uvlid);
		free (op);
	}

	return uo;
} /* end H5VL_log_wrap_object() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_log_free_wrap_ctx
 *
 * Purpose:     Release a "wrapper context" for an object
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_log_free_wrap_ctx (void *_wrap_ctx) {
	herr_t err				 = 0;
	H5VL_log_wrap_ctx_t *ctx = (H5VL_log_wrap_ctx_t *)_wrap_ctx;
	hid_t err_id;
#ifdef LOGVOL_VERBOSE_DEBUG
	printf ("H5VL_log_free_wrap_ctx(%p)\n", _wrap_ctx);
#endif
	err_id = H5Eget_current_stack ();

	/* Release underlying VOL ID and wrap context */
	if (ctx->uctx) err = H5VLfree_wrap_ctx (ctx->uctx, ctx->uvlid);
	CHECK_ERR
	H5Idec_ref (ctx->uvlid);

	H5Eset_current_stack (err_id);

	/* Free pass through wrap context object itself */
	free (ctx);

err_out:;
	return err;
} /* end H5VL_log_free_wrap_ctx() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_optional
 *
 * Purpose:     Handles the generic 'optional' callback
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_optional (void *obj, int op_type, hid_t dxpl_id, void **req, va_list arguments) {
	H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
	herr_t ret_value;

#ifdef LOGVOL_VERBOSE_DEBUG
	{
		char vname[128];
		ssize_t nsize;

		nsize = H5Iget_name (dxpl_id, vname, 128);
		if (nsize == 0) {
			sprintf (vname, "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname, "Unknown_Object");
		}
		printf ("H5VL_log_optional(%p,%d,%s,%p,...\n", obj, op_type, vname, req);
	}
#endif

	ret_value = H5VLoptional (o->uo, o->uvlid, op_type, dxpl_id, req, arguments);

	return ret_value;
} /* end H5VL_log_optional() */
