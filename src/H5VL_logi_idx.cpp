#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//
#include <mpi.h>
//
#include "H5VL_log_dataset.hpp"
#include "H5VL_logi_idx.hpp"
//
bool H5VL_log_idx_search_ret_t::operator< (const H5VL_log_idx_search_ret_t &rhs) const {
	int i;

	if (foff < rhs.foff)
		return true;
	else if (foff > rhs.foff)
		return false;

	for (i = 0; i < info->ndim; i++) {
		if (dstart[i] < rhs.dstart[i])
			return true;
		else if (dstart[i] > rhs.dstart[i])
			return false;
	}

	return false;
}
