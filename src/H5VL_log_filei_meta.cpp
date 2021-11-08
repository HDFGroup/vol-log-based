#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <mpi.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <map>
#include <unordered_map>
#include <vector>

#include "H5VL_log_file.hpp"
#include "H5VL_log_filei.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_err.hpp"
#include "H5VL_logi_wrapper.hpp"
#include "H5VL_logi_zip.hpp"

// A hash function used to hash a pair of any kind
struct hash_pair {
	size_t operator() (const std::pair<void *, size_t> &p) const {
		int i;
		size_t ret = 0;
		size_t *val;
		size_t *end = (size_t *)((char *)(p.first) + p.second - p.second % sizeof (size_t));

		for (val = (size_t *)(p.first); val < end; val++) { ret ^= *val; }

		return ret;
	}
};

struct equal_pair {
	bool operator() (const std::pair<void *, size_t> &a, const std::pair<void *, size_t> &b) const {
		if (a.second != b.second) { return false; }
		return memcmp (a.first, b.first, a.second) == 0;
	}
};

herr_t H5VL_log_filei_metaflush (H5VL_log_file_t *fp) {
	herr_t err = 0;
	int mpierr;
	int i, j;
	MPI_Offset
		rbuf[2];  // [Local metadata offset within the metadata dataset, Global metadata size]
	MPI_Offset mdsize  = 0;	 // Local metadata size
	MPI_Offset *mdoffs = NULL;
	MPI_Offset *mdoffs_snd;
	MPI_Aint *offs = NULL;	// Offset in MPI_Type_hindexed
	int *lens	   = NULL;	// Lens in MPI_Type_hindexed
	int nentry	   = 0;		// Number of metadata entries
	size_t bsize   = 0;		// Size of metadata buffer = size of metadata before compression
	size_t esize;			// Size of the current processing metadata entry
#ifdef ENABLE_ZLIB
	MPI_Offset zbsize = 0;	   // Size of zbuf
	char *zbuf		  = NULL;  // Buffer to temporarily sotre compressed data
#endif
	char *ptr;
	char mdname[32];						  // Name of metadata dataset
	void *mdp;								  // under VOL object of the metadata dataset
	hid_t mdsid = -1;						  // metadata dataset data space ID
	hsize_t dsize;							  // Size of the metadata dataset = mdsize_all
	haddr_t mdoff;							  // File offset of the metadata dataset
	MPI_Datatype mmtype = MPI_DATATYPE_NULL;  // Memory datatype for writing the metadata
	MPI_Status stat;						  // Status of MPI I/O
	MPI_Comm ldcomm;						  // Communicator to create data dataset
	H5VL_loc_params_t loc;

	H5VL_LOGI_PROFILING_TIMER_START;

	H5VL_LOGI_PROFILING_TIMER_START;

	// Create memory datatype
	nentry = fp->wreqs.size ();
	if (fp->group_rank == 0) { nentry++; }
	offs = (MPI_Aint *)malloc (sizeof (MPI_Aint) * nentry);
	lens = (int *)malloc (sizeof (int) * nentry);
	if (fp->group_rank == 0) {
		mdoffs = (MPI_Offset *)malloc (sizeof (MPI_Offset) * (fp->group_np + 1) * 3);
		CHECK_PTR (mdoffs)
		mdoffs_snd = mdoffs + fp->group_np + 1;

		offs[0] = (MPI_Aint) (mdoffs);
		lens[0] = (int)(sizeof (MPI_Offset) * (fp->group_np + 1));

		nentry = 1;
		mdsize += lens[0];
	} else {
		nentry = 0;
	}

	// Gather offset and lens
	for (auto &rp : fp->wreqs) {
		offs[nentry] = (MPI_Aint)rp->meta_buf;
		lens[nentry] = (int)rp->hdr->meta_size;
		mdsize += lens[nentry++];
	}

	mpierr = MPI_Type_hindexed (nentry, lens, offs, MPI_BYTE, &mmtype);
	CHECK_MPIERR
	mpierr = MPI_Type_commit (&mmtype);
	CHECK_MPIERR
	H5VL_LOGI_PROFILING_TIMER_STOP (fp,
									TIMER_H5VL_LOG_FILEI_METAFLUSH_PACK);  // Part of writing

	// Sync metadata size
	H5VL_LOGI_PROFILING_TIMER_START;
	// mpierr = MPI_Allreduce (&mdsize, &mdsize_all, 1, MPI_LONG_LONG, MPI_SUM, fp->comm);
	// CHECK_MPIERR
	mpierr =
		MPI_Gather (&mdsize, 1, MPI_LONG_LONG, mdoffs + 1, 1, MPI_LONG_LONG, 0, fp->group_comm);
	CHECK_MPIERR
	if (fp->group_rank == 0) {	// Rank 0 calculate
		mdoffs[0] = 0;
		for (i = 0; i < fp->group_np; i++) { mdoffs[i + 1] += mdoffs[i]; }
		rbuf[1] = mdoffs[fp->group_np];	 // Total size
		// Copy to send array with space
		for (i = 0; i < fp->group_np; i++) { mdoffs_snd[i << 1] = mdoffs[i]; }
		// Fill total size
		for (i = 1; i < fp->group_np * 2; i += 2) { mdoffs_snd[i] = rbuf[1]; }
	}
	mpierr = MPI_Scatter (mdoffs_snd, 2, MPI_LONG_LONG, rbuf, 2, MPI_LONG_LONG, 0, fp->group_comm);
	CHECK_MPIERR
	// Bcast merged into scatter
	// mpierr = MPI_Bcast (&mdsize_all, 1, MPI_LONG_LONG, 0, fp->comm);
	// CHECK_MPIERR

	// The first lens[0] byte is the decomposition map
	if (fp->group_rank == 0) { mdoffs[0] = lens[0] / sizeof (MPI_Offset) - 1; }

	// NOTE: Some MPI implementation do not produce output for rank 0, moffs must ne initialized
	// to 0
	// doff = 0;
	// mpierr = MPI_Exscan (&mdsize, &doff, 1, MPI_LONG_LONG, MPI_SUM, fp->comm);
	// CHECK_MPIERR
	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILEI_METAFLUSH_SYNC);

	// Where to create data dataset, main file or subfile
	loc.type = H5VL_OBJECT_BY_SELF;
	loc.obj_type = H5I_GROUP;

	dsize = (hsize_t)rbuf[1];
	if (dsize > 0) {
		// Create metadata dataset
		H5VL_LOGI_PROFILING_TIMER_START;
		mdsid = H5Screate_simple (1, &dsize, &dsize);
		CHECK_ID (mdsid)
		sprintf (mdname, "_md_%d", fp->nmdset);
		H5VL_LOGI_PROFILING_TIMER_START;
		mdp = H5VLdataset_create (fp->lgp, &loc, fp->uvlid, mdname, H5P_LINK_CREATE_DEFAULT,
								  H5T_STD_B8LE, mdsid, H5P_DATASET_CREATE_DEFAULT,
								  H5P_DATASET_ACCESS_DEFAULT, fp->dxplid, NULL);
		CHECK_PTR (mdp);
		fp->nmdset++;
		H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLDATASET_CREATE);

		// Get metadata dataset file offset
		err = H5VL_logi_dataset_get_foff (fp, mdp, fp->uvlid, fp->dxplid, &mdoff);
		CHECK_ERR
		H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILEI_METAFLUSH_CREATE);

		// Close the metadata dataset
		H5VL_LOGI_PROFILING_TIMER_START;
		H5VL_LOGI_PROFILING_TIMER_START;
		err = H5VLdataset_close (mdp, fp->uvlid, fp->dxplid, NULL);
		CHECK_ERR
		H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLDATASET_CLOSE);
		H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILEI_METAFLUSH_CLOSE);

		// Write metadata
		H5VL_LOGI_PROFILING_TIMER_START;  // TIMER_H5VL_LOG_FILEI_METAFLUSH_WRITE
		err = MPI_File_write_at_all (fp->fh, mdoff + rbuf[0], MPI_BOTTOM, 1, mmtype, &stat);
		CHECK_MPIERR
		H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILEI_METAFLUSH_WRITE);

		H5VL_LOGI_PROFILING_TIMER_START;
		// This barrier is required to ensure no process read metadata before everyone finishes
		// writing
		MPI_Barrier (MPI_COMM_WORLD);
		H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILEI_METAFLUSH_BARRIER);
	}

	// Update status
	fp->idxvalid  = false;
	fp->metadirty = false;

	// Delete requests
	for (auto &rp : fp->wreqs) { delete rp; }
	fp->wreqs.clear ();
	// Recore metadata size
