#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <hdf5.h>
#include <mpi.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "H5VL_logi.hpp"
#include "H5VL_logi_dataspace.hpp"
#include "H5VL_logi_debug.hpp"
#include "H5VL_logi_err.hpp"
#include "H5VL_logi_util.hpp"
#include "H5VL_log_dataset.hpp"
#include "H5VL_logi_meta.hpp"

template <class T>
static bool lessthan (int ndim, T *a, T *b) {
	int i;

	for (i = 0; i < ndim; i++) {
		if (*a < *b) {
			return true;
		} else if (*b < *a) {
			return false;
		}
		++a;
		++b;
	}

	return false;
}

#define ELEM_SWAP_EX(A, B)     \
	{                          \
		tmp		  = starts[A]; \
		starts[A] = starts[B]; \
		starts[B] = tmp;       \
		tmp		  = counts[A]; \
		counts[A] = counts[B]; \
		counts[B] = tmp;       \
	}

static void sortblocks (int ndim, int len, hsize_t **starts, hsize_t **counts) {
	int i, j, k;
	hsize_t *tmp1, *tmp2;
	std::vector<int> idx (len);

	for (i = 0; i < len; i++) { idx[i] = i; }

	auto comp = [&] (int &l, int &r) -> bool { return lessthan (ndim, starts[l], starts[r]); };
	std::sort (idx.begin (), idx.end (), comp);

	// Reorder according to index
	for (i = 0; i < len; i++) {
		if (idx[i] != i) {
			// Pull out element i
			tmp1 = starts[i];
			tmp2 = counts[i];

			// Cyclic swap until we ran back
			j = i;
			k = idx[j];
			while (k != i) {
				starts[j] = starts[k];
				counts[j] = counts[k];
				idx[j]	  = j;

				j = k;
				k = idx[j];
			}

			starts[j] = tmp1;
			counts[j] = tmp2;
			idx[j]	  = j;
		}
	}
}

H5VL_log_selections::H5VL_log_selections () : ndim (0), nsel (0), sels_arr(NULL) {}
H5VL_log_selections::H5VL_log_selections (int ndim, int nsel) : ndim (ndim), nsel (nsel) {
	this->reserve (nsel);
}
H5VL_log_selections::H5VL_log_selections (int ndim, int nsel, hsize_t **starts, hsize_t **counts)
	: ndim (ndim), nsel (nsel), starts (starts), counts (counts) {}

H5VL_log_selections::H5VL_log_selections (H5VL_log_selections &rhs)
	: ndim (rhs.ndim), nsel (rhs.nsel) {
	int i;

	if (rhs.sels_arr) {
		this->reserve (rhs.nsel);

		// Copy the data
		memcpy (starts, rhs.starts, sizeof (hsize_t *) * nsel);
		memcpy (counts, rhs.counts, sizeof (hsize_t *) * nsel);

		for (i = 0; i < nsel; i++) {
			memcpy (starts[i], rhs.starts[i], sizeof (hsize_t) * ndim);
			memcpy (counts[i], rhs.counts[i], sizeof (hsize_t) * ndim);
		}
	} else {
		this->starts	 = rhs.starts;
		this->counts	 = rhs.counts;
		this->sels_arr = NULL;
	}
}

