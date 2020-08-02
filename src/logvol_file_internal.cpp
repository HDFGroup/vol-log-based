#include "logvol_internal.hpp"

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

herr_t H5VL_log_filei_metaflush (H5VL_log_file_t *fp) {
	herr_t err = 0;
	int mpierr;
	int i;
	int *cnt;
	MPI_Offset *mlens = NULL, *mlens_all;
	MPI_Offset *moffs = NULL, *doffs;
	char *buf		  = NULL;
	char **bufp		  = NULL;
	H5VL_loc_params_t loc;
	void *mdp, *ldp;
	hid_t dxplid;
	hid_t mdsid = -1, ldsid = -1, mmsid = -1, lmsid = -1;
	hsize_t start, count, one = 1;
	hsize_t dsize, msize;
	htri_t has_idx;
	MPI_Offset seloff;
	TIMER_START;

	TIMER_START;
	// Calculate size and offset of the metadata per dataset
	mlens	  = (MPI_Offset *)malloc (sizeof (MPI_Offset) * fp->ndset * 2);
	mlens_all = mlens + fp->ndset;
	moffs	  = (MPI_Offset *)malloc (sizeof (MPI_Offset) * (fp->ndset + 1) * 2);
	doffs	  = moffs + fp->ndset + 1;
	cnt=(int *)malloc (sizeof (int) *fp->ndset );
	memset (cnt, 0, sizeof (int) * fp->ndset);
	memset (mlens, 0, sizeof (MPI_Offset) * fp->ndset);
	if(fp->rank==0){
		mlens[0]+=sizeof(int)*fp->ndset;
	}
	for(i=0;i<fp->ndset;i++){
		mlens[i]+=sizeof (int) * 2;
	}
	for (auto &rp : fp->wreqs) {
		mlens[rp.did] += (fp->ndim[rp.did] * sizeof (MPI_Offset) * 2 +
						  sizeof (MPI_Offset) + sizeof (size_t)) *
						 rp.sels.size ();
		cnt[rp.did]+=rp.sels.size();
	}
	moffs[0]=0;
	for (i = 0; i < fp->ndset; i++) { moffs[i + 1] = moffs[i] + mlens[i]; }
	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_INIT);

	TIMER_START;
	// Pack data
	buf	 = (char *)malloc (sizeof (char) * moffs[fp->ndset]);
	bufp = (char **)malloc (sizeof (char *) * fp->ndset);
#ifdef LOGVOL_PROFILING
	H5VL_log_profile_add_time (fp, TIMER_FILEI_METAFLUSH_SIZE,
							   (double)(moffs[fp->ndset]) / 1048576);