#ifdef LOGVOL_PROFILING
	H5VL_log_profile_add_time (fp, TIMER_H5VL_LOG_FILEI_METASIZE, (double)(fp->mdsize) / 1048576);
#endif
	fp->mdsize = 0;
	// Record dedup hash
	fp->wreq_hash.clear ();

	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILEI_METAFLUSH);
err_out:
	// Cleanup
	H5VL_log_free (offs);
	H5VL_log_free (lens);
	H5VL_log_free (mdoffs);
#ifdef ENABLE_ZLIB
	H5VL_log_free (zbuf);
#endif
	H5VL_log_Sclose (mdsid);
	if (mmtype != MPI_DATATYPE_NULL) { MPI_Type_free (&mmtype); }
	return err;
}

/*
 * Remove all existing index entry in fp
 * Load all metadata in the metadata index of fp
 */
herr_t H5VL_log_filei_metaupdate (H5VL_log_file_t *fp) {
	herr_t err = 0;
	int i;
	H5VL_loc_params_t loc;
	void *mdp	= NULL;	 // Metadata dataset
	hid_t mdsid = -1;	 // Metadata dataset space
	hid_t mmsid = -1;	 // metadata buffer memory space
	hsize_t mdsize;		 // Size of metadata dataset
	hsize_t start, count, one = 1;
	char *buf = NULL;			  // Buffer for raw metadata
	char *bufp;					  // Buffer for raw metadata
	int ndim;					  // metadata dataset dimensions (should be 1)
	MPI_Offset nsec;			  // Number of sections in current metadata dataset
	H5VL_logi_metablock_t block;  // Buffer of decoded metadata entry
	std::map<char *, std::vector<H5VL_logi_metasel_t>> bcache;	// Cache for linked metadata entry
	char mdname[16];

	H5VL_LOGI_PROFILING_TIMER_START;

	// Dataspace for memory buffer
	start = count = INT64_MAX - 1;
	mmsid		  = H5Screate_simple (1, &start, &count);

	// Flush all write requests
	if (fp->metadirty) { H5VL_log_filei_metaflush (fp); }

	// Remove all index entries
	for (i = 0; i < fp->ndset; i++) { fp->idx[i].clear (); }

	// iterate through all metadata datasets
	loc.type	 = H5VL_OBJECT_BY_SELF;
	loc.obj_type = H5I_GROUP;
	for (i = 0; i < fp->nmdset; i++) {
		// Open the metadata dataset
		sprintf (mdname, "_md_%d", i);
		mdp = H5VLdataset_open (fp->lgp, &loc, fp->uvlid, mdname, H5P_DATASET_ACCESS_DEFAULT,
								fp->dxplid, NULL);
		CHECK_PTR (mdp)

		// Get data space and size
		mdsid = H5VL_logi_dataset_get_space (fp, mdp, fp->uvlid, fp->dxplid);
		CHECK_ID (mdsid)
		ndim = H5Sget_simple_extent_dims (mdsid, &mdsize, NULL);
		LOG_VOL_ASSERT (ndim == 1);

		// N sections
		start = 0;
		count = sizeof (MPI_Offset);
		err	  = H5Sselect_hyperslab (mmsid, H5S_SELECT_SET, &start, NULL, &one, &count);
		CHECK_ERR
		err = H5Sselect_hyperslab (mdsid, H5S_SELECT_SET, &start, NULL, &one, &count);
		CHECK_ERR
		err =
			H5VLdataset_read (mdp, fp->uvlid, H5T_NATIVE_B8, mmsid, mdsid, fp->dxplid, &nsec, NULL);
		CHECK_ERR

		// Allocate buffer for raw metadata
		start = sizeof (MPI_Offset) * (nsec + 1);
		count = mdsize - start;
		buf	  = (char *)malloc (sizeof (char) * count);

		// Read metadata
		err = H5Sselect_hyperslab (mdsid, H5S_SELECT_SET, &start, NULL, &one, &count);
		CHECK_ERR
		start = 0;
		err	  = H5Sselect_hyperslab (mmsid, H5S_SELECT_SET, &start, NULL, &one, &count);
		CHECK_ERR
		err = H5VLdataset_read (mdp, fp->uvlid, H5T_NATIVE_B8, mmsid, mdsid, fp->dxplid, buf, NULL);
		CHECK_ERR

		// Close the metadata dataset
		err = H5VLdataset_close (mdp, fp->uvlid, fp->dxplid, NULL);
		CHECK_ERR

		// Parse metadata
		bufp = buf;
		if (fp->config &
			H5VL_FILEI_CONFIG_METADATA_SHARE) {	 // Need to maintina cache if file contains
												 // referenced metadata entries
			while (bufp < buf + count) {
				H5VL_logi_meta_hdr *hdr_tmp = (H5VL_logi_meta_hdr *)bufp;

				// Have to parse all entries for reference purpose
				if (hdr_tmp->flag & H5VL_LOGI_META_FLAG_SEL_REF) {
					MPI_Offset roff = *((MPI_Offset *)hdr_tmp + 1);

					err = H5VL_logi_metaentry_ref_decode (fp->dsets[hdr_tmp->did], bufp, block,
														  bcache);
					CHECK_ERR
				} else {
					err = H5VL_logi_metaentry_decode (fp->dsets[hdr_tmp->did], bufp, block);
					CHECK_ERR

					// Insert to cache
					bcache[bufp] = block.sels;
				}
				bufp += hdr_tmp->meta_size;

				// Insert to the index
				fp->idx[hdr_tmp->did].insert (block);
			}
		} else {
			while (bufp < buf + count) {
				H5VL_logi_meta_hdr *hdr_tmp = (H5VL_logi_meta_hdr *)bufp;

				err = H5VL_logi_metaentry_decode (fp->dsets[hdr_tmp->did], bufp, block);
				CHECK_ERR
				bufp += hdr_tmp->meta_size;

				// Insert to the index
				fp->idx[hdr_tmp->did].insert (block);
			}
		}
	}

	// Mark index as up to date
	fp->idxvalid = true;

	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILEI_METAUPDATE);