H5VL_log_selections::H5VL_log_selections (hid_t dsid) {
	herr_t err = 0;
	int i, j, k, l;
	int nreq, old_nreq, nbreq;
	hssize_t nblock;
	H5S_sel_type stype;
	hsize_t **hstarts = NULL, **hends;
	int *group		  = NULL;

	ndim = H5Sget_simple_extent_ndims (dsid);
	CHECK_ID (ndim)

	// Get selection type
	if (dsid == H5S_ALL) {
		stype = H5S_SEL_ALL;
	} else {
		stype = H5Sget_select_type (dsid);
	}

	switch (stype) {
		case H5S_SEL_HYPERSLABS: {
			nblock = H5Sget_select_hyper_nblocks (dsid);
			CHECK_ID (nblock)

			if (nblock == 1) {
				hsize_t cord[H5S_MAX_RANK * 2];

				err = H5Sget_select_hyper_blocklist (dsid, 0, 1, cord);

				this->nsel = 1;
				this->reserve (1);

				for (i = 0; i < ndim; i++) {
					starts[0][i] = (MPI_Offset)cord[i];
					counts[0][i] = (MPI_Offset)cord[i + ndim] - starts[0][i] + 1;
				}
			} else {
				hstarts = (hsize_t **)malloc (sizeof (hsize_t *) * nblock * 2);
				CHECK_PTR (hstarts)
				hends = hstarts + nblock;

				hstarts[0] = (hsize_t *)malloc (sizeof (hsize_t) * ndim * 2 * nblock);
				CHECK_PTR (hstarts[0])
				hends[0] = hstarts[0] + ndim;
				for (i = 1; i < nblock; i++) {
					hstarts[i] = hstarts[i - 1] + ndim * 2;
					hends[i]   = hends[i - 1] + ndim * 2;
				}

				err = H5Sget_select_hyper_blocklist (dsid, 0, nblock, hstarts[0]);

				sortblocks (ndim, nblock, hstarts, hends);

				// Check for interleving
				group = (int *)malloc (sizeof (int) * (nblock + 1));
				CHECK_PTR (group)
				group[0] = 0;
				for (i = 0; i < nblock - 1; i++) {
					if (lessthan (
							ndim, hends[i],
							hstarts[i + 1])) {	// The end cord is the max offset of the previous
												// block. If less than the start offset of the next
												// block, there won't be interleving
						group[i + 1] = group[i] + 1;
					} else {
						group[i + 1] = group[i];
					}
				}
				group[i + 1] = group[i] + 1;  // Dummy group to trigger process for final group

				// Count number of requests
				j	 = 0;
				nreq = 0;
				for (i = j = 0; i <= nblock; i++) {
					if (group[i] != group[j]) {	 // Within 1 group
						if (i - j == 1) {		 // Sinlge block, no need to breakdown
							// offs[k] = nreq; // Record offset
							nreq++;
							j++;
						} else {
							for (; j < i; j++) {
								// offs[i] = nreq; // Record offset
								nbreq = 1;
								for (k = 0; k < ndim - 1; k++) {
									nbreq *= hends[j][k] - hstarts[j][k] + 1;
								}
								nreq += nbreq;
							}
						}
					}
				}

				// Allocate buffer
				this->reserve (nreq);

				// Fill up selections
				nreq = 0;
				for (i = j = 0; i <= nblock; i++) {
					if (group[i] != group[j]) {	 // Within 1 group
						if (i - j == 1) {		 // Sinlge block, no need to breakdown
							for (k = 0; k < ndim; k++) {
								starts[nreq][k] = (MPI_Offset)hstarts[j][k];
								counts[nreq][k] = (MPI_Offset) (hends[j][k] - hstarts[j][k] + 1);
							}
							// offs[k] = nreq; // Record offset
							nreq++;
							j++;
						} else {
							old_nreq = nreq;
							for (; j < i; j++) {  // Breakdown each block
								for (k = 0; k < ndim; k++) {
									starts[nreq][k] = (MPI_Offset)hstarts[j][k];
									counts[nreq][k] = 1;
								}
								starts[nreq][ndim - 1] =
									(MPI_Offset) (hends[j][ndim - 1] - hstarts[j][ndim - 1] + 1);

								for (l = 0; l < ndim;
									 l++) {	 // The lowest dimension that we haven't reach the end
									if (starts[nreq][l] < (MPI_Offset)hends[j][l]) break;
								}
								nreq++;
								while (l < ndim - 1) {	// While we haven't handle the last one
									memcpy (
										starts[nreq], starts[nreq - 1],
										sizeof (MPI_Offset) * ndim);  // Start from previous value
									memcpy (counts[nreq], counts[nreq - 1],
											sizeof (MPI_Offset) * ndim);  // Fill in Count

									// Increase start to the next location, carry to lower dim if
									// overflow
									starts[nreq][ndim - 2]++;
									for (k = ndim - 2; k > 0; k--) {
										if (starts[nreq][k] > (MPI_Offset) (hends[j][k])) {
											starts[nreq][k] = (MPI_Offset) (hstarts[j][k]);
											starts[nreq][k - 1]++;
										} else {
											break;
										}
									}

									for (l = 0; l < ndim; l++) {  // The lowest dimension that we
																  // haven't reach the end
										if (starts[nreq][l] < (MPI_Offset)hends[j][l]) break;
									}
									nreq++;
								}
							}

							// Sort into non-decreasing order
							sortblocks (ndim, nreq - old_nreq, starts + old_nreq,
										counts + old_nreq);

							// HDF5 selection guarantees no interleving, no need to merge
						}
					}
				}
			}
		} break;
		case H5S_SEL_POINTS: {
			nblock = H5Sget_select_elem_npoints (dsid);
			CHECK_ID (nblock)

			this->nsel = nblock;
			this->reserve (nblock);

			if (nblock) {
				hstarts	   = (hsize_t **)malloc (sizeof (hsize_t) * nblock);
				hstarts[0] = (hsize_t *)malloc (sizeof (hsize_t) * ndim * nblock);
				for (i = 1; i < nblock; i++) { hstarts[i] = hstarts[i - 1] + ndim; }

				err = H5Sget_select_elem_pointlist (dsid, 0, nblock, hstarts[0]);
				CHECK_ERR

				for (i = 0; i < nblock; i++) {
					for (j = 0; j < ndim; j++) {
						starts[i][j] = (MPI_Offset)hstarts[i][j];
						counts[i][j] = 1;
					}
				}
			}
		} break;
		case H5S_SEL_ALL: {
			hsize_t dims[32];

			err = H5Sget_simple_extent_dims (dsid, dims, NULL);
			CHECK_ERR

			this->nsel = 1;
			this->reserve (1);

			for (j = 0; j < ndim; j++) {
				starts[0][j] = (MPI_Offset)dims[j];
				counts[0][j] = 1;
			}

		} break;
		default:
			ERR_OUT ("Unsupported selection type");
	}

err_out:;
	if (hstarts) {
		free (hstarts[0]);
		free (hstarts);
	}
	if (group) { free (group); }

	if (err) { throw "Can not retrieve selections"; }
}

