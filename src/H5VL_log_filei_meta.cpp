#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <mpi.h>

#include <array>
#include <cstdlib>
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
	int *cnt, *flag;	   // Number of entry to merge per dataset; flag of merged entry per dataset
	int merge_threshould;  // A trick to merge entry with 1 selection
	MPI_Offset *mlens = NULL;  // metadata size of merged entry per dataset
	MPI_Offset *moffs = NULL;  // metadata offset in memory of merged entry per dataset
	MPI_Offset doff;		   // Local metadata offset within the metadata dataset
	MPI_Offset mdsize_all;	   // Global metadata size
	MPI_Offset mdsize = 0;	   // Local metadata size
	MPI_Aint *offs	  = NULL;  // Offset in MPI_Type_hindexed
	int *lens		  = NULL;  // Lens in MPI_Type_hindexed
	int nentry		  = 0;	   // Number of metadata entries
	size_t bsize	  = 0;	   // Size of metadata buffer = size of metadata before compression
	size_t esize;			   // Size of the current processing metadata entry
#ifdef ENABLE_ZLIB
	MPI_Offset zbsize = 0;	// Size of zbuf
	char *zbuf;				// Buffer to temporarily sotre compressed data
#endif
	char *buf	= NULL;				// Buffer to store merged entries
	char **bufp = NULL;				// Point to merged entry per dataset in buf
	char mdname[32];				// Name of metadata dataset
	int clen, inlen;				// Compressed size; Size of data to be compressed
	void *mdp;						// under VOL object of the metadata dataset
	hid_t mdsid = -1;				// metadata dataset data space ID
	hsize_t dsize;					// Size of the metadata dataset = mdsize_all
	MPI_Offset seloff, soff, eoff;	// Temp variable for encoding start and end
	haddr_t mdoff;					// File offset of the metadata dataset
	MPI_Datatype mmtype = MPI_DATATYPE_NULL;
	MPI_Status stat;
	std::vector<std::array<MPI_Offset, H5S_MAX_RANK>> dsteps (fp->ndset);

	TIMER_START;

	TIMER_START;

	mlens = (MPI_Offset *)malloc (sizeof (MPI_Offset) * (fp->ndset * 2 + 1));
	memset (mlens, 0, sizeof (MPI_Offset) * fp->ndset);
	moffs = mlens + fp->ndset;
	cnt	  = (int *)malloc (sizeof (int) * fp->ndset * 2);
	memset (cnt, 0, sizeof (int) * fp->ndset);
	flag = cnt + fp->ndset;

	// Merge entries with 1 selection of a dataset into a single entry with multiple selection
	if (fp->config & H5VL_FILEI_CONFIG_METADATA_MERGE) {
		// Calculate the information to encode and decode selection
		if (fp->config & H5VL_FILEI_CONFIG_SEL_ENCODE) {
			for (i = 0; i < fp->ndset; i++) {
				dsteps[i][fp->ndim[i] - 1] = 1;
				for (j = fp->ndim[i] - 2; j > -1; j--) {
					dsteps[i][j] = dsteps[i][j + 1] * fp->dsizes[i][j + 1];
				}
			}
		}
		// Calculate size and offset of the metadata per dataset
		merge_threshould = 2;  // merge entries < 2 selection
	} else {
		merge_threshould = -1;	// don't merge any
	}

	for (auto &rp : fp->wreqs) {
		if (rp.nsel < merge_threshould) {  // Don't merge varn entries
			cnt[rp.hdr.did]++;
		} else {
			char *ptr;
#ifdef ENABLE_ZLIB
			if (zbsize < rp.hdr.meta_size) { zbsize = rp.hdr.meta_size; }
#endif
			mdsize += rp.hdr.meta_size;
			nentry++;

			ptr					 = rp.meta_buf + sizeof (H5VL_logi_meta_hdr);
			*((MPI_Offset *)ptr) = rp.ldoff;
			ptr += sizeof (MPI_Offset);
			*((MPI_Offset *)ptr) = rp.rsize;
			ptr += sizeof (MPI_Offset);
		}
	}

	// Count the size of merged entries
	if (fp->config & H5VL_FILEI_CONFIG_METADATA_MERGE) {
		for (i = 0; i < fp->ndset; i++) {
			if (cnt[i] > 0) {							  // Have merged entry
				mlens[i] += sizeof (H5VL_logi_meta_hdr);  // Size, ID and flag
				// Size of selection list
				// nsel field only present when there are more than 1 hyperslab
				if (cnt[i] > 1) { mlens[i] += sizeof (int); }
				// Selection list
				if (fp->config & H5VL_FILEI_CONFIG_SEL_ENCODE) {
					mlens[i] += fp->ndim[i] * sizeof (MPI_Offset);	// dstep
					mlens[i] += (sizeof (MPI_Offset) * 3 + sizeof (size_t)) * (size_t)cnt[i];
				} else {
					mlens[i] += (sizeof (MPI_Offset) * fp->ndim[i] * 2 + sizeof (MPI_Offset) +
								 sizeof (size_t)) *
								(size_t)cnt[i];
				}
			}
			mdsize += mlens[i];
		}
	}

	// Buf offset for each merged entry
	moffs[0] = 0;
	for (i = 0; i < fp->ndset; i++) {
		moffs[i + 1] = moffs[i] + mlens[i];
#ifdef ENABLE_ZLIB
		if (zbsize < mlens[i]) { zbsize = mlens[i]; }
#endif
	}

	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_INIT);

