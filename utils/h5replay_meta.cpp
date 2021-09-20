#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>
//
#include <hdf5.h>
#include <mpi.h>
//
#include "H5VL_log_dataset.hpp"
#include "H5VL_log_file.hpp"
#include "H5VL_log_filei.hpp"
#include "H5VL_logi_idx.hpp"
#include "H5VL_logi_meta.hpp"
#include "H5VL_logi_zip.hpp"
#include "h5replay.hpp"
#include "h5replay_meta.hpp"

herr_t h5replay_parse_meta (int rank,
							int np,
							hid_t lgid,
							int nmdset,
							std::vector<dset_info> &dsets,
							std::vector<h5replay_idx_t> &reqs,
							int config) {
	herr_t err = 0;
	int i, j, k, l;
	hid_t did = -1;
	MPI_Offset nsec;
	hid_t dsid = -1, msid = -1;
	hsize_t start, count, one = 1;
	meta_sec sec;
	bool zbufalloc = false;
	char *ep;
	char *zbuf = NULL;
	H5VL_logi_metablock_t block;								 // Buffer of decoded metadata entry
	std::map<char *, std::vector<H5VL_logi_metasel_t> > bcache;	 // Cache for linked metadata entry

	// Memory space set to contiguous
	start = count = INT64_MAX - 1;
	msid		  = H5Screate_simple (1, &start, &count);

	// Read the metadata
	for (i = 0; i < nmdset; i++) {
		char mdname[16];

		sprintf (mdname, "_md_%d", i);
		did = H5Dopen2 (lgid, mdname, H5P_DEFAULT);
		CHECK_ID (did)

		dsid = H5Dget_space (did);
		CHECK_ID (dsid)

		// N sections
		start = 0;
		count = sizeof (MPI_Offset);
		err	  = H5Sselect_hyperslab (dsid, H5S_SELECT_SET, &start, NULL, &one, &count);
		CHECK_ERR
		err = H5Sselect_hyperslab (msid, H5S_SELECT_SET, &start, NULL, &one, &count);
		CHECK_ERR
		err = H5Dread (did, H5T_NATIVE_B8, msid, dsid, H5P_DEFAULT, &nsec);
		CHECK_ERR

		// Dividing jobs
		// Starting section
		start = rank * nsec / np * sizeof (MPI_Offset);
		count = sizeof (MPI_Offset);
		err	  = H5Sselect_hyperslab (dsid, H5S_SELECT_SET, &start, NULL, &one, &count);
		CHECK_ERR
		// Memspace already selected
		err = H5Dread (did, H5T_NATIVE_B8, msid, dsid, H5P_DEFAULT, &(sec.start));
		CHECK_ERR
		if (rank == 0) {  // We save the end of each section, the start of each section is the end
						  // of previous section
			sec.start = sizeof (MPI_Offset) * (nsec + 1);
		}
		// Ending section
		if (nsec >= np) {  // More section than processes, likely
			// No process sharing a section
			sec.stride = 1;
			sec.off	   = 0;

			// End
			start = ((rank + 1) * nsec / np) * sizeof (MPI_Offset);
		} else {  // Less sec than process
			int first, last;

			// First rank to carry this section
			first = np * start / nsec;
			if ((np * start) % nsec == 0) first++;
			// Last rank to carry this section
			last = np * (start + 1) / nsec;
			if ((np * (start + 1)) % nsec == 0) last--;
			sec.stride = last - first + 1;
			sec.off	   = rank - first;

			// End
			start += sizeof (MPI_Offset);
		}
		err = H5Sselect_hyperslab (dsid, H5S_SELECT_SET, &start, NULL, &one, &count);
		CHECK_ERR
		// Memspace already selected

		err = H5Dread (did, H5T_NATIVE_B8, msid, dsid, H5P_DEFAULT, &(sec.end));
		CHECK_ERR

		// Read metadata
		start = sec.start;
		count = sec.end - sec.start;
		err	  = H5Sselect_hyperslab (dsid, H5S_SELECT_SET, &start, NULL, &one, &count);
		CHECK_ERR
		start = 0;
		err	  = H5Sselect_hyperslab (msid, H5S_SELECT_SET, &start, NULL, &one, &count);
		CHECK_ERR
		sec.buf = (char *)malloc (count);
		err		= H5Dread (did, H5T_NATIVE_B8, msid, dsid, H5P_DEFAULT, sec.buf);

		// Close the metadata dataset
		H5Sclose (dsid);
		dsid = -1;
		H5Dclose (did);
		did = -1;

		// Parse the metadata
		ep = sec.buf;
		if (config & H5VL_FILEI_CONFIG_METADATA_SHARE) {  // Need to maintina cache if file contains
														  // referenced metadata entries
			for (j = 0; ep < sec.buf + count; j++) {
				H5VL_logi_meta_hdr *hdr = (H5VL_logi_meta_hdr *)ep;

				// Have to parse all entries for reference purpose
				if (hdr->flag & H5VL_LOGI_META_FLAG_SEL_REF) {
					MPI_Offset roff = *((MPI_Offset *)hdr + 1);

					H5VL_logi_metaentry_ref_decode (dsets[hdr->did], ep, block, bcache[ep + roff]);
				} else {
					H5VL_logi_metaentry_decode (dsets[hdr->did], ep, block);

					// Insert to cache
					bcache[ep] = block.sels;
				}
				ep += hdr->meta_size;

				// Only insert to index if we are responsible to the entry
				if ((j - sec.off) % sec.stride == 0) {
					// Insert to the index
					reqs[hdr->did].insert (block);
				}
			}

			// Clean up the cache, referenced are local to each section
			bcache.clear();
		} else {
			for (j = 0; ep < sec.buf + count; j++) {
				H5VL_logi_meta_hdr *hdr = (H5VL_logi_meta_hdr *)ep;
				if ((j - sec.off) % sec.stride == 0) {
					H5VL_logi_metaentry_decode (dsets[hdr->did], ep, block);
					ep += hdr->meta_size;

					// Insert to the index
					reqs[hdr->did].insert (block);
				}
			}
		}
	}
err_out:;
	if (zbufalloc && zbuf) { free (zbuf); }

	if (did >= 0) { H5Dclose (did); }
	if (msid >= 0) { H5Sclose (msid); }
	if (dsid >= 0) { H5Sclose (dsid); }
	return err;
}

herr_t h5replay_idx_t::clear () {
	this->entries.clear ();

	return 0;
}

herr_t h5replay_idx_t::insert (H5VL_logi_metablock_t &meta) {
	meta_block block;

	block.dsize = meta.dsize;
	block.foff	= meta.foff;
	block.fsize = meta.fsize;
	block.hdr	= meta.hdr;
	block.sels	= meta.sels;
	this->entries.push_back (block);

	return 0;
}

herr_t h5replay_idx_t::search (H5VL_log_rreq_t &req, std::vector<H5VL_log_idx_search_ret_t> &ret) {
	return 0;
}