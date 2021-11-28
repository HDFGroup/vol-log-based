#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
//
#include <hdf5.h>
//
#include "H5VL_log_filei.hpp"
#include "H5VL_logi_nb.hpp"
#include "unistd.h"

herr_t h5ldump_mdset (hid_t lgid, std::string name, std::vector<H5VL_log_dset_info_t> &dsets, int indent) {
	herr_t err = 0;
	int i;
	hid_t did  = -1;				// Metadata dataset ID
	hid_t dsid = -1;				// Metadata dataset space ID
	hid_t msid = -1;				// Memory space ID
	hsize_t start, count, one = 1;	// Start and count to set dataspace selections
	MPI_Offset nsec;				// Number of processes writing to this metadata dataset
	MPI_Offset *offs = NULL;		// Offset of each metadata section
	uint8_t *buf;						// Metadata buffer
	size_t bsize = 0;				// Size of metadata buffer

	did = H5Dopen2 (lgid, name.c_str (), H5P_DEFAULT);
	CHECK_ID (did)

	dsid = H5Dget_space (did);
	CHECK_ID (dsid)
	msid = H5Scopy (dsid);
	CHECK_ID (msid)

	// N sections (process), first 8 bytes
	start = 0;
	count = sizeof (MPI_Offset);
	err	  = H5Sselect_hyperslab (dsid, H5S_SELECT_SET, &start, NULL, &one, &count);
	CHECK_ERR
	err = H5Dread (did, H5T_NATIVE_B8, H5S_ALL, dsid, H5P_DEFAULT, &nsec);
	CHECK_ERR

	std::cout << std::string (indent, ' ') << "Number of metadata sections: " << nsec << std::endl;

	// Getting the offsets of sections
	start = sizeof (MPI_Offset);
	count = sizeof (MPI_Offset) * nsec;
	offs  = (MPI_Offset *)malloc (sizeof (MPI_Offset) * (nsec + 1));
	err	  = H5Sselect_hyperslab (dsid, H5S_SELECT_SET, &start, NULL, &one, &count);
	CHECK_ERR
	err = H5Dread (did, H5T_NATIVE_B8, H5S_ALL, dsid, H5P_DEFAULT, offs);
	CHECK_ERR
	offs[0] = start + count;

	// Allocate metadata buffer
	for (i = 0; i < nsec; i++) {
		if (bsize < offs[i + 1] - offs[i]) bsize = offs[i + 1] - offs[i];
	}
	buf=(uint8_t*)malloc(bsize);
	CHECK_PTR(buf);

	// Getting the metadata sections
	for (i = 0; i < nsec; i++) {
		start = offs[i];
		count = offs[i+1]-start;
		err	  = H5Sselect_hyperslab (dsid, H5S_SELECT_SET, &start, NULL, &one, &count);
		CHECK_ERR
		start = 0;
		err	  = H5Sselect_hyperslab (msid, H5S_SELECT_SET, &start, NULL, &one, &count);
		CHECK_ERR
		err = H5Dread (did, H5T_NATIVE_B8, H5S_ALL, dsid, H5P_DEFAULT, buf);
		CHECK_ERR

		err= h5ldump_mdsec (buf, count, dsets, indent);
		CHECK_ERR
	}

err_out:;
	if (did >= 0) { H5Dclose (did); }
	return err;
}

herr_t h5ldump_mdsec (uint8_t *buf, size_t len, std::vector<H5VL_log_dset_info_t> &dsets, int indent) {
	herr_t err=0;
	uint8_t *bufp=buf;						// Current decoding location in buf
	H5VL_logi_meta_hdr *hdr;// Header of current decoding entry
	H5VL_logi_metablock_t block;

	while(bufp<buf+len){
	hdr = (H5VL_logi_meta_hdr *)bufp;

	// Have to parse all entries for reference purpose
	if (hdr->flag & H5VL_LOGI_META_FLAG_SEL_REF) {
		H5VL_logi_metaentry_ref_decode (dsets[hdr->did], bufp, block, bcache);
	} else {
		H5VL_logi_metaentry_decode (dsets[hdr->did], bufp, block);

		// Insert to cache
		bcache[ep] = block.sels;
	}
	ep += hdr->meta_size;

	// Only insert to index if we are responsible to the entry
	if ((j - sec.off) % sec.stride == 0) {
		// Insert to the index
		reqs[hdr->did].insert (block);
	}

		bufp+=hdr->meta_size;
	}

	err_out:;
	return err;
}


	// Copy data objects
	copy_arg.dsets.resize (ndset);
	copy_arg.fid = foutid;
	err = H5Ovisit3 (finid, H5_INDEX_CRT_ORDER, H5_ITER_INC, h5replay_copy_handler, &copy_arg,
					 H5O_INFO_ALL);
	CHECK_ERR