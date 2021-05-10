#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//
#include <cstring>
//
#include <sys/types.h>
#include <unistd.h>
//
#include "H5VL_log_dataset.hpp"
#include "H5VL_log_dataseti.hpp"
#include "H5VL_log_filei.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_idx.hpp"
#include "H5VL_logi_meta.hpp"
#include "H5VL_logi_nb.hpp"
#include "H5VL_logi_wrapper.hpp"

#define H5VL_LOGI_MERGED_REQ_SEL_RESERVE 32

H5VL_log_merged_wreq_t::~H5VL_log_merged_wreq_t () {
	for (auto &dbuf : this->dbufs) {
		if (dbuf.first != dbuf.second) { free (dbuf.first); }
	}
}

herr_t H5VL_log_merged_wreq_t::init (H5VL_log_dset_t *dp, int nsel) {
	herr_t err = 0;

	if (nsel < H5VL_LOGI_MERGED_REQ_SEL_RESERVE) { nsel = H5VL_LOGI_MERGED_REQ_SEL_RESERVE; }

	this->hdr.meta_size = sizeof (H5VL_logi_meta_hdr) + sizeof (int) + sizeof (MPI_Offset);

	this->hdr.did = dp->id;

	this->hdr.flag = H5VL_LOGI_META_FLAG_MUL_SEL;
	if ((dp->ndim > 1) && (dp->fp->config & H5VL_FILEI_CONFIG_SEL_ENCODE)) {
		this->hdr.flag |= H5VL_LOGI_META_FLAG_SEL_ENCODE;
		this->hdr.meta_size += sizeof (MPI_Offset) * 2 * nsel;
	} else {
		this->hdr.meta_size += sizeof (MPI_Offset) * dp->ndim * 2 * nsel;
	}

	if (dp->fp->config & H5VL_FILEI_CONFIG_SEL_DEFLATE) {
		this->hdr.flag |= H5VL_LOGI_META_FLAG_SEL_DEFLATE;
	}

	this->mbuf = (char *)malloc (this->hdr.meta_size);
	CHECK_PTR (this->mbuf);
	this->mbufe = this->mbuf + this->hdr.meta_size;
	this->mbufp = this->mbuf + sizeof (H5VL_logi_meta_hdr) + sizeof (int) + sizeof (MPI_Offset) * 2;

	// Pack dsteps
	if (this->hdr.flag & H5VL_FILEI_CONFIG_SEL_ENCODE) {
		memcpy (this->mbufp, dp->dsteps, sizeof (MPI_Offset) * (dp->ndim - 1));
		this->mbufp += sizeof (MPI_Offset) * (dp->ndim - 1);
	}

err_out:;
	return err;
}

herr_t H5VL_log_merged_wreq_t::reserve (size_t size) {
	herr_t err = 0;
	size_t target_size;

	if (this->mbufp + size > this->mbufe) {
		target_size = this->mbufp - this->mbuf + size;
		while (this->hdr.meta_size < target_size) { this->hdr.meta_size <<= 3; }
		target_size = this->mbufp - this->mbuf;	 // reuse target size for used buffer

		this->mbuf = (char *)realloc (this->mbuf, this->hdr.meta_size);
		CHECK_PTR (this->mbuf);
		this->mbufp = this->mbuf + target_size;
		this->mbufe = this->mbuf + this->hdr.meta_size;
	}
err_out:;
	return err;
}

herr_t H5VL_log_merged_wreq_t::append (H5VL_log_dset_t *dp,
									   int nsel,
									   hsize_t **starts,
									   hsize_t **counts) {
	herr_t err = 0;
	size_t msize;

	if (this->hdr.flag & H5VL_FILEI_CONFIG_SEL_ENCODE) {
		msize = nsel * 2 * sizeof (MPI_Offset);
	} else {
		msize = nsel * 2 * sizeof (hsize_t) * dp->ndim;
	}

	this->reserve (msize);

	err = H5VL_logi_metaentry_encode (*dp, this->hdr, nsel, starts, counts, this->mbufp);

	this->mbufp += msize;

err_out:;
	return err;
}

herr_t H5VL_log_merged_wreq_t::append (H5VL_log_dset_t *dp, std::vector<H5VL_log_selection> sels) {
	herr_t err = 0;
	size_t msize;

	if (this->hdr.flag & H5VL_FILEI_CONFIG_SEL_ENCODE) {
		msize = nsel * 2 * sizeof (MPI_Offset);
	} else {
		msize = nsel * 2 * sizeof (hsize_t) * dp->ndim;
	}

	this->reserve (msize);

	err = H5VL_logi_metaentry_encode (*dp, this->hdr, sels, this->mbufp);

	this->mbufp += msize;

err_out:;
	return err;
}

