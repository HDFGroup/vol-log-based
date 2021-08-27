#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <mpi.h>

#include <array>
#include <cstdlib>
#include <cstring>
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
	int *cnt, *flag;  // Number of entry to merge per dataset; flag of merged entry per dataset
	MPI_Offset *mlens = NULL;  // metadata size of merged entry per dataset
	MPI_Offset *moffs = NULL;  // metadata offset in memory of merged entry per dataset
	MPI_Offset doff;		   // Local metadata offset within the metadata dataset
	MPI_Offset mdsize_all;	   // Global metadata size
	MPI_Offset mdsize  = 0;	   // Local metadata size
	MPI_Offset *mdoffs = NULL;
	MPI_Aint *offs	   = NULL;	// Offset in MPI_Type_hindexed
	int *lens		   = NULL;	// Lens in MPI_Type_hindexed
	int nentry		   = 0;		// Number of metadata entries
	size_t bsize	   = 0;		// Size of metadata buffer = size of metadata before compression
	size_t esize;				// Size of the current processing metadata entry
#ifdef ENABLE_ZLIB
	MPI_Offset zbsize = 0;	// Size of zbuf
	char *zbuf;				// Buffer to temporarily sotre compressed data
#endif
	char *buf	= NULL;	 // Buffer to store merged entries
	char **bufp = NULL;	 // Point to merged entry per dataset in buf
	char *ptr;
	char mdname[32];						  // Name of metadata dataset
	int clen, inlen;						  // Compressed size; Size of data to be compressed
	void *mdp;								  // under VOL object of the metadata dataset
	hid_t mdsid = -1;						  // metadata dataset data space ID
	hsize_t dsize;							  // Size of the metadata dataset = mdsize_all
	MPI_Offset seloff, soff, eoff;			  // Temp variable for encoding start and end
	haddr_t mdoff;							  // File offset of the metadata dataset
	MPI_Datatype mmtype = MPI_DATATYPE_NULL;  // Memory datatype for writing the metadata
	MPI_Status stat;
	std::vector<std::array<MPI_Offset, H5S_MAX_RANK>> dsteps (fp->ndset);
	std::unordered_map<std::pair<void *, size_t>, H5VL_log_wreq_t *, hash_pair, equal_pair>
		meta_ref;

	H5VL_LOGI_PROFILING_TIMER_START;

	H5VL_LOGI_PROFILING_TIMER_START;

	// Count the number of metadata blocks
	for (auto &rp : fp->wreqs) {
#ifdef ENABLE_ZLIB
		// metadata compression buffer size
		if (zbsize < rp->hdr.meta_size) { zbsize = rp->hdr.meta_size; }
#endif
		// Calculate total metadata size
		rp->meta_off = mdsize;
		mdsize += rp->hdr.meta_size;
		nentry++;

		// Update file offset and size of the data block unknown when the request was posted
		ptr = rp->meta_buf + sizeof (H5VL_logi_meta_hdr);
		*((MPI_Offset *)ptr) = rp->ldoff;  // file offset
		ptr += sizeof (MPI_Offset);
		*((MPI_Offset *)ptr) = rp->rsize;  // file size
		ptr += sizeof (MPI_Offset);
		if (rp->hdr.flag & H5VL_LOGI_META_FLAG_MUL_SEL) {  // # selections
			*((int *)ptr) = rp->nsel;
			// nsel can be overwritten if transformed into referenced entry, do not avdance the pointer
			// ptr += sizeof (int);
		}
		
		if (fp->config & H5VL_FILEI_CONFIG_METADATA_SHARE) {
			auto t = std::pair<void *, size_t> (
				(void *)ptr,
				rp->hdr.meta_size - sizeof (H5VL_logi_meta_hdr) - sizeof (MPI_Offset) * 2);

			if (meta_ref.find (t) == meta_ref.end ()) {
				meta_ref[t] = rp;
			} else {
				rp->hdr.flag |= H5VL_LOGI_META_FLAG_SEL_REF;
				rp->hdr.flag &= ~(H5VL_LOGI_META_FLAG_SEL_DEFLATE);	 // Remove compression flag
				rp->hdr.meta_size = sizeof (H5VL_logi_meta_hdr) +
									sizeof (MPI_Offset) * 3;  // Recalculate metadata size
				// We write the address of reference targe temporarily into the reference offset
				// It will be replaced once the offset of the reference target is known
				// NOTE: This trick only works when sizeof(H5VL_log_wreq_t*) <= sizeof(MPI_Offset)
				*((H5VL_log_wreq_t **)ptr) = meta_ref[t];
			}
		}
	}

	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILEI_METAFLUSH_INIT);

