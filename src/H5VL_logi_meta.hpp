#pragma once
//
#include <functional>
#include <vector>
//
#include "H5VL_logi_dataspace.hpp"
#include "H5VL_logi_err.hpp"
#include "H5VL_logi_zip.hpp"

#define H5VL_LOGI_META_FLAG_MUL_SEL		0x01
#define H5VL_LOGI_META_FLAG_MUL_SELX	0x02
#define H5VL_LOGI_META_FLAG_SEL_ENCODE	0x04
#define H5VL_LOGI_META_FLAG_SEL_DEFLATE 0x08

typedef struct H5VL_logi_meta_hdr {
	int meta_size;	// Size of the metadata entry
	int did;		// Target dataset ID
	int flag;
} H5VL_logi_meta_hdr;

typedef struct H5VL_logi_metasel_t {
	hsize_t start[H5S_MAX_RANK];
	hsize_t count[H5S_MAX_RANK];
} H5VL_logi_metasel_t;

typedef struct H5VL_logi_metablock_t {
	H5VL_logi_meta_hdr hdr;
	std::vector<H5VL_logi_metasel_t> sels;
	MPI_Offset foff;
	size_t fsize;
} H5VL_logi_metablock_t;

inline void H5VL_logi_sel_decode (int ndim, MPI_Offset *dsteps, MPI_Offset off, hsize_t *cord) {
	int i;
	for (i = 0; i < ndim; i++) {
		cord[i] = off / dsteps[i];
		off %= dsteps[i];
	}
}

inline void H5VL_logi_sel_encode (int ndim, MPI_Offset *dsteps, hsize_t *cord, MPI_Offset *off) {
	int i;
	*off = 0;
	for (i = 0; i < ndim; i++) {
		*off += cord[i] * dsteps[i];  // Ending offset of the bounding box
	}
}


template <class T>
herr_t H5VL_logi_metaentry_decode (int ndim, void *ent, std::vector<T> &blocks) {
	herr_t err = 0;
	int i, j;
	int nsel;
	MPI_Offset *bp;
	MPI_Offset dsteps[H5S_MAX_RANK];
	char *zbuf	   = NULL;
	bool zbufalloc = false;
	T block;
	char *bufp = (char *)ent;

	block.hdr = *((H5VL_logi_meta_hdr *)bufp);
	bufp += sizeof (H5VL_logi_meta_hdr);

	// Extract the metadata
	if (block.hdr.flag & H5VL_LOGI_META_FLAG_MUL_SEL) {
		nsel = *((int *)bufp);
		bufp += sizeof (int);

		if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_DEFLATE) {
#ifdef ENABLE_ZLIB
			int inlen, clen;
			MPI_Offset bsize;  // Size of all zbuf
			MPI_Offset esize;  // Size of a block
							   // Calculate buffer size;
			esize = 0;
			bsize = 0;
			if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {	// Logical location
				esize += nsel * sizeof (MPI_Offset) * 2;
				bsize += sizeof (MPI_Offset) * (ndim - 1);
			} else {
				esize += nsel * sizeof (MPI_Offset) * 2 * ndim;
			}
			if (block.hdr.flag & H5VL_LOGI_META_FLAG_MUL_SELX) {  // Physical location
				esize += sizeof (MPI_Offset) * 2;
			} else {
				bsize += sizeof (MPI_Offset) * 2;
			}
			bsize += esize * nsel;

			zbuf	  = (char *)malloc (bsize);
			zbufalloc = true;

			inlen = block.hdr.meta_size - sizeof (H5VL_logi_meta_hdr) - sizeof (int);
			clen  = bsize;
			err	  = H5VL_log_zip_compress (bufp, inlen, zbuf, &clen);
			CHECK_ERR
#else
			RET_ERR ("Comrpessed Metadata Support Not Enabled")
#endif
		} else {
			zbuf	  = bufp;
			zbufalloc = false;
		}
	} else {
		nsel	  = 1;
		zbuf	  = bufp;
		zbufalloc = false;
	}

	// Convert to req
	bp = (MPI_Offset *)zbuf;
	if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
		memcpy (dsteps, bp, sizeof (MPI_Offset) * (ndim - 1));
		dsteps[ndim - 1] = 1;
		bp += ndim - 1;
	}

	if (block.hdr.flag & H5VL_LOGI_META_FLAG_MUL_SELX) {
		for (i = 0; i < nsel; i++) {
			block.foff = *((MPI_Offset *)bp);
			bp++;
			block.fsize = *((MPI_Offset *)bp);
			bp++;

			block.sels.resize (1);

			if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
				MPI_Offset off;

				// Decode start
				H5VL_logi_sel_decode (ndim, dsteps, *((MPI_Offset *)bp), block.sels[0].start);
				bp++;

				// Decode count
				H5VL_logi_sel_decode (ndim, dsteps, *((MPI_Offset *)bp), block.sels[0].count);
				bp++;
			} else {
				memcpy (block.sels[0].start, bp, sizeof (MPI_Offset) * ndim);
				bp += ndim;
				memcpy (block.sels[0].count, bp, sizeof (MPI_Offset) * ndim);
				bp += ndim;
			}
			blocks.push_back (block);
		}
	} else {
		block.foff = *((MPI_Offset *)bp);
		bp++;
		block.fsize = *((MPI_Offset *)bp);
		bp++;

		block.sels.resize (nsel);

		// Decode start
		for (i = 0; i < nsel; i++) {
			if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
				H5VL_logi_sel_decode (ndim, dsteps, *((MPI_Offset *)bp), block.sels[i].start);
				bp++;
			} else {
				memcpy (block.sels[i].start, bp, sizeof (MPI_Offset) * ndim);
				bp += ndim;
			}
		}
		// Decode count
		for (i = 0; i < nsel; i++) {
			if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
				H5VL_logi_sel_decode (ndim, dsteps, *((MPI_Offset *)bp), block.sels[i].count);
				bp++;
			} else {
				memcpy (block.sels[i].count, bp, sizeof (MPI_Offset) * ndim);
				bp += ndim;
			}
		}

		blocks.push_back (block);
	}
err_out:;
	if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_DEFLATE) { free (zbuf); }

	return err;
}

template <class T>
inline MPI_Offset H5VL_logi_get_metaentry_size (int ndim, H5VL_logi_meta_hdr &hdr, int nsel) {
	MPI_Offset size;

	size = sizeof (H5VL_logi_meta_hdr);	 // Header
	if (hdr.flag & H5VL_LOGI_META_FLAG_MUL_SEL) {
		size += sizeof (int);  // N
	}
	if (hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
		size += sizeof (MPI_Offset) * (ndim - 1 + nsel * 2) + sizeof (MPI_Offset) * 2;
	} else {
		size += sizeof (MPI_Offset) * ((MPI_Offset)ndim * (MPI_Offset)nsel * 2) +
				sizeof (MPI_Offset) * 2;
	}

	return size;
}
struct H5VL_log_dset_info_t;
struct H5VL_logi_meta_hdr;
herr_t H5VL_logi_metaentry_encode (H5VL_log_dset_info_t &dset,
								   H5VL_logi_meta_hdr &hdr,
								   int nsel,
								   hsize_t **starts,
								   hsize_t **counts,
								   void *meta);

herr_t H5VL_logi_metaentry_encode (H5VL_log_dset_info_t &dset,
								   H5VL_logi_meta_hdr &hdr,
								   std::vector<H5VL_log_selection> sels,
								   void *meta);