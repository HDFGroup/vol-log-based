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

herr_t H5VL_logi_get_dataspace_sel_type (hid_t sid, size_t esize, MPI_Datatype *type) {
	herr_t err = 0;
	int mpierr;
	int i, j;
	bool derived_etype;
	int n, ndim;
	std::vector<H5VL_log_selection> sels;
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

	ndim = H5Sget_simple_extent_dims (sid, hsize, NULL);
	CHECK_ID (ndim)
	for (i = 0; i < ndim; i++) { size[i] = (MPI_Offset)hsize[i]; }

	err = H5VL_logi_get_dataspace_selection (sid, sels);
	CHECK_ERR

	n = sels.size ();
	if (n == 0) {
		*type = MPI_DATATYPE_NULL;
		return 0;
	}

	lens  = (int *)malloc (sizeof (int) * n);
	offs  = (MPI_Aint *)malloc (sizeof (MPI_Aint) * n);
	types = (MPI_Datatype *)malloc (sizeof (MPI_Datatype) * n);

	// Construct the actual type
	for (i = 0; i < n; i++) {
		for (j = 0; j < ndim; j++) {
			ssize[j]  = (int)(sels[i].count[j]);
			sstart[j] = (int)(sels[i].start[j]);
		}

		mpierr = H5VL_log_debug_MPI_Type_create_subarray (ndim, size, ssize, sstart, MPI_ORDER_C,
														  etype, types + i);
		CHECK_MPIERR
		mpierr = MPI_Type_commit (types + i);
		CHECK_MPIERR
		offs[i] = 0;
		lens[i] = 1;
	}

	mpierr = MPI_Type_struct (n, lens, offs, types, type);
	CHECK_MPIERR
	mpierr = MPI_Type_commit (type);
	CHECK_MPIERR

err_out:

	if (types != NULL) {
		for (i = 0; i < n; i++) {
			if (types[i] != MPI_BYTE) { MPI_Type_free (types + i); }
		}
		free (types);
	}
	free (offs);
	free (lens);

	if (derived_etype) { MPI_Type_free (&etype); }

	return err;
}

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
template <class T>
static void sortvec_ex (int ndim, int len, T **starts, T **counts) {
	int i, j, p;
	T *tmp;

	if (len < 16) {
		j = 1;
		while (j) {
			j = 0;
			for (i = 0; i < len - 1; i++) {
				if (lessthan (ndim, starts[i + 1], starts[i])) {
					ELEM_SWAP_EX (i, i + 1)
					j = 1;
				}
			}
		}
		return;
	}

	p = len / 2;
	ELEM_SWAP_EX (p, len - 1);
	p = len - 1;

	j = 0;
	for (i = 0; i < len; i++) {
		if (lessthan (ndim, starts[i], starts[p])) {
			if (i != j) { ELEM_SWAP_EX (i, j) }
			j++;
		}
	}

	ELEM_SWAP_EX (p, j)

	sortvec_ex (ndim, j, starts, counts);
	sortvec_ex (ndim, len - j - 1, starts + j + 1, counts + j + 1);
}

bool sel_less_than (int ndim, H5VL_log_selection &l, H5VL_log_selection &r) {
	int i;

	for (i = 0; i < ndim; i++) {
		if (l.start[i] < r.start[i]) {
			return true;
		} else if (r.start[i] < l.start[i]) {
			return false;
		}
	}

	return false;
}

