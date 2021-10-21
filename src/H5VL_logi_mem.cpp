#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>

#include "H5VL_log_obj.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_mem.hpp"

/*-------------------------------------------------------------------------
 * Function:    H5VL__H5VL_log_new_obj
 *
 * Purpose:     Create a new log object for an underlying object
 *
 * Return:      Success:    Pointer to the new log object
 *              Failure:    NULL
 *
 * Programmer:  Quincey Koziol
 *              Monday, December 3, 2018
 *
 *-------------------------------------------------------------------------
 */
H5VL_log_obj_t *H5VL_log_new_obj (void *under_obj, hid_t uvlid) {
	H5VL_log_obj_t *new_obj;

	new_obj		   = (H5VL_log_obj_t *)calloc (1, sizeof (H5VL_log_obj_t));
	new_obj->uo	   = under_obj;
	new_obj->uvlid = uvlid;
	H5VL_logi_inc_ref (new_obj->uvlid);

	return new_obj;
} /* end H5VL__H5VL_log_new_obj() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__H5VL_log_free_obj
 *
 * Purpose:     Release a log object
 *
 * Note:	Take care to preserve the current HDF5 error stack
 *		when calling HDF5 API calls.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 * Programmer:  Quincey Koziol
 *              Monday, December 3, 2018
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_free_obj (H5VL_log_obj_t *obj) {
	// hid_t err_id;

	// err_id = H5Eget_current_stack();

	H5VL_logi_dec_ref (obj->uvlid);

	// H5Eset_current_stack(err_id);

	free (obj);

	return 0;
} /* end H5VL__H5VL_log_free_obj() */

#define H5VL_LOGI_BUFFER_MIN_BLOCK_SIZE 10485760
H5VL_logi_buffer::H5VL_logi_buffer () : H5VL_logi_buffer (H5VL_LOGI_BUFFER_MIN_BLOCK_SIZE) {}
H5VL_logi_buffer::H5VL_logi_buffer (size_t block_size) {
	if (block_size < H5VL_LOGI_BUFFER_MIN_BLOCK_SIZE) {
		block_size = H5VL_LOGI_BUFFER_MIN_BLOCK_SIZE;
	}
	this->block_size = block_size;
	cur.start = cur.cur = NULL;
	new_block ();
};

H5VL_logi_buffer::~H5VL_logi_buffer () {
	reset ();
	free (cur.start);
}

void H5VL_logi_buffer::new_block () {
	// Recycle current block
	if (cur.start) {
		if (cur.cur > cur.start) {	// Record in history
			blocks.push_back (cur);
		} else {  // Never used, replace directly
			free (cur.start);
		}
	}
	// Allocate buffer
	cur.start = cur.cur = (char *)malloc (block_size);
	if (cur.start == NULL) throw "OOM";
	cur.end = cur.start + block_size;
};
void H5VL_logi_buffer::reset () {
	for (auto &b : blocks) { free (b.start); }
	blocks.resize (0);
	cur.cur = cur.start;
}
char *H5VL_logi_buffer::reserve (size_t size) {
	char *ret;
	LOG_VOL_ASSERT (size < INT_MAX);
	if (cur.cur + size > cur.end) {
		if (block_size < size) { block_size = size; }
		try {
			new_block ();
		} catch (...) { return NULL; }
	}
	ret = cur.cur;
	cur.cur += size;
	return ret;
}
void H5VL_logi_buffer::reclaim (size_t size) {
	cur.cur -= size;
	LOG_VOL_ASSERT (cur.cur >= cur.start);
}

herr_t H5VL_logi_buffer::get_mem_type (MPI_Datatype &type) {
	herr_t err;
	int nentry;
	MPI_Aint *offs;
	int *lens;

	nentry = blocks.size ();
	if (cur.cur > cur.start) { nentry++; }

	offs = (MPI_Aint *)malloc (sizeof (MPI_Aint) * nentry);
	CHECK_PTR (offs)
	lens = (int *)malloc (sizeof (int) * nentry);
	CHECK_PTR (lens)

	nentry = 0;
	if (cur.cur > cur.start) {
		offs[nentry]   = (MPI_Aint) (cur.start);
		lens[nentry++] = cur.cur - cur.start;
	}

	for (auto &b : blocks) {
		offs[nentry]   = (MPI_Aint) (b.start);
		lens[nentry++] = b.cur - b.start;
	}

err_out:;
	return err;
}