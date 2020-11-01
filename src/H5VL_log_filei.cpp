#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "H5VL_log_filei.hpp"

#include <array>

#include "H5VL_log_file.hpp"
#include "H5VL_log.h"
#include "H5VL_logi.hpp"
#include "H5VL_logi_util.hpp"
#include "H5VL_logi_wrapper.hpp"
#include "H5VL_logi_zip.hpp"

//#define DEFAULT_SIZE 1073741824 // 1 GiB
#define DEFAULT_SIZE 209715200	// 200 MiB
//#define DEFAULT_SIZE 10485760 // 10 MiB

herr_t H5VL_log_filei_balloc (H5VL_log_file_t *fp, size_t size, void **buf) {
	herr_t err = 0;
	size_t *bp;

	// printf("Balloc %llu\n", size);

	if (fp->bsize != LOG_VOL_BSIZE_UNLIMITED) {
		if (fp->bused + size > fp->bsize) {
			err = -1;
			RET_ERR ("Out of buffer")
		}
	}

	bp = (size_t *)malloc (size + sizeof (size_t));
	if (bp == NULL) {
		err = -1;

		RET_ERR ("OOM")
	}
	*bp	 = size;
	*buf = bp + 1;

	fp->bused += size;

err_out:;
	return err;
}

herr_t H5VL_log_filei_bfree (H5VL_log_file_t *fp, void *buf) {
	herr_t err = 0;
	size_t *bp;

	bp = ((size_t *)buf) - 1;
	fp->bused -= bp[0];
	free (bp);

err_out:;
	return err;
}

herr_t H5VL_log_filei_parse_fapl (H5VL_log_file_t *fp, hid_t faplid) {
	herr_t err = 0;
	hbool_t ret;
	H5VL_log_sel_encoding_t encoding;
	char *env;

	err = H5Pget_meta_merge (faplid, &ret);
	CHECK_ERR
	if (ret) { fp->config |= H5VL_FILEI_CONFIG_METADATA_MERGE; }
	err = H5Pget_meta_zip (faplid, &ret);
	CHECK_ERR
	if (ret) { fp->config |= H5VL_FILEI_CONFIG_METADATA_ZIP; }
	err = H5Pget_sel_encoding (faplid, &encoding);
	CHECK_ERR
	if (ret) { fp->config |= H5VL_FILEI_CONFIG_METADATA_ENCODE; }

	env = getenv ("H5VL_LOG_METADATA_MERGE");
	if (env) {
		if (strcmp (env, "1") == 0) {
			fp->config |= H5VL_FILEI_CONFIG_METADATA_MERGE;
		} else {
			fp->config &= ~H5VL_FILEI_CONFIG_METADATA_MERGE;
		}
	}
	env = getenv ("H5VL_LOG_METADATA_ZIP");
	if (env) {
		if (strcmp (env, "1") == 0) {
			fp->config |= H5VL_FILEI_CONFIG_METADATA_ZIP;
		} else {
			fp->config &= ~H5VL_FILEI_CONFIG_METADATA_ZIP;
		}
	}
	env = getenv ("H5VL_LOG_SEL_ENCODING");
	if (env) {
		if (strcmp (env, "canonical") == 0) {
			fp->config &= ~H5VL_FILEI_CONFIG_METADATA_ENCODE;

		} else {
			fp->config |= H5VL_FILEI_CONFIG_METADATA_ZIP;
		}
	}

err_out:;
	return err;
}

H5VL_log_buffer_block_t *H5VL_log_filei_pool_new_block (size_t bsize) {
	herr_t err					= 0;
	H5VL_log_buffer_block_t *bp = NULL;

	assert (bsize > 0);

	bp = (H5VL_log_buffer_block_t *)malloc (sizeof (H5VL_log_buffer_block_t));
	CHECK_NERR (bp)

	bp->cur = bp->begin = (char *)malloc (bsize);
	CHECK_NERR (bp->begin);
	bp->end = bp->begin + bsize;

	bp->next = NULL;

err_out:;
	if (!(bp->begin)) {
		free (bp);
		bp = NULL;
	}
	return bp;
}

herr_t H5VL_log_filei_pool_alloc (H5VL_log_buffer_pool_t *p, size_t bsize, void **buf) {
	herr_t err = 0;
	H5VL_log_buffer_block_t *bp;

	// Need to add blocks
	if (p->head->cur + bsize > p->head->end) {
		size_t asize;
		if (!(p->inf)) {
			err = -1;
			RET_ERR ("Out of buffer")
		}

		asize = p->bsize;
		if (bsize > p->bsize) {
			bp = H5VL_log_filei_pool_new_block (bsize);
		} else {
			if (p->free_blocks) {  // pull from recycle list
				bp			   = p->free_blocks;
				p->free_blocks = bp->next;
			} else {
				bp = H5VL_log_filei_pool_new_block ((size_t) (p->bsize));
			}
		}
		if (!bp) err = -1;
		CHECK_NERR (bp)

		bp->next = p->head;
		p->head	 = bp;
	}

	*buf = p->head->cur;
	p->head->cur += bsize;

err_out:;
	return err;
}

