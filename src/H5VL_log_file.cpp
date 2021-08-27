#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "H5VL_log.h"
#include "H5VL_log_file.hpp"
#include "H5VL_log_filei.hpp"
#include "H5VL_log_info.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_util.hpp"

/********************* */
/* Function prototypes */
/********************* */
const H5VL_file_class_t H5VL_log_file_g {
	H5VL_log_file_create,	/* create */
	H5VL_log_file_open,		/* open */
	H5VL_log_file_get,		/* get */
	H5VL_log_file_specific, /* specific */
	H5VL_log_file_optional, /* optional */
	H5VL_log_file_close		/* close */
};

int H5VL_log_dataspace_contig_ref = 0;
hid_t H5VL_log_dataspace_contig	  = H5I_INVALID_HID;

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
	MPI_Comm comm = MPI_COMM_WORLD;
	hbool_t po_supported;
	int attbuf[4];
	H5VL_LOGI_PROFILING_TIMER_START;

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
		if (ret != 1) { ERR_OUT ("Native VOL not found") }
		uvlid = H5VLpeek_connector_id_by_name ("native");
		CHECK_ID (uvlid)
		under_vol_info = NULL;
	}

	// Make sure we have mpi enabled
	err = H5Pget_fapl_mpio (fapl_id, &comm, NULL);
	if (err != 0) {	 // No MPI, use MPI_COMM_WORLD
		comm = MPI_COMM_WORLD;
	}

	// Init file obj
	fp		   = new H5VL_log_file_t (uvlid);
	fp->flag   = flags;
	fp->nldset = 0;
	fp->nmdset = 0;
	fp->ndset  = 0;
	fp->config = 0;
	mpierr	   = MPI_Comm_dup (comm, &(fp->comm));
	CHECK_MPIERR
	mpierr = MPI_Comm_rank (comm, &(fp->rank));
	CHECK_MPIERR
	mpierr = MPI_Comm_size (comm, &(fp->np));
	CHECK_MPIERR
	fp->dxplid = H5Pcopy (dxpl_id);
	fp->name   = std::string (name);
	err		   = H5Pget_nb_buffer_size (fapl_id, &(fp->bsize));
	CHECK_ERR
	// err=H5VL_log_filei_pool_init(&(fp->data_buf),fp->bsize);
	// CHECK_ERR
	err = H5VL_log_filei_contig_buffer_init (&(fp->meta_buf), 2097152);	 // 200 MiB
	CHECK_ERR
	err = H5VL_log_filei_parse_fapl (fp, fapl_id);
	CHECK_ERR
	err = H5VL_log_filei_parse_fcpl (fp, fcpl_id);
	CHECK_ERR

	// Create the file with underlying VOL
	under_fapl_id = H5Pcopy (fapl_id);
	H5Pset_vol (under_fapl_id, uvlid, under_vol_info);
	H5Pset_all_coll_metadata_ops (under_fapl_id, (hbool_t) false);
	H5Pset_coll_metadata_write (under_fapl_id, (hbool_t) true);
	H5VL_LOGI_PROFILING_TIMER_START;
	fp->uo = H5VLfile_create (name, flags, fcpl_id, under_fapl_id, dxpl_id, NULL);
	CHECK_PTR (fp->uo)
	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLFILE_CREATE);
	H5Pclose (under_fapl_id);

	// Create LOG group
	loc.obj_type = H5I_FILE;
	loc.type	 = H5VL_OBJECT_BY_SELF;
	fp->lgp = H5VLgroup_create (fp->uo, &loc, fp->uvlid, LOG_GROUP_NAME, H5P_LINK_CREATE_DEFAULT,
								H5P_GROUP_CREATE_DEFAULT, H5P_GROUP_CREATE_DEFAULT, dxpl_id, NULL);
	CHECK_PTR (fp->lgp)

	// Open the file with MPI
	mpierr = MPI_File_open (fp->comm, name, MPI_MODE_RDWR, MPI_INFO_NULL, &(fp->fh));
	CHECK_MPIERR

	// Figure out lustre configuration
	if (fp->config & H5VL_FILEI_CONFIG_DATA_ALIGN) {
		err = H5VL_log_filei_parse_strip_info (fp);
		CHECK_ERR
		// For debugging without lustre
		// fp->scount=2;
		// fp->ssize=8388608;
		if ((fp->scount <= 0) || (fp->ssize <= 0)) {
			fp->config &= ~H5VL_FILEI_CONFIG_DATA_ALIGN;
			if (fp->rank == 0) {
				printf ("Warning: Cannot retrive stripping info, disable aligned data layout\n");
			}
		} else {
			err = H5VL_log_filei_calc_node_rank (fp);
			CHECK_ERR
		}
	}
	if (fp->config & H5VL_FILEI_CONFIG_DATA_ALIGN) {
		fp->fd = open (name, O_RDWR);
		if (fp->fd < 0) { ERR_OUT ("open fail") }
	} else {
		fp->fd = -1;
	}

	// Att
	attbuf[0] = fp->ndset;
	attbuf[1] = fp->nldset;
	attbuf[2] = fp->nmdset;
	attbuf[3] = fp->config;
	err = H5VL_logi_add_att (fp, "_int_att", H5T_STD_I32LE, H5T_NATIVE_INT32, 4, attbuf, dxpl_id,
							 NULL);
	CHECK_ERR

	// create the contig SID
	if (H5VL_log_dataspace_contig_ref == 0) { H5VL_log_dataspace_contig = H5Screate (H5S_SCALAR); }
	H5VL_log_dataspace_contig_ref++;

	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILE_CREATE);

	goto fn_exit;