#ifdef LOGVOL_PROFILING
	H5VL_log_profile_add_time (fp, TIMER_FILEI_METAFLUSH_SIZE, (double)(mdsize) / 1048576);
#endif

	// Pack data
	TIMER_START;
#ifdef ENABLE_ZLIB
	buf	 = (char *)malloc (moffs[fp->ndset] + zbsize);
	zbuf = buf + moffs[fp->ndset];
#else
	buf = (char *)malloc (moffs[fp->ndset]);
#endif
	bufp = (char **)malloc (sizeof (char *) * fp->ndset * 2);

	// Pack data for the merged requests
	if (fp->config & H5VL_FILEI_CONFIG_METADATA_MERGE) {
		nentry += fp->ndset;
		// Header for merged entries
		for (i = 0; i < fp->ndset; i++) {
			bufp[i] = buf + moffs[i] + sizeof (H5VL_logi_meta_hdr);	 // Skip the header for now
			flag[i] = 0;
			// Fill the header
			// Don't generate merged entry if there is none
			if (cnt[i] > 0) {
				if ((fp->ndim[i] > 1) && (fp->config & H5VL_FILEI_CONFIG_SEL_ENCODE)) {
					flag[i] |= H5VL_LOGI_META_FLAG_SEL_ENCODE;
				}
				if (cnt[i] > 1) {
					flag[i] |= H5VL_LOGI_META_FLAG_MUL_SELX;

					if (fp->config & H5VL_FILEI_CONFIG_SEL_DEFLATE) {
						flag[i] |= H5VL_LOGI_META_FLAG_SEL_DEFLATE;
					}

					// Number of entries
					*((int *)bufp[i]) = cnt[i];
					bufp[i] += sizeof (int);
				}
			}
			bufp[i + fp->ndset] = bufp[i] + sizeof (MPI_Offset) * cnt[i] * 2;
		}

		// Pack selection
		for (auto &rp : fp->wreqs) {
			if (rp.nsel < merge_threshould) {  // Part of merged entries
				char *ptr;

				ptr = bufp[rp.hdr.did];
				bufp[rp.hdr.did] += sizeof (MPI_Offset);

				// File offset and size
				*((MPI_Offset *)ptr) = rp.ldoff;
				ptr += sizeof (MPI_Offset) * cnt[rp.hdr.did];
				*((size_t *)ptr) = rp.rsize;
				ptr += sizeof (size_t) * cnt[rp.hdr.did];

				// Start and count
				if (fp->config & H5VL_FILEI_CONFIG_SEL_ENCODE) {
					MPI_Offset *start;
					MPI_Offset *count;

					start = (MPI_Offset *)(rp.meta_buf + sizeof (H5VL_logi_meta_hdr) +
										   sizeof (MPI_Offset) * 2);
					count = start + fp->ndim[rp.hdr.did];
					soff = eoff = 0;
					for (i = 0; i < fp->ndim[rp.hdr.did]; i++) {
						soff += start[i] *
								dsteps[rp.hdr.did][i];	// Starting offset of the bounding box
						eoff += (count[i]) *
								dsteps[rp.hdr.did][i];	// Ending offset of the bounding box
					}
					ptr = bufp[rp.hdr.did + fp->ndset];
					bufp[rp.hdr.did + fp->ndset] += sizeof (MPI_Offset);
					*((MPI_Offset *)ptr) = soff;
					ptr += sizeof (MPI_Offset) * cnt[rp.hdr.did];
					*((MPI_Offset *)ptr) = eoff;
					ptr += sizeof (MPI_Offset) * cnt[rp.hdr.did];
				} else {
					ptr = bufp[rp.hdr.did + fp->ndset];
					bufp[rp.hdr.did + fp->ndset] += sizeof (MPI_Offset) * cnt[rp.hdr.did];
					memcpy (ptr,
							rp.meta_buf + sizeof (H5VL_logi_meta_hdr) + sizeof (MPI_Offset) * 2,
							sizeof (MPI_Offset) * fp->ndim[rp.hdr.did]);
					ptr += sizeof (MPI_Offset) * fp->ndim[rp.hdr.did] * cnt[rp.hdr.did];
					memcpy (ptr,
							rp.meta_buf + sizeof (H5VL_logi_meta_hdr) + sizeof (MPI_Offset) * 2,
							sizeof (MPI_Offset) * fp->ndim[rp.hdr.did]);
				}
			}
		}
	}

	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_PACK);