herr_t H5VL_log_filei_pool_init (H5VL_log_buffer_pool_t *p, ssize_t bsize) {
	herr_t err = 0;

	if (bsize < 0) {
		p->bsize = DEFAULT_SIZE;
		p->inf	 = 1;
	} else {
		p->bsize = bsize;
		p->inf	 = 0;
	}

	if (p->bsize) {
		p->head = H5VL_log_filei_pool_new_block ((size_t) (p->bsize));
		CHECK_NERR (p->head);
	} else {
		p->head = NULL;
	}
	p->free_blocks = NULL;

err_out:;
	return err;
}

herr_t H5VL_log_filei_pool_free (H5VL_log_buffer_pool_t *p) {
	herr_t err = 0;
	H5VL_log_buffer_block_t *i, *j = NULL;

	for (i = p->head->next; i; i = i->next) {
		i->cur = i->begin;
		j	   = i;
	}

	if (j) {
		j->next		   = p->free_blocks;
		p->free_blocks = p->head->next;
		p->head->next  = NULL;
	}
	p->head->cur = p->head->begin;

err_out:;
	return err;
}

herr_t H5VL_log_filei_pool_finalize (H5VL_log_buffer_pool_t *p) {
	herr_t err = 0;
	H5VL_log_buffer_block_t *i, *j;

	for (i = p->head; i; i = j) {
		j = i->next;
		free (i->begin);
		free (i);
	}
	p->head = NULL;
	for (i = p->free_blocks; i; i = j) {
		j = i->next;
		free (i->begin);
		free (i);
	}
	p->free_blocks = NULL;

	p->bsize = 0;
	p->inf	 = 0;
err_out:;
	return err;
}

herr_t H5VL_log_filei_contig_buffer_init (H5VL_log_contig_buffer_t *bp, size_t init_size) {
	herr_t err = 0;

	bp->begin = (char *)malloc (init_size);
	CHECK_NERR (bp->begin);

	bp->cur = bp->begin;
	bp->end += init_size;

err_out:;
	return err;
}

void H5VL_log_filei_contig_buffer_free (H5VL_log_contig_buffer_t *bp) { free (bp->begin); }

void *H5VL_log_filei_contig_buffer_alloc (H5VL_log_contig_buffer_t *bp, size_t size) {
	char *tmp;

	if (bp->cur + size > bp->end) {
		size_t new_size = bp->end - bp->begin;
		size_t used		= bp->end - bp->cur;

		while (used + size > new_size) new_size <<= 1;

		tmp = (char *)realloc (bp->begin, new_size);
		if (!tmp) return NULL;

		bp->begin = tmp;
		bp->cur	  = bp->begin + used;
		bp->end	  = bp->begin + new_size;
	}

	tmp = bp->cur;
	bp->cur += size;

	return (void *)tmp;
}

herr_t H5VL_log_filei_contig_buffer_alloc (H5VL_log_buffer_pool_t *p) {
	herr_t err = 0;
	H5VL_log_buffer_block_t *i, *j;

	for (i = p->head; i; i = j) {
		j = i->next;
		free (i->begin);
		free (i);
	}
	p->head = NULL;
	for (i = p->free_blocks; i; i = j) {
		j = i->next;
		free (i->begin);
		free (i);
	}
	p->free_blocks = NULL;

	p->bsize = 0;
	p->inf	 = 0;
err_out:;
	return err;
}

herr_t H5VL_log_filei_flush (H5VL_log_file_t *fp, hid_t dxplid) {
	herr_t err = 0;
	TIMER_START;

	if (fp->wreqs.size () > 0) {
		err = H5VL_log_nb_flush_write_reqs (fp, dxplid);
		CHECK_ERR
	}

	if (fp->rreqs.size () > 0) {
		err = H5VL_log_nb_flush_read_reqs (fp, fp->rreqs, dxplid);
		CHECK_ERR
		fp->rreqs.clear ();
	}

	TIMER_STOP (fp, TIMER_FILEI_FLUSH);
err_out:;
	return err;
}

herr_t H5VL_log_filei_close (H5VL_log_file_t *fp) {
	herr_t err = 0;
	int mpierr;
	int attbuf[3];

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
			err = H5VL_log_nb_flush_write_reqs (fp, fp->dxplid);
			CHECK_ERR
		}

		// Generate metadata table
		err = H5VL_log_filei_metaflush (fp);
		CHECK_ERR

		// Att
		attbuf[0] = fp->ndset;
		attbuf[1] = fp->nldset;
		attbuf[2] = fp->nmdset;
		err		  = H5VL_logi_put_att (fp, "_int_att", H5T_NATIVE_INT32, attbuf, fp->dxplid);
		CHECK_ERR
	}

	// Close log group
	TIMER_START
	err = H5VLgroup_close (fp->lgp, fp->uvlid, fp->dxplid, NULL);
	CHECK_ERR
	TIMER_STOP (fp, TIMER_H5VL_GROUP_CLOSE);

	// Close the file with MPI
	mpierr = MPI_File_close (&(fp->fh));
	CHECK_MPIERR

	H5VL_log_filei_contig_buffer_free (&(fp->meta_buf));

	// Close contig dataspacce ID
	H5VL_log_dataspace_contig_ref--;
	if (H5VL_log_dataspace_contig_ref == 0) { H5Sclose (H5VL_log_dataspace_contig); }

	// Close the file with under VOL
	TIMER_START;
	err = H5VLfile_close (fp->uo, fp->uvlid, H5P_DATASET_XFER_DEFAULT, NULL);
	CHECK_ERR
	TIMER_STOP (fp, TIMER_H5VL_FILE_CLOSE);

	TIMER_STOP (fp, TIMER_FILE_CLOSE);