herr_t H5VL_log_nb_flush_read_reqs (void *file, std::vector<H5VL_log_rreq_t> reqs, hid_t dxplid) {
	herr_t err = 0;
	int mpierr;
	int i, j;
	size_t esize;
	MPI_Datatype ftype, mtype;
	std::vector<H5VL_log_search_ret_t> intersections;
	std::vector<H5VL_log_copy_ctx> overlaps;
	MPI_Status stat;
	H5VL_log_file_t *fp = (H5VL_log_file_t *)file;
	H5VL_LOGI_PROFILING_TIMER_START;

	if (!(fp->idxvalid)) {
		err = H5VL_log_filei_metaupdate (fp);
		CHECK_ERR
	}

	for (auto &r : reqs) {
		err = H5VL_logi_idx_search (fp, r, intersections);
		CHECK_ERR
	}

	if (intersections.size () > 0) {
		H5VL_LOGI_PROFILING_TIMER_START;
		err = H5VL_log_dataset_readi_gen_rtypes (intersections, &ftype, &mtype, overlaps);
		CHECK_ERR
		H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_DATASETI_READI_GEN_RTYPES);
		mpierr = MPI_Type_commit (&mtype);
		CHECK_MPIERR
		mpierr = MPI_Type_commit (&ftype);
		CHECK_MPIERR

		mpierr = MPI_File_set_view (fp->fh, 0, MPI_BYTE, ftype, "native", MPI_INFO_NULL);
		CHECK_MPIERR

		mpierr = MPI_File_read_all (fp->fh, MPI_BOTTOM, 1, mtype, &stat);
		CHECK_MPIERR
	} else {
		mpierr =
			MPI_File_set_view (fp->fh, 0, MPI_BYTE, MPI_DATATYPE_NULL, "native", MPI_INFO_NULL);
		CHECK_MPIERR

		mpierr = MPI_File_read_all (fp->fh, MPI_BOTTOM, 0, MPI_DATATYPE_NULL, &stat);
		CHECK_MPIERR
	}

	for (auto &o : overlaps) { memcpy (o.dst, o.src, o.size); }

	// Post processing
	for (auto &r : reqs) {
		// Type convertion
		if (r.dtype != r.mtype) {
			void *bg = NULL;

			esize = H5Tget_size (r.mtype);
			CHECK_ID (esize)

			if (H5Tget_class (r.mtype) == H5T_COMPOUND) bg = malloc (r.rsize * esize);
			err = H5Tconvert (r.dtype, r.mtype, r.rsize, r.xbuf, bg, dxplid);
			CHECK_ERR
			free (bg);

			H5Tclose (r.dtype);
			H5Tclose (r.mtype);
		} else {
			esize = r.esize;
		}

		// Packing
		if (r.xbuf != r.ubuf) {
			if (r.ptype != MPI_DATATYPE_NULL) {
				i = 0;
				MPI_Unpack (r.xbuf, 1, &i, r.ubuf, 1, r.ptype, fp->comm);
				MPI_Type_free (&(r.ptype));
			} else {
				memcpy (r.ubuf, r.xbuf, r.rsize * esize);
			}

			H5VL_log_filei_bfree (fp, r.xbuf);
		}
	}

	reqs.clear ();

	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_NB_FLUSH_READ_REQS);
err_out:;

	return err;
}