err_out:;
	if (fp) { delete fp; }
	fp = NULL;
fn_exit:;
	if (comm != MPI_COMM_WORLD) { MPI_Comm_free (&comm); }

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
	int attbuf[4];

	H5VL_LOGI_PROFILING_TIMER_START;

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
		if (ret != 1) { ERR_OUT ("Native VOL not found") }
		uvlid = H5VLpeek_connector_id_by_name ("native");
		CHECK_ID (uvlid)
		under_vol_info = NULL;
		// return NULL;
	}

	// Make sure we have mpi enabled
	err = H5Pget_fapl_mpio (fapl_id, &comm, NULL);
	if (err != 0) {	 // No MPI, use MPI_COMM_WORLD
		comm = MPI_COMM_WORLD;
	}

	// Init file obj
	fp		   = new H5VL_log_file_t (uvlid);
	fp->flag   = flags;
	fp->config = 0;
	mpierr	   = MPI_Comm_dup (comm, &(fp->comm));
	CHECK_MPIERR
	mpierr = MPI_Comm_rank (comm, &(fp->rank));
	CHECK_MPIERR
	mpierr = MPI_Comm_size (comm, &(fp->np));
	CHECK_MPIERR
	fp->dxplid = H5Pcopy (dxpl_id);
	fp->name   = std::string (name);
	err		   = H5Pget_nb_buffer_size (fapl_id, &(fp->bsize));
	CHECK_ERR
	// err=H5VL_log_filei_pool_init(&(fp->data_buf),fp->bsize);
	// CHECK_ERR
	err = H5VL_log_filei_contig_buffer_init (&(fp->meta_buf), 2097152);	 // 200 MiB
	CHECK_ERR

	// Create the file with underlying VOL
	under_fapl_id = H5Pcopy (fapl_id);
	H5Pset_vol (under_fapl_id, uvlid, under_vol_info);
	H5VL_LOGI_PROFILING_TIMER_START;
	fp->uo = H5VLfile_open (name, flags, under_fapl_id, dxpl_id, NULL);
	CHECK_PTR (fp->uo)
	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLFILE_OPEN);
	H5Pclose (under_fapl_id);

	// Create LOG group
	loc.obj_type = H5I_FILE;
	loc.type	 = H5VL_OBJECT_BY_SELF;
	H5VL_LOGI_PROFILING_TIMER_START
	fp->lgp = H5VLgroup_open (fp->uo, &loc, fp->uvlid, LOG_GROUP_NAME, H5P_GROUP_ACCESS_DEFAULT,
							  dxpl_id, NULL);
	CHECK_PTR (fp->lgp)
	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLGROUP_OPEN);

	// Att
	err = H5VL_logi_get_att (fp, "_int_att", H5T_NATIVE_INT32, attbuf, dxpl_id);
	CHECK_ERR
	fp->ndset  = attbuf[0];
	fp->nldset = attbuf[1];
	fp->nmdset = attbuf[2];
	fp->config = attbuf[3];
	fp->idx.resize (fp->ndset);
	fp->mreqs.resize (fp->ndset);

	// Fapl property can overwrite config in file, parse after loading config
	err = H5VL_log_filei_parse_fapl (fp, fapl_id);
	CHECK_ERR

	// Open the file with MPI
	mpierr = MPI_File_open (fp->comm, name, MPI_MODE_RDWR, MPI_INFO_NULL, &(fp->fh));
	CHECK_MPIERR

	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILE_OPEN);

	goto fn_exit;