herr_t H5VL_logi_get_dataspace_selection (hid_t sid, std::vector<H5VL_log_selection> &sels) {
	herr_t err = 0;
	int i, j, k, l;
	int ndim;
	int nreq, old_nreq, nbreq;
	hssize_t nblock;
	H5S_sel_type stype;
	hsize_t **hstarts = NULL, **hends;
	int *group		  = NULL;

	ndim = H5Sget_simple_extent_ndims (sid);
	CHECK_ID (ndim)

	// Get selection type
	if (sid == H5S_ALL) {
		stype = H5S_SEL_ALL;
	} else {
		stype = H5Sget_select_type (sid);
	}

	switch (stype) {
		case H5S_SEL_HYPERSLABS: {
			nblock = H5Sget_select_hyper_nblocks (sid);
			CHECK_ID (nblock)

			if (nblock == 1) {
				hsize_t cord[H5S_MAX_RANK * 2];

				err = H5Sget_select_hyper_blocklist (sid, 0, 1, cord);

				sels.resize (1);
				for (i = 0; i < ndim; i++) {
					sels[0].start[i] = (MPI_Offset)cord[i];
					sels[0].count[i] = (MPI_Offset)cord[i + ndim] - sels[0].start[i] + 1;
				}
			} else {
				hstarts = (hsize_t **)malloc (sizeof (hsize_t *) * nblock * 2);
				hends	= hstarts + nblock;

				hstarts[0] = (hsize_t *)malloc (sizeof (hsize_t) * ndim * 2 * nblock);
				hends[0]   = hstarts[0] + ndim;
				for (i = 1; i < nblock; i++) {
					hstarts[i] = hstarts[i - 1] + ndim * 2;
					hends[i]   = hends[i - 1] + ndim * 2;
				}

				err = H5Sget_select_hyper_blocklist (sid, 0, nblock, hstarts[0]);

				sortvec_ex (ndim, nblock, hstarts, hends);

				// Check for interleving
				group	 = (int *)malloc (sizeof (int) * (nblock + 1));
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
				sels.resize (nreq);

				// Fill up selections
				nreq = 0;
				for (i = j = 0; i <= nblock; i++) {
					if (group[i] != group[j]) {	 // Within 1 group
						if (i - j == 1) {		 // Sinlge block, no need to breakdown
							for (k = 0; k < ndim; k++) {
								sels[nreq].start[k] = (MPI_Offset)hstarts[j][k];
								sels[nreq].count[k] =
									(MPI_Offset) (hends[j][k] - hstarts[j][k] + 1);
							}
							// offs[k] = nreq; // Record offset
							nreq++;
							j++;
						} else {
							old_nreq = nreq;
							for (; j < i; j++) {  // Breakdown each block
								for (k = 0; k < ndim; k++) {
									sels[nreq].start[k] = (MPI_Offset)hstarts[j][k];
									sels[nreq].count[k] = 1;
								}
								sels[nreq].count[ndim - 1] =
									(MPI_Offset) (hends[j][ndim - 1] - hstarts[j][ndim - 1] + 1);

								for (l = 0; l < ndim;
									 l++) {	 // The lowest dimension that we haven't reach the end
									if (sels[nreq].start[l] < (MPI_Offset)hends[j][l]) break;
								}
								nreq++;
								while (l < ndim - 1) {	// While we haven't handle the last one
									memcpy (
										sels[nreq].start, sels[nreq - 1].start,
										sizeof (MPI_Offset) * ndim);  // Start from previous value
									memcpy (sels[nreq].count, sels[nreq - 1].count,
											sizeof (MPI_Offset) * ndim);  // Fill in Count

									// Increase start to the next location, carry to lower dim if
									// overflow
									sels[nreq].start[ndim - 2]++;
									for (k = ndim - 2; k > 0; k--) {
										if (sels[nreq].start[k] > (MPI_Offset) (hends[j][k])) {
											sels[nreq].start[k] = (MPI_Offset) (hstarts[j][k]);
											sels[nreq].start[k - 1]++;
										} else {
											break;
										}
									}

									for (l = 0; l < ndim; l++) {  // The lowest dimension that we
																  // haven't reach the end
										if (sels[nreq].start[l] < (MPI_Offset)hends[j][l]) break;
									}
									nreq++;
								}
							}

							auto comp = [&] (H5VL_log_selection &l, H5VL_log_selection &r) -> bool {
								return sel_less_than (ndim, l, r);
							};
							std::sort (sels.begin () + old_nreq, sels.begin () + nreq, comp);
							// sortvec_ex(ndim, nreq - old_nreq, starts + old_nreq, count +
							// old_nreq);
						}
					}
				}
			}
		} break;
		case H5S_SEL_POINTS: {
			nblock = H5Sget_select_elem_npoints (sid);
			CHECK_ID (nblock)

			if (nblock) {
				hstarts	   = (hsize_t **)malloc (sizeof (hsize_t) * nblock);
				hstarts[0] = (hsize_t *)malloc (sizeof (hsize_t) * ndim * nblock);
				for (i = 1; i < nblock; i++) { hstarts[i] = hstarts[i - 1] + ndim; }

				err = H5Sget_select_elem_pointlist (sid, 0, nblock, hstarts[0]);
				CHECK_ERR

				sels.resize (nblock);

				for (i = 0; i < nblock; i++) {
					for (j = 0; j < ndim; j++) {
						sels[i].start[j] = (MPI_Offset)hstarts[i][j];
						sels[i].count[j] = 1;
					}
				}
			}
		} break;
		case H5S_SEL_ALL: {
			hsize_t dims[32];

			err = H5Sget_simple_extent_dims (sid, dims, NULL);
			CHECK_ERR

			sels.resize (1);
			for (j = 0; j < ndim; j++) {
				sels[0].start[j] = (MPI_Offset)dims[j];
				sels[0].count[j] = 1;
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

	return err;
}

/*
herr_t H5VL_logi_get_dataspace_selection2(H5VL_log_file_t *fp, hid_t sid, H5VL_log_wreq_t *r){
	herr_t err = 0;
	int i, j, k, l;
	int ndim;
	int nreq, old_nreq, nbreq;
	hssize_t nblock;
	H5S_sel_type stype;
	hsize_t **hstarts = NULL, **hends;
	int *group=NULL;

	ndim = H5Sget_simple_extent_ndims(sid); CHECK_ID(ndim)

	// Get selection type
	if (sid == H5S_ALL){
		stype = H5S_SEL_ALL;
	}
	else{
		stype =  H5Sget_select_type(sid);
	}

	switch (stype){
		case H5S_SEL_HYPERSLABS:
			{
				nblock = H5Sget_select_hyper_nblocks(sid); CHECK_ID(nblock)

				hstarts = (hsize_t**)malloc(sizeof(hsize_t*) * nblock * 2);
				hends = hstarts + nblock;

				hstarts[0] = (hsize_t*)malloc(sizeof(hsize_t) * ndim * 2 * nblock);
				hends[0] = hstarts[0] + ndim;
				for(i = 1; i < nblock; i++){
					hstarts[i] = hstarts[i - 1] + ndim * 2;
					hends[i] = hends[i - 1] + ndim * 2;
				}

				err = H5Sget_select_hyper_blocklist(sid, 0, nblock, hstarts[0]);

				sortvec_ex(ndim, nblock, hstarts, hends);

				// Check for interleving
				group = (int*)malloc(sizeof(int) * (nblock + 1));
				group[0] = 0;
				for(i = 0; i < nblock - 1; i++){
					if (lessthan(ndim, hends[i], hstarts[i + 1])){  // The end cord is the max
offset of the previous block. If less than the start offset of the next block, there won't be
interleving group[i + 1] = group[i] + 1;
					}
					else{
						group[i + 1] = group[i];
					}
				}
				group[i + 1] = group[i] + 1; // Dummy group to trigger process for final group

				// Count number of requests
				j = 0;
				nreq = 0;
				for(i = j = 0; i <= nblock; i++){
					if (group[i] != group[j]){  // Within 1 group
						if (i - j == 1){    // Sinlge block, no need to breakdown
							//offs[k] = nreq; // Record offset
							nreq++;
							j++;
						}
						else{
							for(;j < i; j++){
								//offs[i] = nreq; // Record offset
								nbreq = 1;
								for(k = 0; k < ndim - 1; k++){
									nbreq *= hends[j][k] - hstarts[j][k] + 1;
								}
								nreq += nbreq;
							}
						}
					}
				}

				// Allocate buffer
				err= H5VL_log_dataspacei_alloc_selection (fp, r, nreq); CHECK_ERR

				// Fill up selections
				nreq = 0;
				for(i = j = 0; i <= nblock; i++){
					if (group[i] != group[j]){  // Within 1 group
						if (i - j == 1){    // Sinlge block, no need to breakdown
							for(k = 0; k < ndim; k++){
								r->starts[nreq][k] =  (MPI_Offset)hstarts[j][k];
								r->counts[nreq][k] =  (MPI_Offset)(hends[j][k] - hstarts[j][k] + 1);
							}
							//offs[k] = nreq; // Record offset
							nreq++;
							j++;
						}
						else{
							old_nreq = nreq;
							for(;j < i; j++){    // Breakdown each block
								for(k = 0; k < ndim; k++){
									r->starts[nreq][k] = (MPI_Offset)hstarts[j][k];
									r->counts[nreq][k] = 1;
								}
								r->counts[nreq][ndim - 1] =  (MPI_Offset)(hends[j][ndim - 1] -
hstarts[j][ndim - 1] + 1);

								for(l = 0; l < ndim; l++){   // The lowest dimension that we haven't
reach the end if (r->starts[nreq][l] < (MPI_Offset)hends[j][l]) break;
								}
								nreq++;
								while(l < ndim - 1){    // While we haven't handle the last one
									memcpy(r->starts[nreq], r->starts[nreq - 1], sizeof(MPI_Offset)
* ndim);  // Start from previous value memcpy(r->counts[nreq], r->counts[nreq - 1],
sizeof(MPI_Offset) * ndim);  // Fill in Count

									// Increase start to the next location, carry to lower dim if
overflow r->starts[nreq][ndim - 2]++; for(k = ndim - 2; k > 0; k--){ if (r->starts[nreq][k] >
(MPI_Offset)(hends[j][k])){ r->starts[nreq][k] = (MPI_Offset)(hstarts[j][k]); r->starts[nreq][k -
1]++;
										}
										else{
											break;
										}
									}

									for(l = 0; l < ndim; l++){   // The lowest dimension that we
haven't reach the end if (r->starts[nreq][l] < (MPI_Offset)hends[j][l]) break;
									}
									nreq++;
								}
							}

							auto comp = [&](H5VL_log_selection &l, H5VL_log_selection &r)-> bool {
								return sel_less_than(ndim, l, r);
							};
							std::sort(sels.begin() + old_nreq, sels.begin() + nreq, comp);
							//sortvec_ex(ndim, nreq - old_nreq, starts + old_nreq, count +
old_nreq);
						}
					}
				}
			}
			break;
		case H5S_SEL_POINTS:
			{
				nblock = H5Sget_select_elem_npoints(sid); CHECK_ID(nblock)

				if (nblock){
					hstarts = (hsize_t**)malloc(sizeof(hsize_t) * nblock);
					hstarts[0] = (hsize_t*)malloc(sizeof(hsize_t) * ndim * nblock);
					for(i = 1; i < nblock; i++){
						hstarts[i] = hstarts[i - 1] + ndim;
					}

					err = H5Sget_select_elem_pointlist(sid, 0, nblock, hstarts[0]); CHECK_ERR

					sels.resize(nblock);

					for(i = 0; i < nblock; i++){
						for(j = 0; j < ndim; j++){
							sels[i].start[j] = (MPI_Offset)hstarts[i][j];
							sels[i].count[j] = 1;
						}
					}
				}
			}
			break;
		case H5S_SEL_ALL:
			{
				hsize_t dims[32];

				err = H5Sget_simple_extent_dims(sid, dims, NULL); CHECK_ERR

				sels.resize(1);
				for(j = 0; j < ndim; j++){
					sels[0].start[j] = (MPI_Offset)dims[j];
					sels[0].count[j] = 1;
				}

			}
			break;
		default:
			ERR_OUT("Unsupported selection type");
	}

err_out:;
	if (hstarts){
		free(hstarts[0]);
		free(hstarts);
	}
	if(group){
		free(group);
	}

	return err;
}
*/