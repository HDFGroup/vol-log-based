#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <mpi.h>

#include <array>
#include <cstring>
#include <vector>

#include "H5VL_log_file.hpp"
#include "H5VL_log_filei.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_err.hpp"
#include "H5VL_logi_wrapper.hpp"
#include "H5VL_logi_zip.hpp"

herr_t H5VL_log_filei_metaflush (H5VL_log_file_t *fp) {
	herr_t err = 0;
	int mpierr;
	int i, j;
	int *cnt, *flag;
	MPI_Offset *mlens = NULL;
	MPI_Offset *moffs = NULL;
	MPI_Offset doff;	// Local metadata offset within the metadata dataset
	MPI_Offset mdsize_all;	// Global metadata size
	MPI_Offset mdsize;
	MPI_Aint *offs	  = NULL;
	int *lens		  = NULL;
#ifdef ENABLE_ZLIB
	MPI_Offset max_mlen;
	char *zbuf;
#endif
	char *buf	= NULL;
	char **bufp = NULL;
	char dname[32];
	int clen, inlen;
	H5VL_loc_params_t loc;
	void *mdp;
	hid_t mdsid = -1;
	hsize_t dsize;
	MPI_Offset seloff, soff, eoff;
	haddr_t mdoff;	// File offset of the metadata dataset
	MPI_Datatype mmtype = MPI_DATATYPE_NULL;
	MPI_Status stat;
	std::vector<std::array<MPI_Offset, H5S_MAX_RANK>> dsteps (fp->ndset);

	TIMER_START;

	TIMER_START;
	// Calculate size and offset of the metadata per dataset
	offs	  = (MPI_Aint *)malloc (sizeof (MPI_Aint) * fp->ndset);
	lens	  = (int *)malloc (sizeof (int) * fp->ndset);
	mlens	  = (MPI_Offset *)malloc (sizeof (MPI_Offset) * fp->ndset);
	moffs	  = (MPI_Offset *)malloc (sizeof (MPI_Offset) * (fp->ndset + 1));
	cnt		  = (int *)malloc (sizeof (int) * fp->ndset * 2);
	memset (cnt, 0, sizeof (int) * fp->ndset * 2);
	flag = cnt + fp->ndset;
	memset (mlens, 0, sizeof (MPI_Offset) * fp->ndset);
	// if (fp->rank == 0) { mlens[0] += sizeof (int) * fp->ndset; }
	for (i = 0; i < fp->ndset; i++) {
		dsteps[i][fp->ndim[i] - 1] = 1;
		for (j = fp->ndim[i] - 2; j > -1; j--) {
			dsteps[i][j] = dsteps[i][j + 1] * fp->dsizes[i][j + 1];
		}
	}
	for (auto &rp : fp->wreqs) { cnt[rp.did] += rp.sels.size (); }
	for (i = 0; i < fp->ndset; i++) {
		mlens[i] += sizeof (int) * 2;  // ID and flag
		if (cnt[i] > 0) {			   // Multiple selection
			flag[i] |= H5VL_FILEI_CONFIG_METADATA_MERGE;
			mlens[i] += sizeof (int);  // nsel
			if (fp->ndim[i] > 1) {
				flag[i] |= H5VL_FILEI_CONFIG_METADATA_ENCODE;
				mlens[i] += fp->ndim[i] * sizeof (MPI_Offset);	// dstep
				mlens[i] += (sizeof (MPI_Offset) * 3 + sizeof (size_t)) * (size_t)cnt[i];
			} else {
				mlens[i] += (sizeof (MPI_Offset) * fp->ndim[i] * 2 + sizeof (MPI_Offset) +
							 sizeof (size_t)) *
							(size_t)cnt[i];
			}
#ifdef ENABLE_ZLIB
			if (cnt[i] > 0) {  // Compress if we have more than 1k entry
				flag[i] |= H5VL_FILEI_CONFIG_METADATA_ZIP;
			}
#endif
		}
	}
#ifdef ENABLE_ZLIB
	max_mlen = 0;
#endif
	moffs[0] = 0;
	for (i = 0; i < fp->ndset; i++) {
		moffs[i + 1] = moffs[i] + mlens[i];
#ifdef ENABLE_ZLIB
		if (max_mlen < mlens[i]) { max_mlen = mlens[i]; }
#endif
	}
	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_INIT);

	TIMER_START;
	// Pack data
#ifdef ENABLE_ZLIB
	buf	 = (char *)malloc (sizeof (char) * (moffs[fp->ndset] + max_mlen));
	zbuf = buf + moffs[fp->ndset];
#else
	buf	   = (char *)malloc (sizeof (char) * moffs[fp->ndset]);