#ifdef LOGVOL_PROFILING
	H5VL_log_profile_print (fp);
#endif

	// Clean up
	MPI_Comm_free (&(fp->comm));
	delete fp;

err_out:
	return err;
} /* end H5VL_log_file_close() */

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
void *H5VL_log_filei_wrap (void *uo, H5VL_log_obj_t *cp) {
	herr_t err = 0;
	int mpierr;
	H5VL_log_file_t *fp = NULL;
	H5VL_loc_params_t loc;
	int attbuf[3];
	TIMER_START;

	/*
		fp = new H5VL_log_file_t (uo, cp->uvlid);
		CHECK_NERR (fp)
		fp->flag = cp->fp->flag;
		MPI_Comm_dup (cp->fp->comm, &(fp->comm));
		fp->rank   = cp->fp->rank;
		fp->dxplid = H5Pcopy (cp->fp->dxplid);
		fp->bsize  = cp->fp->bsize;
		fp->name   = cp->fp->name;

		// err=H5VL_log_filei_pool_init(&(fp->data_buf),fp->bsize);
		// CHECK_ERR
		err = H5VL_log_filei_contig_buffer_init (&(fp->meta_buf), 2097152);	 // 200 MiB
		CHECK_ERR

		// Create LOG group
		loc.obj_type = H5I_FILE;
		loc.type	 = H5VL_OBJECT_BY_SELF;
		TIMER_START
		fp->lgp = H5VLgroup_open (fp->uo, &loc, fp->uvlid, LOG_GROUP_NAME, H5P_GROUP_ACCESS_DEFAULT,
								  fp->dxplid, NULL);
		CHECK_NERR (fp->lgp)
		TIMER_STOP (fp, TIMER_H5VL_GROUP_OPEN);

		// Att
		err = H5VL_logi_get_att (fp, "_int_att", H5T_NATIVE_INT32, attbuf, fp->dxplid);
		CHECK_ERR
		fp->ndset  = attbuf[0];
		fp->nldset = attbuf[1];
		fp->nmdset = attbuf[2];
		fp->idx.resize (fp->ndset);

		// Open the file with MPI
		mpierr = MPI_File_open (fp->comm, fp->name.c_str (), MPI_MODE_RDWR, MPI_INFO_NULL,
	   &(fp->fh)); CHECK_MPIERR
	*/

	fp = cp->fp;

	TIMER_STOP (fp, TIMER_FILE_OPEN);

	goto fn_exit;
err_out:;
	if (fp) { delete fp; }
	fp = NULL;
fn_exit:;
	return (void *)fp;
} /* end H5VL_log_dataset_open() */

void H5VL_log_filei_inc_ref (H5VL_log_file_t *fp) { fp->refcnt++; }

herr_t H5VL_log_filei_dec_ref (H5VL_log_file_t *fp) {
	fp->refcnt--;
	if (fp->refcnt == 0) { return H5VL_log_filei_close (fp); }
	return 0;
}

H5VL_log_file_t::H5VL_log_file_t () {
	this->refcnt	= 1;
	this->closing	= false;
	this->fp		= this;
	this->type		= H5I_FILE;
	this->nflushed	= 0;
	this->type		= H5I_FILE;
	this->idxvalid	= false;
	this->metadirty = false;
#ifdef LOGVOL_DEBUG
	this->ext_ref = 0;
#endif
#ifdef LOGVOL_PROFILING
	H5VL_log_profile_reset (fp);
#endif
}
H5VL_log_file_t::H5VL_log_file_t (hid_t uvlid) : H5VL_log_file_t () {
	this->uvlid = uvlid;
#ifdef LOGVOL_DEBUG
	this->ext_ref = 1;
#endif
	H5VL_logi_inc_ref (this->uvlid);
}
H5VL_log_file_t::H5VL_log_file_t (void *uo, hid_t uvlid) : H5VL_log_file_t (uvlid) {
	this->uo = uo;
}
/*
H5VL_log_file_t::~H5VL_log_file_t () {
#ifdef LOGVOL_DEBUG
	this->ext_ref--;
	assert (this->ext_ref >= 0);
#endif
	H5VL_logi_dec_ref (this->uvlid);
}
*/