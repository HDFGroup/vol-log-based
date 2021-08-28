#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <mpi.h>

#include <algorithm>
#include <vector>

#include "H5VL_log_file.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_idx.hpp"
#include "H5VL_logi_nb.hpp"

herr_t H5VL_logi_array_idx_t::clear () {
	this->entries.clear ();
	return 0;
}

herr_t H5VL_logi_array_idx_t::insert (H5VL_logi_metablock_t &meta) {
	this->entries.push_back (meta);
	return 0;
}

static bool intersect (
	int ndim, hsize_t *sa, hsize_t *ca, hsize_t *sb, hsize_t *cb, hsize_t *so, hsize_t *co) {
	int i;

	for (i = 0; i < ndim; i++) {
		so[i] = std::max (sa[i], sb[i]);
		co[i] = std::min (sa[i] + ca[i], sb[i] + cb[i]) - so[i];
		if (co[i] <= 0) return false;
	}

	return true;
}

herr_t H5VL_logi_array_idx_t::search (H5VL_log_rreq_t &req,
									  std::vector<H5VL_log_idx_search_ret_t> &ret) {
	herr_t err = 0;
	int i, j, k;
	size_t soff;
	hsize_t os[H5S_MAX_RANK], oc[H5S_MAX_RANK];
	H5VL_log_idx_search_ret_t cur;

	soff = 0;
	for (i = 0; i < req.sels->nsel; i++) {
		for (auto &ent : this->entries) {
			for (auto &msel : ent.sels) {
				if (intersect (req.ndim, msel.start, msel.count, req.sels->starts[i],
							   req.sels->counts[i], os, oc)) {
					for (j = 0; j < req.ndim; j++) {
						cur.dstart[j] = os[j] - msel.start[j];
						cur.dsize[j]  = msel.count[j];
						cur.mstart[j] = os[j] - req.sels->starts[i][j];
						cur.msize[j]  = req.sels->counts[i][j];
						cur.count[j]  = oc[j];
					}
					cur.info  = req.info;
					cur.foff  = ent.foff;
					cur.fsize = ent.fsize;
					cur.doff  = msel.doff;
					cur.xsize = ent.dsize;
					cur.xbuf  = req.xbuf + soff;
					ret.push_back (cur);
				}
			}
		}
		soff += req.sels->get_sel_size(i);
	}

	return err;
}