H5VL_log_selections::~H5VL_log_selections () {
	if (sels_arr) {
		free (sels_arr);
		free (starts);
	}
}

H5VL_log_selections &H5VL_log_selections::operator= (H5VL_log_selections &rhs) {
	int i;

	// Deallocate existing start and count array
	if (sels_arr) {
		free (sels_arr);
		free (starts);
	}

	this->nsel = rhs.nsel;
	this->ndim = rhs.ndim;

	if (rhs.sels_arr) {
		this->reserve (nsel);

		// Copy the data
		memcpy (starts, rhs.starts, sizeof (hsize_t *) * nsel);
		memcpy (counts, rhs.counts, sizeof (hsize_t *) * nsel);

		for (i = 0; i < nsel; i++) {
			memcpy (starts[i], rhs.starts[i], sizeof (hsize_t) * ndim);
			memcpy (counts[i], rhs.counts[i], sizeof (hsize_t) * ndim);
		}
	} else {
		this->starts	 = rhs.starts;
		this->counts	 = rhs.counts;
		this->sels_arr = NULL;
	}

	return *this;
}

// Assume blocks sorted according to start
bool H5VL_log_selections::operator== (H5VL_log_selections &rhs) {
	int i, j;

	if (ndim != rhs.ndim) { return false; }
	if (nsel != rhs.nsel) { return false; }
	for (i = 0; i < nsel; i++) {
		for (j = 0; j < ndim; j++) {
			if (starts[i][j] != rhs.starts[i][j]) { return false; }
			if (counts[i][j] != rhs.counts[i][j]) { return false; }
		}
	}

	return true;
}

// Should only be called once
void H5VL_log_selections::reserve (int nsel) {
	int err = 0;
	int i;

	if (ndim && nsel) {
		this->starts = (hsize_t **)malloc (sizeof (hsize_t *) * nsel * 2);
		CHECK_PTR (starts)
		this->counts	= this->starts + nsel;
		this->starts[0] = sels_arr = (hsize_t *)malloc (sizeof (hsize_t) * nsel * ndim * 2);
		CHECK_PTR (starts[0])
		this->counts[0] = this->starts[0] + nsel * ndim;
		for (i = 1; i < nsel; i++) {
			starts[i] = starts[i - 1] + ndim;
			counts[i] = counts[i - 1] + ndim;
		}
	} else {
		this->starts = NULL;
		this->counts = NULL;
		this->sels_arr = NULL;
	}

err_out:;
	if (err) { throw "OOM"; }
}

void H5VL_log_selections::convert_to_deep () {
	int i;
	hsize_t **starts_tmp;
	hsize_t **counts_tmp;

	if (!sels_arr) {
		starts_tmp = starts;
		counts_tmp = counts;

		// Allocate new space
		this->reserve (nsel);

		// Copy the data
		memcpy (starts, starts_tmp, sizeof (hsize_t *) * nsel);
		memcpy (counts, counts_tmp, sizeof (hsize_t *) * nsel);

		for (i = 0; i < nsel; i++) {
			memcpy (starts[i], starts_tmp[i], sizeof (hsize_t) * ndim);
			memcpy (counts[i], counts_tmp[i], sizeof (hsize_t) * ndim);
		}
	}
}

