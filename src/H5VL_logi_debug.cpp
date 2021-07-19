#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cassert>
#include <cstdio>

#include "H5VL_logi.hpp"
#include "H5VL_logi_debug.hpp"

int H5VL_log_debug_MPI_Type_create_subarray (int ndims,
											 const int array_of_sizes[],
											 const int array_of_subsizes[],
											 const int array_of_starts[],
											 int order,
											 MPI_Datatype oldtype,
											 MPI_Datatype *newtype) {
	int i;

	for (i = 0; i < ndims; i++) {
		assert (array_of_subsizes[i] > 0);
		assert (array_of_subsizes[i] + array_of_starts[i] <= array_of_sizes[i]);
		assert (array_of_starts[i] >= 0);
	}

	return MPI_Type_create_subarray (ndims, array_of_sizes, array_of_subsizes, array_of_starts,
									 order, oldtype, newtype);
}

#ifdef LOGVOL_DEBUG
void hexDump (char *desc, void *addr, size_t len, char *fname) {
	FILE *fp;

	fp = fopen (fname, "a");
	if (fp) { hexDump (desc, addr, len, fp); }

	fclose (fp);
}

void hexDump (char *desc, void *addr, size_t len) { hexDump (desc, addr, len, stdout); }

void hexDump (char *desc, void *addr, size_t len, FILE *fp) {
	size_t i;
	unsigned char buff[17];
	unsigned char *pc = (unsigned char *)addr;

	// Output description if given.
	if (desc != NULL) printf ("%s:\n", desc);

	// Process every byte in the data.
	for (i = 0; i < len; i++) {
		// Multiple of 16 means new line (with line offset).

		if ((i % 16) == 0) {
			// Just don't print ASCII for the zeroth line.
			if (i != 0) fprintf (fp, "  %s\n", buff);

			// Output the offset.
			fprintf (fp, "  %04lx ", i);
		}

		// Now the hex code for the specific character.
		fprintf (fp, " %02x", pc[i]);

		// And store a printable ASCII character for later.
		if ((pc[i] < 0x20) || (pc[i] > 0x7e)) {
			buff[i % 16] = '.';
		} else {
			buff[i % 16] = pc[i];
		}

		buff[(i % 16) + 1] = '\0';
	}

	// Pad out last line if not exactly 16 characters.
	while ((i % 16) != 0) {
		fprintf (fp, "   ");
		i++;
	}

	// And print the final ASCII bit.
	fprintf (fp, "  %s\n", buff);
}

static int ext_ref_cnt = 0;
int H5VL_logi_inc_ref (hid_t id) {
	ext_ref_cnt++;
	return H5Iinc_ref (id);
}

int H5VL_logi_dec_ref (hid_t id) {
	ext_ref_cnt--;
	assert (ext_ref_cnt >= 0);
	return H5Idec_ref (id);
}
#endif