#endif
	bufp = (char **)malloc (sizeof (char *) * fp->ndset);
	for (i = 0; i < fp->ndset; i++) {
		bufp[i] = buf + moffs[i];

		// ID
		*((int *)bufp[i]) = i;
		bufp[i] += sizeof (int);
		// Flag
		*((int *)bufp[i]) = i;
		bufp[i] += sizeof (int);
		// Nsel
		if (flag[i] & H5VL_FILEI_CONFIG_METADATA_MERGE) {
			*((int *)bufp[i]) = cnt[i];
			bufp[i] += sizeof (int);
		}
		// Dstep
		if (flag[i] & H5VL_FILEI_CONFIG_METADATA_ENCODE) {
			memcpy (bufp[i], dsteps[i].data (), sizeof (MPI_Offset) * fp->ndim[i]);
			bufp[i] += sizeof (MPI_Offset) * fp->ndim[i];
		}
	}
	for (auto &rp : fp->wreqs) {
		seloff = 0;
		for (auto &sp : rp.sels) {
			//*((int *)bufp[rp.did]) = rp.did;
			// bufp[rp.did] += sizeof (int);
			if (flag[rp.did] & H5VL_FILEI_CONFIG_METADATA_ENCODE) {
				soff = eoff = 0;
				for (i = 0; i < fp->ndim[rp.did]; i++) {
					soff += sp.start[i] * dsteps[rp.did][i];
					eoff += (sp.start[i] + sp.count[i]) * dsteps[rp.did][i];
				}
				*((MPI_Offset *)bufp[rp.did]) = soff;
				bufp[rp.did] += sizeof (MPI_Offset);
				*((MPI_Offset *)bufp[rp.did]) = eoff;
				bufp[rp.did] += sizeof (MPI_Offset);
			} else {
				memcpy (bufp[rp.did], sp.start, sizeof (MPI_Offset) * fp->ndim[rp.did]);
				bufp[rp.did] += sizeof (MPI_Offset) * fp->ndim[rp.did];
				memcpy (bufp[rp.did], sp.count, sizeof (MPI_Offset) * fp->ndim[rp.did]);
				bufp[rp.did] += sizeof (MPI_Offset) * fp->ndim[rp.did];
			}
			*((MPI_Offset *)bufp[rp.did]) = rp.ldoff + seloff;
			bufp[rp.did] += sizeof (MPI_Offset);
			*((size_t *)bufp[rp.did]) = rp.rsize;
			bufp[rp.did] += sizeof (size_t);
			seloff += sp.size;
		}
	}
	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_PACK);
#ifdef LOGVOL_PROFILING
	H5VL_log_profile_add_time (fp, TIMER_FILEI_METAFLUSH_SIZE,
							   (double)(moffs[fp->ndset]) / 1048576);
#endif

#ifdef ENABLE_ZLIB
	TIMER_START;
	mdsize = 0;
	for (i = 0; i < fp->ndset; i++) {
		if (flag[i] & H5VL_FILEI_CONFIG_METADATA_ZIP) {
			inlen = mlens[i] - sizeof (int) * 3;
			clen  = max_mlen;
			err	  = H5VL_log_zip_compress (buf + moffs[i] + sizeof (int) * 3, inlen, zbuf, &clen);
			if ((err == 0) && (clen < inlen)) {
				memcpy (buf + moffs[i] + sizeof (int) * 3, zbuf, clen);
				mlens[i] = sizeof (int) * 3 + clen;
			} else {
				// Compressed size larger, abort compression
				flag[i] ^= H5VL_FILEI_CONFIG_METADATA_ZIP;
				*((int *)(buf + moffs[i] + sizeof (int))) = flag[i];
			}
		}
		// Recalculate metadata size after comrpession
		mdsize += mlens[i];
	}
	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_ZIP);
#ifdef LOGVOL_PROFILING
		H5VL_log_profile_add_time (fp, TIMER_FILEI_METAFLUSH_SIZE_ZIP, (double)(mdsize) / 1048576);
#endif
#endif

	// Create memory datatype
	TIMER_START;
#ifdef ENABLE_ZLIB
	// moffs will be reused as file offset, create memory type first
	for (i = 0; i < fp->ndset; i++) {
		offs[i] = (MPI_Aint)moffs[i];
		lens[i] = (int)mlens[i];
	}
	mpierr = MPI_Type_hindexed (fp->ndset, lens, offs, MPI_BYTE, &mmtype);
	CHECK_MPIERR
#else
	mpierr = MPI_Type_contiguous ((int)moffs[fp->ndset], MPI_BYTE, &mmtype);
	CHECK_MPIERR