herr_t H5VL_log_nb_flush_write_reqs (void *file, hid_t dxplid) {
	herr_t err = 0;
	int mpierr;
	int i;
	int cnt;
	int *mlens		   = NULL;
	MPI_Aint *moffs	   = NULL;
	MPI_Datatype mtype = MPI_DATATYPE_NULL;
	MPI_Status stat;
	MPI_Offset fsize_local, fsize_all, foff;
	void *ldp;
	hid_t ldsid = -1;
	hsize_t dsize;
	haddr_t doff;
	H5VL_loc_params_t loc;
	char dname[16];
	H5VL_log_file_t *fp = (H5VL_log_file_t *)file;

	H5VL_LOGI_PROFILING_TIMER_START;
	H5VL_LOGI_PROFILING_TIMER_START;

	cnt = fp->wreqs.size () - fp->nflushed;

	// Construct memory type
	fsize_local = 0;
	if (cnt) {
		mlens = (int *)malloc (sizeof (int) * cnt);
		moffs = (MPI_Aint *)malloc (sizeof (MPI_Aint) * cnt);
		for (i = fp->nflushed; i < fp->wreqs.size (); i++) {
			moffs[i - fp->nflushed] = (MPI_Aint)fp->wreqs[i].xbuf;
			mlens[i - fp->nflushed] = fp->wreqs[i].rsize;
			fp->wreqs[i].ldoff		= fsize_local;
			fsize_local += fp->wreqs[i].rsize;
		}
		mpierr = MPI_Type_hindexed (cnt, mlens, moffs, MPI_BYTE, &mtype);
		CHECK_MPIERR
		mpierr = MPI_Type_commit (&mtype);
		CHECK_MPIERR
	} else {
		mtype = MPI_DATATYPE_NULL;
	}
#ifdef LOGVOL_PROFILING
	H5VL_log_profile_add_time (fp, TIMER_H5VL_LOG_NB_FLUSH_WRITE_REQS_SIZE,
							   (double)(fsize_local) / 1048576);
#endif

	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_NB_FLUSH_WRITE_REQS_INIT);
	H5VL_LOGI_PROFILING_TIMER_START;

	// Get file offset and total size
	mpierr = MPI_Allreduce (&fsize_local, &fsize_all, 1, MPI_LONG_LONG, MPI_SUM, fp->comm);
	CHECK_MPIERR
	// NOTE: Some MPI implementation do not produce output for rank 0, foff must ne initialized to 0
	foff   = 0;
	mpierr = MPI_Exscan (&fsize_local, &foff, 1, MPI_LONG_LONG, MPI_SUM, fp->comm);
	CHECK_MPIERR

	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_NB_FLUSH_WRITE_REQS_SYNC);

	// Create log dataset
	dsize = (hsize_t)fsize_all;
	if (dsize) {
		H5VL_LOGI_PROFILING_TIMER_START;

		ldsid = H5Screate_simple (1, &dsize, &dsize);
		CHECK_ID (ldsid)
		sprintf (dname, "_ld_%d", fp->nldset);
		loc.obj_type = H5I_GROUP;
		loc.type	 = H5VL_OBJECT_BY_SELF;
		H5VL_LOGI_PROFILING_TIMER_START;
		ldp = H5VLdataset_create (fp->lgp, &loc, fp->uvlid, dname, H5P_LINK_CREATE_DEFAULT,
								  H5T_STD_B8LE, ldsid, H5P_DATASET_CREATE_DEFAULT,
								  H5P_DATASET_ACCESS_DEFAULT, dxplid, NULL);
		CHECK_PTR (ldp);
		H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLDATASET_CREATE);

		H5VL_LOGI_PROFILING_TIMER_START;
		err = H5VL_logi_dataset_optional_wrapper (ldp, fp->uvlid, H5VL_NATIVE_DATASET_GET_OFFSET,
												  dxplid, NULL, &doff);
		CHECK_ERR  // Get dataset file offset
		H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLDATASET_OPTIONAL);

		err = H5VLdataset_close (ldp, fp->uvlid, dxplid, NULL);
		CHECK_ERR  // Close the dataset

		H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_NB_FLUSH_WRITE_REQS_CREATE);
		H5VL_LOGI_PROFILING_TIMER_START;

		// Write the data
		foff += doff;
		if (mtype == MPI_DATATYPE_NULL) {
			mpierr = MPI_File_write_at_all (fp->fh, foff, MPI_BOTTOM, 0, MPI_INT, &stat);
			CHECK_MPIERR
		} else {
			mpierr = MPI_File_write_at_all (fp->fh, foff, MPI_BOTTOM, 1, mtype, &stat);
			CHECK_MPIERR
		}
		H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_NB_FLUSH_WRITE_REQS_WR);

		// Update metadata
		for (i = fp->nflushed; i < fp->wreqs.size (); i++) {
			fp->wreqs[i].ldid = fp->nldset;
			fp->wreqs[i].ldoff += foff;
			if (fp->wreqs[i].xbuf != fp->wreqs[i].ubuf) {
				H5VL_log_filei_bfree (fp, (void *)(fp->wreqs[i].xbuf));
			}
		}
		(fp->nldset)++;
		fp->nflushed = fp->wreqs.size ();

		if (fsize_all) { fp->metadirty = true; }

		// err = H5VL_log_filei_pool_free (&(fp->data_buf));
		// CHECK_ERR
	}

	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_NB_FLUSH_WRITE_REQS);
err_out:
	// Cleanup
	if (mtype != MPI_DATATYPE_NULL) MPI_Type_free (&mtype);
	H5VL_log_Sclose (ldsid);

	H5VL_log_free (mlens);
	H5VL_log_free (moffs);

	return err;
}

