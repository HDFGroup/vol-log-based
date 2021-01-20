#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <hdf5.h>
#include <mpi.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "H5VL_log_file.hpp"
#include "H5VL_logi_nb.hpp"
#include "H5VL_logi_zip.hpp"
#include "h5replay.hpp"
#include "h5replay_meta.hpp"

herr_t h5replay_parse_meta (int rank,
							int np,
							hid_t lgid,
							int nmdset,
							std::vector<dset_info> &dsets,
							std::vector<req_block> &reqs) {
	herr_t err = 0;
	int i, j, k, l;
	hid_t did = -1;
	MPI_Offset nsec;
	hid_t dsid, msid;
	hsize_t start, count, one = 1;
	meta_sec sec;
	char *ep;
	char *zbuf = NULL;

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
		err = H5Dread (did, H5T_NATIVE_B8, dsid, msid, H5P_DEFAULT, &nsec);

		// Dividing jobs
		// Starting section
		start = rank * nsec / np;
		count = sizeof (MPI_Offset);
		err	  = H5Sselect_hyperslab (dsid, H5S_SELECT_SET, &start, NULL, &one, &count);
		CHECK_ERR
		err = H5Dread (did, H5T_NATIVE_B8, dsid, msid, H5P_DEFAULT, &(sec.start));
		if (rank == 0) {  // We save the end of each section, the start of each section is the end
						  // of the section list
			sec.start = sizeof (MPI_Offset) * (nsec + 1);
		}
		// Ending section
		if (nsec > np) {  // More section than processes, likely
			// No process sharing a section
			sec.stride = 1;
			sec.off	   = 0;

			start = (rank + 1) * nsec / np;
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
		}
		err = H5Sselect_hyperslab (dsid, H5S_SELECT_SET, &start, NULL, &one, &count);
		CHECK_ERR
		err = H5Dread (did, H5T_NATIVE_B8, dsid, msid, H5P_DEFAULT, &(sec.end));

		// Read metadata
		start = sec.start;
		count = sec.end - sec.start;
		err	  = H5Sselect_hyperslab (dsid, H5S_SELECT_SET, &start, NULL, &one, &count);
		CHECK_ERR
		sec.buf = (char *)malloc (count);
		err		= H5Dread (did, H5T_NATIVE_B8, dsid, msid, H5P_DEFAULT, sec.buf);

		// Close the metadata dataset
		H5Sclose (dsid);
		dsid = -1;
		H5Dclose (did);
		did = -1;

		// Parse the metadata
		for (j = 0; ep < sec.buf + count; j++) {
			H5VL_logi_meta_hdr *hdr = (H5VL_logi_meta_hdr *)ep;
			if ((j - sec.off) % sec.stride == 0) {
				int nsel;
				MPI_Offset *bp;
				MPI_Offset *dsteps;
				req_block req;

				// Extract the metadata
				if (hdr->flag & H5VL_LOGI_META_FLAG_MUL_SEL) {
					nsel = *((int *)hdr + 1);

					if (hdr->flag & H5VL_LOGI_META_FLAG_SEL_DEFLATE) {
#ifdef ENABLE_ZLIB
						int inlen, clen;
						MPI_Offset bsize;  // Size of all zbuf
						MPI_Offset esize;  // Size of a block
										   // Calculate buffer size;
						esize = 0;
						bsize = 0;
						if (hdr->flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {  // Logical location
							esize += nsel * sizeof (MPI_Offset) * 2;
							bsize += sizeof (MPI_Offset) * (ndims[hdr->did] - 1);
						} else {
							esize += nsel * sizeof (MPI_Offset) * 2 * ndims[hdr->did];
						}
						if (hdr->flag & H5VL_LOGI_META_FLAG_MUL_SELX) {	 // Physical location
							esize += sizeof (MPI_Offset) * 2;
						} else {
							bsize += sizeof (MPI_Offset) * 2;
						}
						bsize += esize * nsel;

						zbuf = (char *)malloc (bsize);

						inlen = hdr->meta_size - sizeof (H5VL_logi_meta_hdr) - sizeof (int);
						clen  = bsize;
						err	  = H5VL_log_zip_compress (
							  ep + sizeof (H5VL_logi_meta_hdr) + sizeof (int), inlen, zbuf, &clen);
						CHECK_ERR
#else
						RET_ERR ("Comrpessed Metadata Not Supported")
#endif
					} else {
						zbuf = (char *)(ep + sizeof (H5VL_logi_meta_hdr) + sizeof (int));
					}
				} else {
					nsel = 1;
					zbuf = (char *)(hdr + 1);
				}

				// Convert to req
				bp = (MPI_Offset *)zbuf;
				if (hdr->flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
					dsteps = bp;
					bp += ndims[hdr->did] - 1;
				}

				req.doff = 0;
				req.did	 = hdr->did;
				if (hdr->flag & H5VL_LOGI_META_FLAG_MUL_SELX) {
					req.foff = *((MPI_Offset *)bp);
					bp++;
					req.fsize = *((MPI_Offset *)bp);
					bp++;
				}

				for (k = 0; k < nsel; k++) {
					if (hdr->flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
						MPI_Offset off;

						// Decode start
						off = *((MPI_Offset *)bp);
						bp++;
						for (l = 0; l < ndims[req.did]; l++) {
							req.start[l] = off / dsteps[l];
							off %= dsteps;
						}
						// Decode count
						off = *((MPI_Offset *)bp);
						bp++;
						for (l = 0; l < ndims[req.did]; l++) {
							req.count[l] = off / dsteps[l];
							off %= dsteps;
						}
					} else {
						memcpy (req.start, bp, sizef (MPI_Offset) * ndims[req.did]);
						bp += sizef (MPI_Offset) * ndims[req.did];
						memcpy (req.start, bp, sizef (MPI_Offset) * ndims[req.did]);
						bp += sizef (MPI_Offset) * ndims[req.did];
					}

					if (!(hdr->flag & H5VL_LOGI_META_FLAG_MUL_SELX)) {
						req.foff = *((MPI_Offset *)bp);
						bp++;
						req.fsize = *((MPI_Offset *)bp);
						bp++;
					} else {
						req.fsize = 0;	// 0 menas sharing with previous req
					}

					reqs[req.did].push_back (req);
					if (hdr->flag & H5VL_LOGI_META_FLAG_SEL_DEFLATE) {
						free (zbuf);
						zbuf = NULL;
					}
				}
			}
			ep += hdr->meta_size;
		}
	}
err_out:;
	if (zbuf) { free (zbuf); }
	if (did >= 0) { H5Fclose (did); }
	if (msid >= 0) { H5Sclose (msid); }
	if (dsid >= 0) { H5Sclose (dsid); }
	return err;
}