#endif
	mpierr = MPI_Type_commit (&mmtype);
	CHECK_MPIERR
	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_WRITE);  // Part of writing

	// Sync metadata size
	TIMER_START;
	mpierr = MPI_Allreduce (&mdsize, &mdsize_all, 1, MPI_LONG_LONG, MPI_SUM, fp->comm);
	CHECK_MPIERR
	// NOTE: Some MPI implementation do not produce output for rank 0, moffs must ne initialized to
	// 0
	doff=0;
	mpierr = MPI_Exscan (&mdsize, &doff, 1, MPI_LONG_LONG, MPI_SUM, fp->comm);
	CHECK_MPIERR
	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_SYNC);

	dsize = (hsize_t)mdsize_all;
	if (dsize > 0) {
		// Create metadata dataset
		TIMER_START;
		mdsid = H5Screate_simple (1, &dsize, &dsize);
		CHECK_ID (mdsid)
		sprintf (dname, "_md_%d", fp->nmdset);
		loc.obj_type = H5I_GROUP;
		loc.type	 = H5VL_OBJECT_BY_SELF;
		TIMER_START;
		mdp = H5VLdataset_create (fp->lgp, &loc, fp->uvlid, dname, H5P_LINK_CREATE_DEFAULT,
								  H5T_STD_B8LE, mdsid, H5P_DATASET_CREATE_DEFAULT,
								  H5P_DATASET_ACCESS_DEFAULT, fp->dxplid, NULL);
		CHECK_NERR (mdp);
		fp->nmdset++;
		TIMER_STOP (fp, TIMER_H5VL_DATASET_CREATE);

		// Get metadata dataset file offset
		TIMER_START;
		err = H5VL_logi_dataset_optional_wrapper (mdp, fp->uvlid, H5VL_NATIVE_DATASET_GET_OFFSET,
												  fp->dxplid, NULL, &mdoff);
		CHECK_ERR
		TIMER_STOP (fp, TIMER_H5VL_DATASET_OPTIONAL);
		TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_CREATE);

		// Close the metadata dataset
		TIMER_START;
		TIMER_START;
		err = H5VLdataset_close (mdp, fp->uvlid, fp->dxplid, NULL);
		CHECK_ERR
		TIMER_STOP (fp, TIMER_H5VL_DATASET_CLOSE);
		TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_CLOSE);

		// Write metadata
		TIMER_START;  // TIMER_FILEI_METAFLUSH_WRITE
		err = MPI_File_write_at_all (fp->fh, mdoff + doff, buf, 1, mmtype, &stat);
		CHECK_MPIERR
		TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_WRITE);

		TIMER_START;
		// This barrier is required to ensure no process read metadata before everyone finishes
		// writing
		MPI_Barrier (MPI_COMM_WORLD);
		TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_BARR);

		// Update status
		fp->idxvalid = false;
		fp->metadirty = false;
	}

	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH);
err_out:
	// Cleanup
	H5VL_log_free (offs);
	H5VL_log_free (lens);
	H5VL_log_free (mlens);
	H5VL_log_free (moffs);
	H5VL_log_free (buf);
	H5VL_log_free (bufp);
	H5VL_log_free (mlens);
	H5VL_log_Sclose (mdsid);
	if (mmtype != MPI_DATATYPE_NULL) { MPI_Type_free (&mmtype); }
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
	err = H5VL_logi_link_specific_wrapper (fp->lgp, &loc, fp->uvlid, H5VL_LINK_EXISTS, fp->dxplid,
										   NULL, &mdexist);
	CHECK_ERR

	if (mdexist > 0) {
		mdp = H5VLdataset_open (fp->lgp, &loc, fp->uvlid, "_idx", H5P_DATASET_ACCESS_DEFAULT,
								fp->dxplid, NULL);
		CHECK_NERR (mdp)
		// ldp = H5VLdataset_open(fp->lgp, &loc, fp->uvlid, "_lookup", H5P_DATASET_ACCESS_DEFAULT,
		// fp->dxplid, NULL); CHECK_NERR(ldp)

		// Get data space and size
		TIMER_START;
		err = H5VL_logi_dataset_get_wrapper (mdp, fp->uvlid, H5VL_DATASET_GET_SPACE, fp->dxplid,
											 NULL, &mdsid);
		CHECK_ERR
		TIMER_STOP (fp, TIMER_H5VL_DATASET_GET);
		// err = H5VL_logi_dataset_get_wrapper(ldp, fp->uvlid, H5VL_DATASET_GET_SPACE, fp->dxplid,
		// NULL, &ldsid); CHECK_ERR
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