#ifdef LOGVOL_PROFILING
	H5VL_log_profile_add_time (fp, TIMER_H5VL_LOG_FILEI_METAFLUSH_SIZE, (double)(mdsize) / 1048576);
#endif

#ifdef ENABLE_ZLIB
	H5VL_LOGI_PROFILING_TIMER_START;
	// Recount mdsize after compression
	mdsize = 0;
	// Compress metadata
	for (auto &rp : fp->wreqs) {
		if (rp->hdr.flag & H5VL_LOGI_META_FLAG_SEL_DEFLATE) {
			inlen = rp->hdr.meta_size - sizeof (H5VL_logi_meta_hdr) -  sizeof(MPI_Offset) * 2 - sizeof(int);
			clen  = zbsize;
			err	  = H5VL_log_zip_compress (rp->meta_buf + sizeof (H5VL_logi_meta_hdr) + sizeof(MPI_Offset) * 2 + sizeof(int), inlen, zbuf,
										   &clen);
			if ((err == 0) && (clen < inlen)) {
				memcpy (rp->meta_buf + sizeof (H5VL_logi_meta_hdr) + sizeof(MPI_Offset) * 2 + sizeof(int), zbuf, clen);
				rp->hdr.meta_size = sizeof (H5VL_logi_meta_hdr) + sizeof(MPI_Offset) * 2 + sizeof(int) + clen;
			} else {
				// Compressed size larger, abort compression
				rp->hdr.flag &= ~(H5VL_FILEI_CONFIG_SEL_DEFLATE);
			}
		}
		rp->meta_off = mdsize;
		mdsize += rp->hdr.meta_size;
	}
	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILEI_METAFLUSH_ZIP);
#ifdef LOGVOL_PROFILING
	H5VL_log_profile_add_time (fp, TIMER_H5VL_LOG_FILEI_METAFLUSH_SIZE_ZIP,
							   (double)(mdsize) / 1048576);
