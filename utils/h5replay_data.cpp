#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
//
#include <hdf5.h>
#include <mpi.h>
//
#include "H5VL_log_dataset.hpp"
#include "H5VL_logi_filter.hpp"
#include "h5replay.hpp"
#include "h5replay_data.hpp"
#include "h5replay_meta.hpp"

typedef struct hidx {
	MPI_Aint foff;
	MPI_Aint moff;
	int len;
	bool operator< (struct hidx &rhs) { return foff < rhs.foff; }
} hidx;

herr_t h5replay_read_data (MPI_File fin,
						   std::vector<dset_info> &dsets,
						   std::vector<std::vector<meta_block>> &reqs) {
	herr_t err = 0;
	int mpierr;
	int i, j, k, l;
	size_t bsize;	// Data buffer size of a req
	size_t esize;	// Size of a sel block
	size_t zbsize;	// Size of the decompression buffer
	char *buf;
	char *zbuf;
	std::vector<MPI_Aint> foffs, moffs;
	std::vector<int> lens;
	std::vector<hidx> idxs;
	MPI_Datatype ftype = MPI_DATATYPE_NULL;
	MPI_Datatype mtype = MPI_DATATYPE_NULL;
	MPI_Status stat;

	// Allocate buffer
	zbsize = 0;
	for (auto &reqp : reqs) {
		for (auto &req : reqp) {
			req.bufs.resize (req.sels.size ());
			bsize = 0;
			for (j = 0; j < req.sels.size (); j++) {
				esize = dsets[req.hdr.did].esize;
				for (i = 0; i < dsets[req.hdr.did].ndim; i++) { esize *= req.sels[j].count[i]; }
				req.bufs[j] = (char *)bsize;
				bsize += esize;
			}
			if (zbsize < bsize) { zbsize = bsize; }
			// Comrpessed size can be larger
			if (bsize < req.fsize) { bsize = req.fsize; }
			// Allocate buffer
			buf = (char *)malloc (bsize);
			assert (buf != NULL);
			for (j = 0; j < req.sels.size (); j++) { req.bufs[j] = buf + (size_t)req.bufs[j]; }

			// Record off and len for mpi type
			idxs.push_back ({req.foff, (MPI_Aint)req.bufs[0], (int)req.fsize});
		}
	}
	// zbuf = (char *)malloc (zbsize);

	// Read the data
	foffs.reserve (idxs.size ());
	moffs.reserve (idxs.size ());
	lens.reserve (idxs.size ());
	std::sort (idxs.begin (), idxs.end ());
	for (auto &idx : idxs) {
		foffs.push_back (idx.foff);
		moffs.push_back (idx.moff);
		lens.push_back (idx.len);
	}
	mpierr = MPI_Type_hindexed (foffs.size (), lens.data (), foffs.data (), MPI_BYTE, &ftype);
	CHECK_MPIERR
	mpierr = MPI_Type_hindexed (moffs.size (), lens.data (), moffs.data (), MPI_BYTE, &mtype);
	CHECK_MPIERR
	mpierr = MPI_Type_commit (&ftype);
	CHECK_MPIERR
	mpierr = MPI_Type_commit (&mtype);
	CHECK_MPIERR
	mpierr = MPI_File_set_view (fin, 0, MPI_BYTE, ftype, "native", MPI_INFO_NULL);
	CHECK_MPIERR
	mpierr = MPI_File_read_all (fin, MPI_BOTTOM, 1, mtype, &stat);
	CHECK_MPIERR

	// Unfilter the data
	for (auto &reqp : reqs) {
		for (auto &req : reqp) {
			if (dsets[req.hdr.did].filters.size () > 0) {
				char *buf = NULL;
				int csize = 0;

				err = H5VL_logi_unfilter (dsets[req.hdr.did].filters, req.bufs[0], req.fsize,
										  (void **)&buf, &csize);
				CHECK_ERR

				memcpy (buf, req.bufs[0], csize);
			}
		}
	}

err_out:;
	if (ftype != MPI_DATATYPE_NULL) { MPI_Type_free (&ftype); }
	if (mtype != MPI_DATATYPE_NULL) { MPI_Type_free (&mtype); }
	return err;
}

herr_t h5replay_write_data (hid_t foutid,
							std::vector<dset_info> &dsets,
							std::vector<std::vector<meta_block>> &reqs) {
	herr_t err = 0;
	hid_t dsid = -1;
	hid_t msid = -1;
	hsize_t one[H5S_MAX_RANK];
	hsize_t zero = 0;
	hsize_t msize;
	int i, j, k;

	one[0] = INT_MAX;
	msid   = H5Screate_simple (1, one, one);

	for (i = 0; i < H5S_MAX_RANK; i++) { one[i] = 1; }

	for (i = 0; i < dsets.size (); i++) {
		dsid = H5Dget_space (dsets[i].id);
		for (auto &req : reqs[i]) {
			for (k = 0; k < req.sels.size (); k++) {
				msize = 1;
				for (j = 0; j < dsets[req.hdr.did].ndim; j++) { msize *= req.sels[k].count[j]; }
				err = H5Sselect_hyperslab (msid, H5S_SELECT_SET, &zero, NULL, one, &msize);
				CHECK_ERR
				err = H5Sselect_hyperslab (dsid, H5S_SELECT_SET, req.sels[k].start, NULL, one,
										   req.sels[k].count);
				CHECK_ERR
				err = H5Dwrite (dsets[i].id, dsets[i].type, msid, dsid, H5P_DEFAULT, req.bufs[k]);
				CHECK_ERR
			}
		}
		H5Sclose (dsid);
		dsid = -1;
	}

err_out:;
	if (dsid >= 0) { H5Sclose (dsid); }
	if (msid >= 0) { H5Sclose (msid); }
	return err;
}