err_out:;
	if (fp) { delete fp; }
	fp = NULL;
fn_exit:;
	if (comm != MPI_COMM_WORLD) { MPI_Comm_free (&comm); }
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
herr_t H5VL_log_file_get (void *file, H5VL_file_get_args_t *args, hid_t dxpl_id, void **req) {
	herr_t err			= 0;
	H5VL_log_file_t *fp = (H5VL_log_file_t *)file;
	H5VL_LOGI_PROFILING_TIMER_START;

#ifdef LOGVOL_VERBOSE_DEBUG
	{
		char vname[2][128];
		ssize_t nsize;

		nsize = H5Iget_name (args->get_type, vname[0], 128);
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

	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VLfile_get (fp->uo, fp->uvlid, args, dxpl_id, req);
	CHECK_ERR
	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLFILE_GET);

	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILE_GET);

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
herr_t H5VL_log_file_specific (void *file,
							   H5VL_file_specific_args_t *args,
							   hid_t dxpl_id,
							   void **req) {
	herr_t err = 0;
	int i;
	H5VL_log_file_t *fp = (H5VL_log_file_t *)file;
	H5VL_LOGI_PROFILING_TIMER_START;

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

	switch (args->op_type) {
		case H5VL_FILE_IS_ACCESSIBLE:
		case H5VL_FILE_DELETE: {
			hid_t uvlid, under_fapl_id, fapl_id;
			void *under_vol_info;
			H5VL_log_info_t *info = NULL;

			// Try get info about under VOL
			fapl_id = args->args.del.fapl_id;
			H5Pget_vol_info (fapl_id, (void **)&info);
			if (info) {
				uvlid		   = info->uvlid;
				under_vol_info = info->under_vol_info;
				free (info);
			} else {  // If no under VOL specified, use the native VOL
				htri_t ret;
				ret = H5VLis_connector_registered_by_name ("native");
				if (ret != 1) { ERR_OUT ("Native VOL not found") }
				uvlid = H5VLpeek_connector_id_by_name ("native");
				CHECK_ID (uvlid)
				under_vol_info = NULL;
			}

			/* Call specific of under VOL */
			under_fapl_id = H5Pcopy (fapl_id);
			H5Pset_vol (under_fapl_id, uvlid, under_vol_info);
			H5VL_LOGI_PROFILING_TIMER_START;
			err = H5VLfile_specific (NULL, uvlid, args, dxpl_id, req);
			CHECK_ERR
			H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLFILE_SPECIFIC);
			H5Pclose (under_fapl_id);
		} break;
		case H5VL_FILE_FLUSH: {
			// Flush all merged requests
			for (i = 0; i < fp->mreqs.size (); i++) {
				if (fp->mreqs[i] && (fp->mreqs[i]->nsel > 0)) {
					fp->wreqs.push_back (fp->mreqs[i]);
					fp->mreqs[i] = new H5VL_log_merged_wreq_t (fp, i, 1);
				}
			}
			if (fp->config & H5VL_FILEI_CONFIG_DATA_ALIGN) {
				err = H5VL_log_nb_flush_write_reqs_align (fp, dxpl_id);
			} else {
				err = H5VL_log_nb_flush_write_reqs (fp, dxpl_id);
			}
			break;
		} break;
		default:
			ERR_OUT ("Unsupported specific_type")
	} /* end select */

	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILE_SPECIFIC);
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
herr_t H5VL_log_file_optional (void *file, H5VL_optional_args_t *args, hid_t dxpl_id, void **req) {
	herr_t err			= 0;
	H5VL_log_file_t *fp = (H5VL_log_file_t *)file;
	H5VL_LOGI_PROFILING_TIMER_START;

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
	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VLfile_optional (fp->uo, fp->uvlid, args, dxpl_id, req);
	CHECK_ERR
	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLFILE_OPTIONAL);

	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILE_OPTIONAL);
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
	return H5VL_log_filei_dec_ref ((H5VL_log_file_t *)file);
	// return H5VL_log_filei_close ((H5VL_log_file_t *)file);
} /* end H5VL_log_file_close() */