#endif
#endif

	H5VL_LOGI_PROFILING_TIMER_START;
	// Fill up all metadata reference once the offset of refered entries is known
	if (fp->config & H5VL_FILEI_CONFIG_METADATA_SHARE) {
		for (auto &rp : fp->wreqs) {
			if (rp->hdr.flag & H5VL_LOGI_META_FLAG_SEL_REF) {
				ptr = rp->meta_buf + sizeof (H5VL_logi_meta_hdr) + sizeof (MPI_Offset) * 2;
				H5VL_log_wreq_t *t = *((H5VL_log_wreq_t **)ptr);
				// Replace with the file offset of the reference metadata entry related to this
				// entry, the result should be negative (looking in previous records)
				*((MPI_Offset *)ptr) = t->meta_off - rp->meta_off;
			}
		}
	}

	// Rewrite entry header later after compression
	// Header for standalone varn requests
	for (auto &rp : fp->wreqs) { *((H5VL_logi_meta_hdr *)rp->meta_buf) = rp->hdr; }

	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILEI_METAFLUSH_PACK);

	// Write metadata to file
	H5VL_LOGI_PROFILING_TIMER_START;

	// Create memory datatype
	if (fp->rank == 0) { nentry++; }
	offs = (MPI_Aint *)malloc (sizeof (MPI_Aint) * nentry);
	lens = (int *)malloc (sizeof (int) * nentry);
	if (fp->rank == 0) {
		mdoffs = (MPI_Offset *)malloc (sizeof (MPI_Offset) * (fp->np + 1));
		CHECK_PTR (mdoffs)

		offs[0] = (MPI_Aint) (mdoffs);
		lens[0] = (int)(sizeof (MPI_Offset) * (fp->np + 1));

		nentry = 1;
		mdsize += lens[0];
	} else {
		nentry = 0;
	}
	if (fp->config & H5VL_FILEI_CONFIG_METADATA_MERGE) {
		// moffs will be reused as file offset, create memory type first
		for (i = 0; i < fp->ndset; i++) {
			offs[nentry]   = (MPI_Aint) (moffs[i] + (size_t)buf);
			lens[nentry++] = (int)mlens[i];
		}
	}
	// Gather offset and lens
	for (auto &rp : fp->wreqs) {
		offs[nentry]   = (MPI_Aint)rp->meta_buf;
		lens[nentry++] = (int)rp->hdr.meta_size;
	}

	mpierr = MPI_Type_hindexed (nentry, lens, offs, MPI_BYTE, &mmtype);
	CHECK_MPIERR
	mpierr = MPI_Type_commit (&mmtype);
	CHECK_MPIERR
	H5VL_LOGI_PROFILING_TIMER_STOP (fp,
									TIMER_H5VL_LOG_FILEI_METAFLUSH_WRITE);	// Part of writing

	// Sync metadata size
	H5VL_LOGI_PROFILING_TIMER_START;
	// mpierr = MPI_Allreduce (&mdsize, &mdsize_all, 1, MPI_LONG_LONG, MPI_SUM, fp->comm);
	// CHECK_MPIERR
	mpierr = MPI_Gather (&mdsize, 1, MPI_LONG_LONG, mdoffs + 1, 1, MPI_LONG_LONG, 0, fp->comm);
	CHECK_MPIERR
	if (fp->rank == 0) {  // Rank 0 calculate
		mdoffs[0] = 0;
		for (i = 0; i < fp->np; i++) { mdoffs[i + 1] += mdoffs[i]; }
	}
	mpierr = MPI_Scatter (mdoffs, 1, MPI_LONG_LONG, &doff, 1, MPI_LONG_LONG, 0, fp->comm);
	CHECK_MPIERR
	mdsize_all = mdoffs[fp->np];
	mpierr	   = MPI_Bcast (&mdsize_all, 1, MPI_LONG_LONG, 0, fp->comm);
	CHECK_MPIERR

	if (fp->rank == 0) { mdoffs[0] = lens[0]; }
	// NOTE: Some MPI implementation do not produce output for rank 0, moffs must ne initialized
	// to 0
	// doff = 0;
	// mpierr = MPI_Exscan (&mdsize, &doff, 1, MPI_LONG_LONG, MPI_SUM, fp->comm);
	// CHECK_MPIERR
	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILEI_METAFLUSH_SYNC);

	dsize = (hsize_t)mdsize_all;
	if (dsize > 0) {
		H5VL_loc_params_t loc;

		// Create metadata dataset
		H5VL_LOGI_PROFILING_TIMER_START;
		mdsid = H5Screate_simple (1, &dsize, &dsize);
		CHECK_ID (mdsid)
		sprintf (mdname, "_md_%d", fp->nmdset);
		loc.obj_type = H5I_GROUP;
		loc.type	 = H5VL_OBJECT_BY_SELF;
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
		err = MPI_File_write_at_all (fp->fh, mdoff + doff, MPI_BOTTOM, 1, mmtype, &stat);
		CHECK_MPIERR
		H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILEI_METAFLUSH_WRITE);

		H5VL_LOGI_PROFILING_TIMER_START;
		// This barrier is required to ensure no process read metadata before everyone finishes
		// writing
		MPI_Barrier (MPI_COMM_WORLD);
		H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILEI_METAFLUSH_BARRIER);

		// Update status
		fp->idxvalid  = false;
		fp->metadirty = false;
	}

	for (auto &rp : fp->wreqs) {
		if (rp->meta_buf) { free (rp->meta_buf); }
		delete rp;
	}
	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILEI_METAFLUSH);
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
	char *buf = NULL;  // Buffer for raw metadata
	char *bufp;		   // Buffer for raw metadata
	int ndim;		   // metadata dataset dimensions (should be 1)
	MPI_Offset nsec;   // Number of sections in current metadata dataset
	H5VL_log_metaentry_t entry;
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
	for (i = 0; i < fp->nmdset; i++) {
		// Open the metadata dataset
		sprintf (mdname, "_md_%d", i);
		mdp = H5VLdataset_open (fp->lgp, &loc, fp->uvlid, "_idx", H5P_DATASET_ACCESS_DEFAULT,
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
		while (bufp < buf + mdsize) {
			H5VL_logi_meta_hdr *hdr = (H5VL_logi_meta_hdr *)bufp;

			err = H5VL_logi_metaentry_decode (fp->dsets[hdr->did], bufp, fp->idx[hdr->did]);
			CHECK_ERR
			bufp += hdr->meta_size;
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
	char *buf = NULL;  // Buffer for raw metadata
	char *bufp;		   // Buffer for raw metadata
	int ndim;		   // metadata dataset dimensions (should be 1)
	MPI_Offset nsec;   // Number of sections in current metadata dataset
	MPI_Offset *offs;  // Section end offset array
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
	while (bufp < buf + mdsize) {
		H5VL_logi_meta_hdr *hdr = (H5VL_logi_meta_hdr *)bufp;

		err = H5VL_logi_metaentry_decode (fp->dsets[hdr->did], bufp, fp->idx[hdr->did]);
		CHECK_ERR
		bufp += hdr->meta_size;
	}

	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILEI_METAUPDATE);
err_out:;

	// Cleanup
	if (mdsid >= 0) H5Sclose (mdsid);
	if (mmsid >= 0) H5Sclose (mmsid);
	H5VL_log_free (buf);

	return err;
}
