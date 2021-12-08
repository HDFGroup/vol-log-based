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
#include <mpi.h>
//
#include "H5VL_log_filei.hpp"
#include "H5VL_logi_nb.hpp"
#include "H5VL_logi_util.hpp"
#include "h5ldump.hpp"

herr_t h5ldump_mdset (hid_t lgid,
					  std::string name,
					  std::vector<H5VL_log_dset_info_t> &dsets,
					  int indent) {
	herr_t err = 0;
	int i;
	hid_t did  = -1;				// Metadata dataset ID
	hid_t dsid = -1;				// Metadata dataset space ID
	hid_t msid = -1;				// Memory space ID
	hsize_t start, count, one = 1;	// Start and count to set dataspace selections
	MPI_Offset nsec;				// Number of processes writing to this metadata dataset
	MPI_Offset *offs = NULL;		// Offset of each metadata section
	uint8_t *buf;					// Metadata buffer
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
	buf = (uint8_t *)malloc (bsize);
	CHECK_PTR (buf);

	// Getting the metadata sections
	for (i = 0; i < nsec; i++) {
		start = offs[i];
		count = offs[i + 1] - start;
		err	  = H5Sselect_hyperslab (dsid, H5S_SELECT_SET, &start, NULL, &one, &count);
		CHECK_ERR
		start = 0;
		err	  = H5Sselect_hyperslab (msid, H5S_SELECT_SET, &start, NULL, &one, &count);
		CHECK_ERR
		err = H5Dread (did, H5T_NATIVE_B8, msid, dsid, H5P_DEFAULT, buf);
		CHECK_ERR

		std::cout << std::string (indent, ' ') << "Metadata section " << i << ": " << std::endl;
		err = h5ldump_mdsec (buf, count, dsets, indent + 4);
		CHECK_ERR
		// std::cout << std::string (indent, ' ') << "End metadata section " << i << ": " <<
		// std::endl;
	}

err_out:;
	if (did >= 0) { H5Dclose (did); }
	return err;
}

herr_t h5ldump_mdsec (uint8_t *buf,
					  size_t len,
					  std::vector<H5VL_log_dset_info_t> &dsets,
					  int indent) {
	herr_t err = 0;
	int i;
	uint8_t *bufp = buf;			  // Current decoding location in buf
	H5VL_logi_meta_hdr *hdr;		  // Header of current decoding entry
	H5VL_logi_metablock_t block;	  // Current metadata block
	MPI_Offset dsteps[H5S_MAX_RANK];  // corrdinate to offset encoding info in ent
	std::map<char *, std::vector<H5VL_logi_metasel_t>>
		bcache;	 // Cache for deduplicated metadata entry

	while (bufp < buf + len) {
		hdr = (H5VL_logi_meta_hdr *)bufp;

#ifdef WORDS_BIGENDIAN
		H5VL_logi_lreverse ((uint64_t *)bufp, (uint64_t *)(bufp + sizeof (H5VL_logi_meta_hdr)));
#endif

		// Have to parse all entries for reference purpose
		if (hdr->flag & H5VL_LOGI_META_FLAG_SEL_REF) {
			H5VL_logi_metaentry_ref_decode (dsets[hdr->did], bufp, block, bcache);
		} else {
			H5VL_logi_metaentry_decode (dsets[hdr->did], bufp, block, dsteps);

			// Insert to cache
			bcache[(char *)bufp] = block.sels;
		}

		std::cout << std::string (indent, ' ') << "Metadata entry at " << (off_t) (bufp - buf)
				  << ": " << std::endl;
		indent += 4;
		;

		std::cout << std::string (indent, ' ') << "Dataset ID: " << hdr->did
				  << "; Entry size: " << hdr->meta_size << std::endl;
		std::cout << std::string (indent, ' ') << "Data offset: " << hdr->foff
				  << "; Data size: " << hdr->fsize << std::endl;
		std::cout << std::string (indent, ' ') << "Flags: ";
		if (hdr->flag & H5VL_LOGI_META_FLAG_MUL_SEL) { std::cout << "multi-selection, "; }
		if (hdr->flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) { std::cout << "encoded, "; }
		if (hdr->flag & H5VL_LOGI_META_FLAG_SEL_DEFLATE) { std::cout << "comrpessed, "; }
		if (hdr->flag & H5VL_LOGI_META_FLAG_REC) { std::cout << "record, "; }
		if (hdr->flag & H5VL_LOGI_META_FLAG_SEL_REF) { std::cout << "duplicate, "; }
		std::cout << std::endl;
		if (hdr->flag & H5VL_LOGI_META_FLAG_SEL_REF) {
			// Get referenced selections
			auto roff = *((MPI_Offset *)(hdr + 1) + (block.hdr.flag & H5VL_LOGI_META_FLAG_REC));
			std::cout << std::string (indent, ' ') << "Referenced entry offset: " << (off_t)bufp
					  << std::endl;
		}
		if (hdr->flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
			std::cout << std::string (indent, ' ') << "Encoding slice size: (";
			for (i = 0; i < dsets[hdr->did].ndim - 1; i++) { std::cout << dsteps[i] << ", "; }
			std::cout << ")" << std::endl;
		}
		std::cout << std::string (indent, ' ') << "Selections: " << block.sels.size () << " blocks"
				  << std::endl;
		indent += 4;
		;
		for (auto &s : block.sels) {
			std::cout << std::string (indent, ' ') << s.doff << ": ( ";
			for (i = 0; i < dsets[hdr->did].ndim; i++) { std::cout << s.start[i] << ", "; }
			std::cout << ") : ( ";
			for (i = 0; i < dsets[hdr->did].ndim; i++) { std::cout << s.count[i] << ", "; }
			std::cout << ")" << std::endl;
		}
		indent -= 4;
		;

		indent -= 4;
		;
		// std::cout << std::string (indent, ' ') << "End metadata entry : " << (off_t) (bufp - buf)
		// << std::endl;

		bufp += hdr->meta_size;
	}

err_out:;
	return err;
}