err_out:;

	// Cleanup
	if (mdsid >= 0) H5Sclose (mdsid);
	if (mmsid >= 0) H5Sclose (mmsid);
	H5VL_log_free (buf);

	return err;
}

/*
 * Remove all existing index entry in fp
 * Load the metadata starting from sec in md in the metadata index of fp until the metadata
 * buffer size is reached Advance sec to the next unprocessed section. If all section is
 * processed, advance md and set sec to 0 If all metadata datasset is processed, set md to -1
 */
herr_t H5VL_log_filei_metaupdate_part (H5VL_log_file_t *fp, int &md, int &sec) {
	herr_t err = 0;
	int i;
	H5VL_loc_params_t loc;
	void *mdp	= NULL;	 // Metadata dataset
	hid_t mdsid = -1;	 // Metadata dataset space
	hid_t mmsid = -1;	 // metadata buffer memory space
	hsize_t mdsize;		 // Size of metadata dataset
	hsize_t start, count, one = 1;
	char *buf = NULL;			  // Buffer for raw metadata
	char *bufp;					  // Buffer for raw metadata
	int ndim;					  // metadata dataset dimensions (should be 1)
	MPI_Offset nsec;			  // Number of sections in current metadata dataset
	MPI_Offset *offs;			  // Section end offset array
	H5VL_logi_metablock_t block;  // Buffer of decoded metadata entry
	std::map<char *, std::vector<H5VL_logi_metasel_t>> bcache;	// Cache for linked metadata entry
	char mdname[16];

	H5VL_LOGI_PROFILING_TIMER_START;

	// Dataspace for memory buffer
	start = count = INT64_MAX - 1;
	mmsid		  = H5Screate_simple (1, &start, &count);

	// Flush all write requests
	if (fp->metadirty) { H5VL_log_filei_metaflush (fp); }

	// Remove all index entries
	for (i = 0; i < fp->ndset; i++) { fp->idx[i].clear (); }

	// Open the metadata dataset
	sprintf (mdname, "_md_%d", md);
	mdp = H5VLdataset_open (fp->lgp, &loc, fp->uvlid, "_idx", H5P_DATASET_ACCESS_DEFAULT,
							fp->dxplid, NULL);
	CHECK_PTR (mdp)

	// Get data space and size
	mdsid = H5VL_logi_dataset_get_space (fp, mdp, fp->uvlid, fp->dxplid);
	CHECK_ID (mdsid)
	ndim = H5Sget_simple_extent_dims (mdsid, &mdsize, NULL);
	LOG_VOL_ASSERT (ndim == 1);

	// Get number of sections (first 8 bytes)
	start = 0;
	count = sizeof (MPI_Offset);
	err	  = H5Sselect_hyperslab (mmsid, H5S_SELECT_SET, &start, NULL, &one, &count);
	CHECK_ERR
	err = H5Sselect_hyperslab (mdsid, H5S_SELECT_SET, &start, NULL, &one, &count);
	CHECK_ERR
	err = H5VLdataset_read (mdp, fp->uvlid, H5T_NATIVE_B8, mmsid, mdsid, fp->dxplid, &nsec, NULL);
	CHECK_ERR

	// Get the ending offset of each section (next 8 * nsec bytes)
	count = sizeof (MPI_Offset) * nsec;
	offs  = (MPI_Offset *)malloc (count);
	err	  = H5Sselect_hyperslab (mmsid, H5S_SELECT_SET, &start, NULL, &one, &count);
	CHECK_ERR
	start = sizeof (MPI_Offset);
	err	  = H5Sselect_hyperslab (mdsid, H5S_SELECT_SET, &start, NULL, &one, &count);
	CHECK_ERR
	err = H5VLdataset_read (mdp, fp->uvlid, H5T_NATIVE_B8, mmsid, mdsid, fp->dxplid, offs, NULL);
	CHECK_ERR

	// Determine #sec to fit
	if (sec >= nsec) { RET_ERR ("Invalid section") }
	if (sec == 0) {	 // First section always starts after the sections offset array
		start = sizeof (MPI_Offset) * (nsec + 1);
	} else {
		start = offs[sec - 1];
	}
	for (i = sec + 1; i < nsec; i++) {
		if (offs[i] - start > fp->mbuf_size) { break; }
	}
	if (i <= sec) { RET_ERR ("OOM") }  // At least 1 section should fit into buffer limit
	count = offs[i - 1] - start;

	// Advance sec and md
	sec = i;
	if (sec >= nsec) {
		sec = 0;
		md++;
	}
	if (md > fp->nmdset) { md = -1; }

	// Allocate buffer for raw metadata
	buf = (char *)malloc (sizeof (char) * count);

	// Read metadata
	err = H5Sselect_hyperslab (mdsid, H5S_SELECT_SET, &start, NULL, &one, &count);
	CHECK_ERR
	start = 0;
	err	  = H5Sselect_hyperslab (mmsid, H5S_SELECT_SET, &start, NULL, &one, &count);
	CHECK_ERR
	err = H5VLdataset_read (mdp, fp->uvlid, H5T_NATIVE_B8, mmsid, mdsid, fp->dxplid, buf, NULL);
	CHECK_ERR

	// Close the metadata dataset
	err = H5VLdataset_close (mdp, fp->uvlid, fp->dxplid, NULL);
	CHECK_ERR

	// Parse metadata
	bufp = buf;
	if (fp->config & H5VL_FILEI_CONFIG_METADATA_SHARE) {  // Need to maintina cache if file contains
														  // referenced metadata entries
		while (bufp < buf + mdsize) {
			H5VL_logi_meta_hdr *hdr_tmp = (H5VL_logi_meta_hdr *)bufp;

			// Have to parse all entries for reference purpose
			if (hdr_tmp->flag & H5VL_LOGI_META_FLAG_SEL_REF) {
				MPI_Offset roff = *((MPI_Offset *)hdr_tmp + 1);

				err = H5VL_logi_metaentry_ref_decode (fp->dsets[hdr_tmp->did], bufp, block, bcache);
				CHECK_ERR
			} else {
				err = H5VL_logi_metaentry_decode (fp->dsets[hdr_tmp->did], bufp, block);
				CHECK_ERR

				// Insert to cache
				bcache[bufp] = block.sels;
			}
			bufp += hdr_tmp->meta_size;

			// Insert to the index
			fp->idx[hdr_tmp->did].insert (block);
		}
	} else {
		while (bufp < buf + mdsize) {
			H5VL_logi_meta_hdr *hdr_tmp = (H5VL_logi_meta_hdr *)bufp;

			err = H5VL_logi_metaentry_decode (fp->dsets[hdr_tmp->did], bufp, block);
			CHECK_ERR
			bufp += hdr_tmp->meta_size;

			// Insert to the index
			fp->idx[hdr_tmp->did].insert (block);
		}
	}

	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILEI_METAUPDATE);
err_out:;

	// Cleanup
	if (mdsid >= 0) H5Sclose (mdsid);
	if (mmsid >= 0) H5Sclose (mmsid);
	H5VL_log_free (buf);

	return err;
}