#ifdef ENABLE_ZLIB
	TIMER_START;
	// Recount mdsize after compression
	mdsize = 0;
	// Compress merged entries
	if (fp->config & H5VL_FILEI_CONFIG_METADATA_MERGE) {
		for (i = 0; i < fp->ndset; i++) {
			if (flag[i] & H5VL_LOGI_META_FLAG_SEL_DEFLATE) {
				inlen = mlens[i] - sizeof (H5VL_logi_meta_hdr);
				clen  = zbsize;
				err	  = H5VL_log_zip_compress (buf + moffs[i] + sizeof (H5VL_logi_meta_hdr), inlen,
											   zbuf, &clen);
				if ((err == 0) && (clen < inlen)) {
					memcpy (buf + moffs[i] + sizeof (H5VL_logi_meta_hdr), zbuf, clen);
					mlens[i] = sizeof (H5VL_logi_meta_hdr) + clen;
				} else {
					// Compressed size larger, abort compression
					flag[i] &= ~(H5VL_FILEI_CONFIG_SEL_DEFLATE);
				}
			}
			// Recalculate metadata size after comrpession
			mdsize += mlens[i];
		}
	}

	// Compress standalone varn entries
	for (auto &rp : fp->wreqs) {
		if (rp.nsel >= merge_threshould) {
			if (rp.hdr.flag & H5VL_LOGI_META_FLAG_SEL_DEFLATE) {
				inlen = rp.hdr.meta_size - sizeof (H5VL_logi_meta_hdr);
				clen  = zbsize;
				err = H5VL_log_zip_compress (rp.meta_buf + sizeof (H5VL_logi_meta_hdr), inlen, zbuf,
											 &clen);
				if ((err == 0) && (clen < inlen)) {
					memcpy (rp.meta_buf + sizeof (H5VL_logi_meta_hdr), zbuf, clen);
					rp.hdr.meta_size = sizeof (H5VL_logi_meta_hdr) + clen;
				} else {
					// Compressed size larger, abort compression
					rp.hdr.flag &= ~(H5VL_FILEI_CONFIG_SEL_DEFLATE);
				}
				mdsize += rp.hdr.meta_size;
			}
		}
	}
	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_ZIP);
#ifdef LOGVOL_PROFILING
	H5VL_log_profile_add_time (fp, TIMER_FILEI_METAFLUSH_SIZE_ZIP, (double)(mdsize) / 1048576);