inline herr_t H5VL_log_nb_flush_posix_write (int fd, char *buf, size_t count) {
	herr_t err = 0;
	ssize_t wsize;

	while (count > 0) {
		wsize = write (fd, buf, count);
		if (wsize < 0) { ERR_OUT ("write fail") }
		buf += wsize;
		count -= wsize;
	}

err_out:;
	return err;
}

herr_t H5VL_log_nb_ost_write (
	void *file, off64_t doff, off64_t off, int cnt, int *mlens, off64_t *moffs) {
	herr_t err = 0;
	int mpierr;
	int i;
	char *bs = NULL, *be, *bp;
	char *mbuf;
	off64_t ost_base;  // File offset of the first byte on the target ost
	off64_t cur_off;   // Offset of current stripe
	off64_t seek_off;
	size_t skip_size;  // First chunk can be partial
	size_t mlen;
	size_t bused;
	size_t stride;
	H5VL_log_file_t *fp = (H5VL_log_file_t *)file;

	H5VL_LOGI_PROFILING_TIMER_START;

	// Buffer of a stripe
	bs = (char *)malloc (fp->ssize);
	be = bs + fp->ssize;

	stride	 = fp->ssize * fp->scount;	// How many bytes to the next stripe on the same ost
	ost_base = doff + fp->ssize * fp->target_ost;  // Skip fp->target_ost stripes to get to the
												   // first byte of the target ost

	cur_off	  = ost_base + (off / fp->ssize) * stride;
	skip_size = off % fp->ssize;
	seek_off  = lseek64 (fp->fd, cur_off + skip_size, SEEK_SET);
	if (seek_off < 0) { ERR_OUT ("lseek64 fail"); }
	bp = bs + skip_size;

	for (i = 0; i < cnt; i++) {
		mlen = mlens[i];
		mbuf = (char *)(moffs[i]);
		while (mlen > 0) {
			if (be - bp <= mlen) {
				memcpy (bp, mbuf, be - bp);

				// flush
				if (skip_size) {
					err		  = H5VL_log_nb_flush_posix_write (fp->fd, bs + skip_size,
														   fp->ssize - skip_size);
					skip_size = 0;
				} else {
					err = H5VL_log_nb_flush_posix_write (fp->fd, bs, fp->ssize);
				}
				CHECK_ERR

				// Move to next stride
				seek_off = lseek64 (fp->fd, stride, SEEK_CUR);
				if (seek_off < 0) { ERR_OUT ("lseek64 fail"); }

				// Bytes written
				mlen -= be - bp;
				mbuf += be - bp;

				// Reset chunk buffer
				bp = bs;
			} else {
				memcpy (bp, mbuf, mlen);
				bp += mlen;
				mlen = 0;
			}
		}
	}
	// Last stripe
	if (bp > bs) {
		err = H5VL_log_nb_flush_posix_write (fp->fd, bs + skip_size, bp - bs - skip_size);
	}

	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_NB_WRITE_REQS_ALIGNED);
err_out:

	// Cleanup
	H5VL_log_free (bs);

	return err;
}