#endif

	for (i = 0; i < fp->ndset; i++) { bufp[i] = buf + moffs[i]; 
		*((int *)bufp[i]) = i;
		bufp[i] += sizeof (int);
		*((int *)bufp[i]) = cnt[i];
		bufp[i] += sizeof (int);
	}
	for (auto &rp : fp->wreqs) {
		seloff = 0;
		for (auto &sp : rp.sels) {
			//*((int *)bufp[rp.did]) = rp.did;
			//bufp[rp.did] += sizeof (int);
			memcpy (bufp[rp.did], sp.start, fp->ndim[rp.did] * sizeof (MPI_Offset));
			bufp[rp.did] += fp->ndim[rp.did] * sizeof (MPI_Offset);
			memcpy (bufp[rp.did], sp.count, fp->ndim[rp.did] * sizeof (MPI_Offset));
			bufp[rp.did] += fp->ndim[rp.did] * sizeof (MPI_Offset);
			*((MPI_Offset *)bufp[rp.did]) = rp.ldoff + seloff;
			bufp[rp.did] += sizeof (MPI_Offset);
			*((size_t *)bufp[rp.did]) = rp.rsize;
			bufp[rp.did] += sizeof (size_t);
			seloff += sp.size;
		}
	}
	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_PACK);

	/* Debug code to dump metadata table
	{
		int i;
		int rank, np;

		MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		MPI_Comm_size(MPI_COMM_WORLD, &np);

		for(i=0;i<np;i++){
			if (rank == i){
				hexDump(NULL, buf, moffs[fp->ndset], "wr.txt");
			}
			MPI_Barrier(MPI_COMM_WORLD);
		}
	}
	*/

	TIMER_START;
	// Sync metadata size
	mpierr = MPI_Allreduce (mlens, mlens_all, fp->ndset, MPI_LONG_LONG, MPI_SUM, fp->comm);
	CHECK_MPIERR
	// NOTE: Some MPI implementation do not produce output for rank 0, moffs must ne initialized to
	// 0
	memset (moffs, 0, sizeof (MPI_Offset) * fp->ndset);
	mpierr = MPI_Exscan (mlens, moffs, fp->ndset, MPI_LONG_LONG, MPI_SUM, fp->comm);
	CHECK_MPIERR
	doffs[0] = 0;
	for (i = 0; i < fp->ndset; i++) {
		doffs[i + 1] = doffs[i] + mlens_all[i];
		moffs[i] += doffs[i];
	}
	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_SYNC);

	TIMER_START;
	// Create metadata dataset
	loc.obj_type					 = H5I_GROUP;
	loc.type						 = H5VL_OBJECT_BY_SELF;
	loc.loc_data.loc_by_name.name	 = "_idx";
	loc.loc_data.loc_by_name.lapl_id = H5P_DATASET_ACCESS_DEFAULT;
	err = H5VLlink_specific_wrapper (fp->lgp, &loc, fp->uvlid, H5VL_LINK_EXISTS,
									 H5P_DATASET_XFER_DEFAULT, NULL, &has_idx);
	CHECK_ERR
	if (has_idx) {
		// If the exist, we expand them
		TIMER_START;
		mdp = H5VLdataset_open (fp->lgp, &loc, fp->uvlid, "_idx", H5P_DATASET_ACCESS_DEFAULT,
								fp->dxplid, NULL);
		CHECK_NERR (mdp)
		// ldp = H5VLdataset_open (fp->lgp, &loc, fp->uvlid, "_lookup", H5P_DATASET_ACCESS_DEFAULT,
		//						fp->dxplid, NULL);
		// CHECK_NERR (ldp)
		TIMER_STOP (fp, TIMER_H5VL_DATASET_OPEN);

		// Resize both dataset
		TIMER_START;
		dsize = (hsize_t)doffs[fp->ndset];
		err	  = H5VLdataset_specific_wrapper (mdp, fp->uvlid, H5VL_DATASET_SET_EXTENT, fp->dxplid,
											  NULL, &dsize);
		CHECK_ERR
		// dsize = fp->ndset;log_io_vol
		TIMER_STOP (fp, TIMER_H5VL_DATASET_SPECIFIC);

		// Get data space
		TIMER_START;
		err = H5VLdataset_get_wrapper (mdp, fp->uvlid, H5VL_DATASET_GET_SPACE, fp->dxplid, NULL,
									   &mdsid);
		CHECK_ERR
		// err = H5VLdataset_get_wrapper(ldp, fp->uvlid, H5VL_DATASET_GET_SPACE, fp->dxplid, NULL,
		// &ldsid); CHECK_ERR
		TIMER_STOP (fp, TIMER_H5VL_DATASET_GET);
	} else {  // Create the idx dataset and look up table dataset
		hid_t mdcplid, ldcplid;
		hsize_t csize;

		// Space
		dsize = (hsize_t)doffs[fp->ndset];
		msize = H5S_UNLIMITED;
		mdsid = H5Screate_simple (1, &dsize, &msize);
		CHECK_ID (mdsid)
		// dsize = fp->ndset;
		// msize = H5S_UNLIMITED;
		// ldsid = H5Screate_simple(1, &dsize, &msize); CHECK_ID(ldsid)

		// Chunk
		mdcplid = H5Pcreate (H5P_DATASET_CREATE);
		CHECK_ID (mdcplid)
		err = H5Pset_fill_time (mdcplid, H5D_FILL_TIME_NEVER);
		CHECK_ERR
		csize = dsize;	// Same as dsize
		if (csize < 1048576)
			csize = 1048576;  // No less than 1 MiB
		else if (csize > 1073741824)
			csize = 1073741824;	 // No more than 1 GiB
		err = H5Pset_chunk (mdcplid, 1, &csize);
		CHECK_ERR

		/* Skip lookup table for now
		ldcplid = H5Pcreate(H5P_DATASET_CREATE); CHECK_ID(ldcplid)
		err = H5Pset_fill_time(ldcplid, H5D_FILL_TIME_NEVER); CHECK_ERR
		dsize = 128;
		err = H5Pset_chunk(ldcplid, 1, &dsize); CHECK_ERR
		*/

		// Create
		TIMER_START;
		mdp = H5VLdataset_create (fp->lgp, &loc, fp->uvlid, "_idx", H5P_LINK_CREATE_DEFAULT,
								  H5T_STD_B8LE, mdsid, mdcplid, H5P_DATASET_ACCESS_DEFAULT,
								  fp->dxplid, NULL);
		CHECK_NERR (mdp);
		// ldp = H5VLdataset_create(fp->lgp, &loc, fp->uvlid, "_lookup", H5P_LINK_CREATE_DEFAULT,
		// H5T_STD_I64LE, ldsid, ldcplid, H5P_DATASET_ACCESS_DEFAULT, fp->dxplid, NULL);
		// CHECK_NERR(ldp);
		TIMER_STOP (fp, TIMER_H5VL_DATASET_CREATE);
	}
	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_CREATE);

	TIMER_START;
	// Write metadata
	err = H5Sselect_none (mdsid);
	CHECK_ERR
	for (i = 0; i < fp->ndset; i++) {
		start = moffs[i];
		count = mlens[i];
		err	  = H5Sselect_hyperslab (mdsid, H5S_SELECT_OR, &start, NULL, &one, &count);
		CHECK_ERR
	}
	msize = moffs[fp->ndset];
	mmsid = H5Screate_simple (1, &msize, &msize);
	CHECK_ID (mmsid)
	/*
	msize = fp->ndset;
	lmsid = H5Screate_simple (1, &msize, &msize);
	CHECK_ID (lmsid)
	if (fp->rank == 0) {
		start = 0;
		count = fp->ndset;
		err	  = H5Sselect_hyperslab (ldsid, H5S_SELECT_SET, &start, NULL, &count, NULL);
		CHECK_ERR
	} else {
		err = H5Sselect_none (ldsid);
		CHECK_ERR
		err = H5Sselect_none (lmsid);
		CHECK_ERR
	}
	*/

	dxplid = H5Pcreate (H5P_DATASET_XFER);
	err	   = H5Pset_dxpl_mpio (dxplid, H5FD_MPIO_COLLECTIVE);
	CHECK_ERR
	TIMER_START;
	err = H5VLdataset_write (mdp, fp->uvlid, H5T_NATIVE_B8, mmsid, mdsid, dxplid, buf, NULL);
	TIMER_STOP (fp, TIMER_H5VL_DATASET_WRITE);
	H5Pclose (dxplid);
	CHECK_ERR
	// err = H5VLdataset_write (ldp, fp->uvlid, H5T_STD_I64LE, lmsid, ldsid, fp->dxplid, doffs,
	// NULL); CHECK_ERR
	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_WRITE);

	TIMER_START;
	TIMER_START;
	// Close the dataset
	err = H5VLdataset_close (mdp, fp->uvlid, fp->dxplid, NULL);
	CHECK_ERR
	// err = H5VLdataset_close (ldp, fp->uvlid, fp->dxplid, NULL);
	// CHECK_ERR
	TIMER_STOP (fp, TIMER_H5VL_DATASET_CLOSE);

	if (moffs[fp->ndset] > 0) { fp->idxvalid = false; }

	fp->metadirty = false;
	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_CLOSE);

	TIMER_START;
	// This barrier is required to ensure no process read metadata before everyone finishes writing
	MPI_Barrier (MPI_COMM_WORLD);
	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_BARR);

	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH);