herr_t H5VL_log_selections::get_mpi_type (size_t esize, MPI_Datatype *type) {
	herr_t err = 0;
	int mpierr;
	int i, j;
	bool derived_etype;
	MPI_Aint *offs		= NULL;
	int *lens			= NULL;
	MPI_Datatype *types = NULL;
	MPI_Datatype etype;
	int size[H5S_MAX_RANK], ssize[H5S_MAX_RANK], sstart[H5S_MAX_RANK];
	hsize_t hsize[H5S_MAX_RANK];

	etype = H5VL_logi_get_mpi_type_by_size (esize);
	if (etype == MPI_DATATYPE_NULL) {
		mpierr = MPI_Type_contiguous (esize, MPI_BYTE, &etype);
		CHECK_MPIERR
		mpierr = MPI_Type_commit (&etype);
		CHECK_MPIERR
		derived_etype = true;
	} else {
		derived_etype = false;
	}

	for (i = 0; i < ndim; i++) { size[i] = (MPI_Offset)hsize[i]; }

	// No selection, return
	if (nsel == 0) {
		*type = MPI_DATATYPE_NULL;
		return 0;
	}

	lens = (int *)malloc (sizeof (int) * nsel);
	CHECK_PTR (lens)
	offs = (MPI_Aint *)malloc (sizeof (MPI_Aint) * nsel);
	CHECK_PTR (offs)
	types = (MPI_Datatype *)malloc (sizeof (MPI_Datatype) * nsel);
	CHECK_PTR (types)

	// Construct the actual type
	for (i = 0; i < nsel; i++) {
		for (j = 0; j < ndim; j++) {
			ssize[j]  = (int)(counts[i][j]);
			sstart[j] = (int)(starts[i][j]);
		}

		mpierr = H5VL_log_debug_MPI_Type_create_subarray (ndim, size, ssize, sstart, MPI_ORDER_C,
														  etype, types + i);
		CHECK_MPIERR
		mpierr = MPI_Type_commit (types + i);
		CHECK_MPIERR
		offs[i] = 0;
		lens[i] = 1;
	}

	mpierr = MPI_Type_struct (nsel, lens, offs, types, type);
	CHECK_MPIERR
	mpierr = MPI_Type_commit (type);
	CHECK_MPIERR

err_out:

	if (types != NULL) {
		for (i = 0; i < nsel; i++) {
			if (types[i] != MPI_BYTE) { MPI_Type_free (types + i); }
		}
		free (types);
	}
	free (offs);
	free (lens);

	if (derived_etype) { MPI_Type_free (&etype); }

	return err;
}

hsize_t H5VL_log_selections::get_sel_size (int idx) {
	hsize_t ret = 1;
	hsize_t *ptr;

	for (ptr = counts[idx]; ptr < counts[idx] + ndim; ptr++) { ret *= *ptr; }

	return ret;
}

hsize_t H5VL_log_selections::get_sel_size () {
	int i;
	hsize_t ret = 0;
	hsize_t bsize;
	hsize_t *ptr;

	for (i = 0; i < nsel; i++) {
		bsize = 1;
		for (ptr = counts[i]; ptr < counts[i] + ndim; ptr++) { 
			bsize *= *ptr; 
			}
		ret += bsize;
	}

	return ret;
}

void H5VL_log_selections::encode (H5VL_log_dset_info_t &dset,
								   H5VL_logi_meta_hdr &hdr,
								   char *mbuf) {
	int i;

	if (hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
		// Dsteps
		memcpy (mbuf, dset.dsteps, sizeof (MPI_Offset) * (dset.ndim - 1));
		mbuf += sizeof (MPI_Offset) * (dset.ndim - 1);

		for (i = 0; i < nsel; i++) {
			H5VL_logi_sel_encode (dset.ndim, dset.dsteps, starts[i], (MPI_Offset *)mbuf);
			mbuf += sizeof (MPI_Offset);
		}
		for (i = 0; i < nsel; i++) {
			H5VL_logi_sel_encode (dset.ndim, dset.dsteps, counts[i], (MPI_Offset *)mbuf);
			mbuf += sizeof (MPI_Offset);
		}
	} else {
		for (i = 0; i < nsel; i++) {
			memcpy (mbuf, starts[i], sizeof (hsize_t) * dset.ndim);
			mbuf += sizeof (hsize_t) * dset.ndim;
		}
		for (i = 0; i < nsel; i++) {
			memcpy (mbuf, counts[i], sizeof (hsize_t) * dset.ndim);
			mbuf += sizeof (hsize_t) * dset.ndim;
		}
	}
}