herr_t H5VL_log_nb_flush_write_reqs_align (void *file, hid_t dxplid) {
	herr_t err = 0;
	int mpierr;
	int i;
	int cnt;
	int *mlens		   = NULL;
	off64_t *moffs	   = NULL;
	MPI_Datatype mtype = MPI_DATATYPE_NULL;
	MPI_Status stat;
	MPI_Offset *fsize_local, *fsize_all, foff, fbase;
	void *ldp;
	hid_t ldsid = -1;
	hsize_t dsize, remain;
	haddr_t doff;
	H5VL_loc_params_t loc;
	char dname[16];
	H5VL_log_file_t *fp = (H5VL_log_file_t *)file;
	H5VL_LOGI_PROFILING_TIMER_START;

	cnt = fp->wreqs.size () - fp->nflushed;

	fsize_local = (MPI_Offset *)malloc (sizeof (MPI_Offset) * fp->scount * 2);
	fsize_all	= fsize_local + fp->scount;

	memset (fsize_local, 0, sizeof (MPI_Offset) * fp->scount);

	// Construct memory type
	mlens = (int *)malloc (sizeof (int) * cnt);
	moffs = (off64_t *)malloc (sizeof (off64_t) * cnt);
	for (i = fp->nflushed; i < fp->wreqs.size (); i++) {
		moffs[i - fp->nflushed] = (off64_t)fp->wreqs[i].xbuf;
		mlens[i - fp->nflushed] = fp->wreqs[i].rsize;
		fp->wreqs[i].ldoff		= fsize_local[fp->target_ost];
		fsize_local[fp->target_ost] += fp->wreqs[i].rsize;
	}

#ifdef LOGVOL_PROFILING
	H5VL_log_profile_add_time (fp, TIMER_H5VL_LOG_NB_FLUSH_WRITE_REQS_SIZE,
							   (double)(fsize_local[fp->target_ost]) / 1048576);
#endif

	// Get file offset and total size per ost
	mpierr = MPI_Allreduce (fsize_local, fsize_all, fp->scount, MPI_LONG_LONG, MPI_SUM, fp->comm);
	CHECK_MPIERR

	// The max size among OSTs
	dsize = 0;
	for (i = 0; i < fp->scount; i++) {
		if (fsize_all[i] > dsize) dsize = fsize_all[i];
	}

	// Create log dataset
	if (dsize) {
		// Align to the next stripe boundary
		remain = dsize % fp->ssize;
		if (remain) { dsize += fp->ssize - remain; }
		dsize *= fp->scount;  // Total size on all osts
		dsize += fp->ssize;	  // Add 1 stripe for alignment alignment

		ldsid = H5Screate_simple (1, &dsize, &dsize);
		CHECK_ID (ldsid)
		sprintf (dname, "_ld_%d", fp->nldset);
		loc.obj_type = H5I_GROUP;
		loc.type	 = H5VL_OBJECT_BY_SELF;
		ldp			 = H5VLdataset_create (fp->lgp, &loc, fp->uvlid, dname, H5P_LINK_CREATE_DEFAULT,
								   H5T_STD_B8LE, ldsid, H5P_DATASET_CREATE_DEFAULT,
								   H5P_DATASET_ACCESS_DEFAULT, dxplid, NULL);
		CHECK_PTR (ldp);

		H5VL_LOGI_PROFILING_TIMER_START;
		err = H5VL_logi_dataset_optional_wrapper (ldp, fp->uvlid, H5VL_NATIVE_DATASET_GET_OFFSET,
												  dxplid, NULL, &doff);
		CHECK_ERR	   // Get dataset file offset
		if (remain) {  // Align to the next stripe
			doff += fp->ssize - remain;
		}
		H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLDATASET_OPTIONAL);

		err = H5VLdataset_close (ldp, fp->uvlid, dxplid, NULL);
		CHECK_ERR  // Close the dataset

		// Receive offset form prev node
		fbase = 0;
		if (fp->noderank == 0) {
			if (fp->prev_rank >= 0) {
				mpierr = MPI_Recv (&fbase, 1, MPI_LONG_LONG, fp->prev_rank, 0, fp->comm, &stat);
				CHECK_MPIERR
			}
		}

		// Offset for each process
		foff = fbase;
		fsize_local[fp->target_ost] +=
			fbase;	// Embed node offset in process offset to save a bcast
		MPI_Exscan (fsize_local + fp->target_ost, &foff, 1, MPI_LONG_LONG, MPI_SUM, fp->nodecomm);
		fsize_local[fp->target_ost] -= fbase;

		// Write the data
		err = H5VL_log_nb_ost_write (fp, doff, foff, cnt, mlens, moffs);
		CHECK_ERR

		// Notify next node
		MPI_Reduce (fsize_local + fp->target_ost, &fbase, 1, MPI_LONG_LONG, MPI_SUM, 0,
					fp->nodecomm);
		if (fp->noderank == 0) {
			if (fp->next_rank >= 0) {
				mpierr = MPI_Send (&fbase, 1, MPI_LONG_LONG, fp->next_rank, 0, fp->comm);
				CHECK_MPIERR
			}
		}

		// Update metadata
		for (i = fp->nflushed; i < fp->wreqs.size (); i++) {
			fp->wreqs[i].ldid = fp->nldset;
			fp->wreqs[i].ldoff += foff;
			if (fp->wreqs[i].xbuf != fp->wreqs[i].ubuf) {
				// H5VL_log_filei_bfree (fp, (void *)(fp->wreqs[i].xbuf));
			}
		}
		(fp->nldset)++;
		fp->nflushed = fp->wreqs.size ();

		if (fsize_all) { fp->metadirty = true; }

		// err = H5VL_log_filei_pool_free (&(fp->data_buf));
		// CHECK_ERR
	}

	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_NB_FLUSH_WRITE_REQS);
err_out:
	// Cleanup
	if (mtype != MPI_DATATYPE_NULL) MPI_Type_free (&mtype);
	H5VL_log_Sclose (ldsid);

	H5VL_log_free (mlens);
	H5VL_log_free (moffs);

	return err;
}