err_out:
	// Cleanup
	H5VL_log_free (mlens);
	H5VL_log_free (moffs);
	H5VL_log_free (buf);
	H5VL_log_free (bufp);
	H5VL_log_free (mlens);
	H5VL_log_Sclose (mdsid);
	H5VL_log_Sclose (ldsid);
	H5VL_log_Sclose (mmsid);
	H5VL_log_Sclose (lmsid);

	return err;
}

herr_t H5VL_log_filei_metaupdate (H5VL_log_file_t *fp) {
	herr_t err = 0;
	int i;
	H5VL_loc_params_t loc;
	htri_t mdexist;
	void *mdp = NULL, *ldp = NULL;
	hid_t mdsid = -1, ldsid = -1;
	hsize_t mdsize, ldsize;
	char *buf = NULL, *bufp;
	int ndim;
	H5VL_log_metaentry_t entry;
	TIMER_START;

	if (fp->metadirty) { H5VL_log_filei_metaflush (fp); }

	loc.obj_type					 = H5I_GROUP;
	loc.type						 = H5VL_OBJECT_BY_SELF;
	loc.loc_data.loc_by_name.name	 = "_idx";
	loc.loc_data.loc_by_name.lapl_id = H5P_LINK_ACCESS_DEFAULT;
	err = H5VLlink_specific_wrapper (fp->lgp, &loc, fp->uvlid, H5VL_LINK_EXISTS, fp->dxplid, NULL,
									 &mdexist);
	CHECK_ERR

	if (mdexist > 0) {
		mdp = H5VLdataset_open (fp->lgp, &loc, fp->uvlid, "_idx", H5P_DATASET_ACCESS_DEFAULT,
								fp->dxplid, NULL);
		CHECK_NERR (mdp)
		// ldp = H5VLdataset_open(fp->lgp, &loc, fp->uvlid, "_lookup", H5P_DATASET_ACCESS_DEFAULT,
		// fp->dxplid, NULL); CHECK_NERR(ldp)

		// Get data space and size
		TIMER_START;
		err = H5VLdataset_get_wrapper (mdp, fp->uvlid, H5VL_DATASET_GET_SPACE, fp->dxplid, NULL,
									   &mdsid);
		CHECK_ERR
		TIMER_STOP (fp, TIMER_H5VL_DATASET_GET);
		// err = H5VLdataset_get_wrapper(ldp, fp->uvlid, H5VL_DATASET_GET_SPACE, fp->dxplid, NULL,
		// &ldsid); CHECK_ERR
		ndim = H5Sget_simple_extent_dims (mdsid, &mdsize, NULL);
		LOG_VOL_ASSERT (ndim == 1);
		// ndim = H5Sget_simple_extent_dims(ldsid, &ldsize, NULL); assert(ndim == 1);
		err = H5Sselect_all (mdsid);
		CHECK_ERR

		// Allocate buffer
		buf = (char *)malloc (sizeof (char) * mdsize);
		// fp->lut.resize(ldsize);

		// Read metadata
		err = H5VLdataset_read (mdp, fp->uvlid, H5T_NATIVE_B8, mdsid, mdsid, fp->dxplid, buf, NULL);
		CHECK_ERR
		// err = H5VLdataset_read(ldp, fp->uvlid, H5T_STD_I64LE, ldsid, ldsid, fp->dxplid,
		// fp->lut.data(), NULL);    CHECK_ERR

		// Close the dataset
		err = H5VLdataset_close (mdp, fp->uvlid, fp->dxplid, NULL);
		CHECK_ERR
		// err = H5VLdataset_close(ldp, fp->uvlid, fp->dxplid, NULL); CHECK_ERR

		/* Debug code to dump metadata read
		{
			int rank;
			char fname[32];

			MPI_Comm_rank(MPI_COMM_WORLD, &rank);

			sprintf(fname,"p%d_rd.txt",rank);
			hexDump(NULL, buf, mdsize, fname);

		}
		*/

		// Parse metadata
		bufp = buf;
		while (bufp < buf + mdsize) {
			entry.did = *((int *)bufp);
			LOG_VOL_ASSERT ((entry.did >= 0) && (entry.did < fp->ndset));
			bufp += sizeof (int);
			ndim = *((int *)bufp);
			LOG_VOL_ASSERT (ndim >= 0);
			bufp += sizeof (int);
			memcpy (entry.start, bufp, ndim * sizeof (MPI_Offset));
			bufp += ndim * sizeof (MPI_Offset);
			memcpy (entry.count, bufp, ndim * sizeof (MPI_Offset));
			bufp += ndim * sizeof (MPI_Offset);
			entry.ldoff = *((MPI_Offset *)bufp);
			bufp += sizeof (MPI_Offset);
			entry.rsize = *((size_t *)bufp);
			bufp += sizeof (size_t);

			fp->idx[entry.did].push_back (entry);
		}
	} else {
		for (i = 0; i < fp->ndset; i++) { fp->idx[i].clear (); }
	}

	fp->idxvalid = true;

	TIMER_STOP (fp, TIMER_FILEI_METAUPDATE);
err_out:;

	// Cleanup
	if (mdsid >= 0) H5Sclose (mdsid);
	if (ldsid >= 0) H5Sclose (ldsid);
	H5VL_log_free (buf);

	return err;
}

herr_t H5VL_log_filei_close (H5VL_log_file_t *fp) {
	herr_t err = 0;
	int mpierr;
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
			err = H5VL_log_nb_flush_write_reqs (fp, H5P_DEFAULT);
			CHECK_ERR
		}

		// Generate metadata table
		err = H5VL_log_filei_metaflush (fp);
		CHECK_ERR

		// Att
		err = H5VL_logi_put_att (fp, "_ndset", H5T_NATIVE_INT32, &(fp->ndset), H5P_DEFAULT);
		CHECK_ERR
	}

	// Close log group
	TIMER_START
	err = H5VLgroup_close (fp->lgp, fp->uvlid, H5P_DEFAULT, NULL);
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
	err = H5VLfile_close (fp->uo, fp->uvlid, H5P_DEFAULT, NULL);
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

void H5VL_log_filei_inc_ref (H5VL_log_file_t *fp) { fp->refcnt++; }

herr_t H5VL_log_filei_dec_ref (H5VL_log_file_t *fp) {
	fp->refcnt--;
	//if (fp->refcnt == 0) { return H5VL_log_filei_close (fp); }
	return 0;
}