#endif
#endif

	// Write entry header later after compression
	TIMER_START;
	// Header for the merged requests
	if (fp->config & H5VL_FILEI_CONFIG_METADATA_MERGE) {
		// Header for merged entries
		for (i = 0; i < fp->ndset; i++) {
			// Don't generate merged entry if there is none
			if (cnt[i] > 0) {
				*((H5VL_logi_meta_hdr *)(buf + moffs[i])) = {(int)mlens[i], i, flag[i]};
			}
		}
	}
	// Header for standalone varn requests
	for (auto &rp : fp->wreqs) {
		if (rp.nsel >= merge_threshould) {	// Part of merged entries
			*((H5VL_logi_meta_hdr *)rp.meta_buf) = rp.hdr;
		}
	}

	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_PACK);

	// Write metadata to file
	TIMER_START;
	// Create memory datatype
	offs = (MPI_Aint *)malloc (sizeof (MPI_Aint) * nentry);
	lens = (int *)malloc (sizeof (int) * nentry);
	if (fp->config & H5VL_FILEI_CONFIG_METADATA_MERGE) {
		// moffs will be reused as file offset, create memory type first
		for (i = 0; i < fp->ndset; i++) {
			offs[i] = (MPI_Aint) (moffs[i] + (size_t)buf);
			lens[i] = (int)mlens[i];
		}
		nentry = fp->ndset;
	} else {
		nentry = 0;
	}
	// Standalone varn requests
	for (auto &rp : fp->wreqs) {
		if (rp.nsel >= merge_threshould) {	// Part of merged entries
			offs[nentry]   = (MPI_Aint)rp.meta_buf;
			lens[nentry++] = (int)rp.hdr.meta_size;
		}
	}

	mpierr = MPI_Type_hindexed (nentry, lens, offs, MPI_BYTE, &mmtype);
	CHECK_MPIERR
	mpierr = MPI_Type_commit (&mmtype);
	CHECK_MPIERR
	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_WRITE);  // Part of writing

	// Sync metadata size
	TIMER_START;
	mpierr = MPI_Allreduce (&mdsize, &mdsize_all, 1, MPI_LONG_LONG, MPI_SUM, fp->comm);
	CHECK_MPIERR
	// NOTE: Some MPI implementation do not produce output for rank 0, moffs must ne initialized
	// to 0
	doff   = 0;
	mpierr = MPI_Exscan (&mdsize, &doff, 1, MPI_LONG_LONG, MPI_SUM, fp->comm);
	CHECK_MPIERR
	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_SYNC);

	dsize = (hsize_t)mdsize_all;
	if (dsize > 0) {
		H5VL_loc_params_t loc;

		// Create metadata dataset
		TIMER_START;
		mdsid = H5Screate_simple (1, &dsize, &dsize);
		CHECK_ID (mdsid)
		sprintf (mdname, "_md_%d", fp->nmdset);
		loc.obj_type = H5I_GROUP;
		loc.type	 = H5VL_OBJECT_BY_SELF;
		TIMER_START;
		mdp = H5VLdataset_create (fp->lgp, &loc, fp->uvlid, mdname, H5P_LINK_CREATE_DEFAULT,
								  H5T_STD_B8LE, mdsid, H5P_DATASET_CREATE_DEFAULT,
								  H5P_DATASET_ACCESS_DEFAULT, fp->dxplid, NULL);
		CHECK_PTR (mdp);
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
		err = MPI_File_write_at_all (fp->fh, mdoff + doff, MPI_BOTTOM, 1, mmtype, &stat);
		CHECK_MPIERR
		TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_WRITE);

		TIMER_START;
		// This barrier is required to ensure no process read metadata before everyone finishes
		// writing
		MPI_Barrier (MPI_COMM_WORLD);
		TIMER_STOP (fp, TIMER_FILEI_METAFLUSH_BARR);

		// Update status
		fp->idxvalid  = false;
		fp->metadirty = false;
	}

	for (auto &rp : fp->wreqs) {
		if (rp.meta_buf) { free (rp.meta_buf); }
	}
	TIMER_STOP (fp, TIMER_FILEI_METAFLUSH);
err_out:
	// Cleanup
	H5VL_log_free (offs);
	H5VL_log_free (lens);
	H5VL_log_free (mlens);
	H5VL_log_free (cnt);
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
		CHECK_PTR (mdp)
		// ldp = H5VLdataset_open(fp->lgp, &loc, fp->uvlid, "_lookup",
		// H5P_DATASET_ACCESS_DEFAULT, fp->dxplid, NULL); CHECK_PTR(ldp)

		// Get data space and size
		TIMER_START;
		err = H5VL_logi_dataset_get_wrapper (mdp, fp->uvlid, H5VL_DATASET_GET_SPACE, fp->dxplid,
											 NULL, &mdsid);
		CHECK_ERR
		TIMER_STOP (fp, TIMER_H5VL_DATASET_GET);
		// err = H5VL_logi_dataset_get_wrapper(ldp, fp->uvlid, H5VL_DATASET_GET_SPACE,
		// fp->dxplid, NULL, &ldsid); CHECK_ERR
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