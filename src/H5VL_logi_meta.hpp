#pragma once
//
#include <functional>
#include <vector>
#include <unordered_map>
//
#include "H5VL_logi_dataspace.hpp"
//#include "H5VL_logi_idx.hpp"
#include "H5VL_logi_err.hpp"
#include "H5VL_logi_zip.hpp"

#define H5VL_LOGI_META_FLAG_MUL_SEL		0x01
#define H5VL_LOGI_META_FLAG_MUL_SELX	0x02
#define H5VL_LOGI_META_FLAG_SEL_ENCODE	0x04
#define H5VL_LOGI_META_FLAG_SEL_DEFLATE 0x08

typedef struct H5VL_logi_meta_hdr {
	int meta_size;	   // Size of the metadata entry
	int did;		   // Target dataset ID
	int flag;		   // Flag for other metadata, format, vertion ... etc
	MPI_Offset foff;   // File offset of the data
	MPI_Offset fsize;  // Size of the data in file
} __attribute__((packed)) H5VL_logi_meta_hdr; 

typedef struct H5VL_logi_metasel_t {
	hsize_t start[H5S_MAX_RANK];
	hsize_t count[H5S_MAX_RANK];
	MPI_Offset doff;
} H5VL_logi_metasel_t;

typedef struct H5VL_logi_metablock_t {
	H5VL_logi_meta_hdr hdr;
	std::vector<H5VL_logi_metasel_t> sels;
	size_t dsize;
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
/*
class H5VL_logi_wreq_hash {
	// A hash function used to hash a pair of any kind
	struct hash_pair {
		size_t operator() (const std::pair<void *, size_t> &p) const {
			int i;
			size_t ret = 0;
			size_t *val;
			size_t *end = (size_t *)((char *)(p.first) + p.second - p.second % sizeof (size_t));

			for (val = (size_t *)(p.first); val < end; val++) { ret ^= *val; }

			return ret;
		}
	};

	struct equal_pair {
		bool operator() (const std::pair<void *, size_t> &a, const std::pair<void *, size_t> &b) const {
			if (a.second != b.second) { return false; }
			return memcmp (a.first, b.first, a.second) == 0;
		}
	};

	std::unordered_map<std::pair<void *, size_t>, int, hash_pair, equal_pair>
		table;

	public:
	H5VL_logi_wreq_hash();
	~H5VL_logi_wreq_hash();

	int find(char *buf, size_t size);
	int add(char *buf, size_t size);
	void clear();
};
*/

struct H5VL_logi_idx_t;
struct H5VL_log_dset_info_t;
herr_t H5VL_logi_metaentry_decode (H5VL_log_dset_info_t &dset,
								   void *ent,
								   H5VL_logi_metablock_t &block);

inline MPI_Offset H5VL_logi_get_metaentry_size (int ndim, H5VL_logi_meta_hdr &hdr, int nsel) {
	MPI_Offset size;

	size = sizeof (H5VL_logi_meta_hdr);	 // Header
	if (hdr.flag & H5VL_LOGI_META_FLAG_MUL_SEL) {
		size += sizeof (int);  // N
	}
	if (hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
		size += sizeof (MPI_Offset) * (ndim - 1 + nsel * 2);
	} else {
		size += sizeof (MPI_Offset) * ((MPI_Offset)ndim * (MPI_Offset)nsel * 2);
	}

	return size;
}

struct H5VL_log_dset_info_t;
struct H5VL_logi_meta_hdr;
struct H5VL_log_selections;
herr_t H5VL_logi_metaentry_ref_decode (H5VL_log_dset_info_t &dset,
									   void *ent,
									   H5VL_logi_metablock_t &block,
									   std::vector<H5VL_logi_metasel_t> &sels);
