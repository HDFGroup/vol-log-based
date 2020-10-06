#include "H5VL_logi_profiling.hpp"
#include <mpi.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include "H5VL_log_file.hpp"
#include "H5VL_logi.hpp"
static double tmax[NTIMER], tmin[NTIMER], tmean[NTIMER], tvar[NTIMER], tvar_local[NTIMER];
const char *const tname[] = {
	"H5VL_log_file_create",
	"H5VL_log_file_open",
	"H5VL_log_file_get",
	"H5VL_log_file_specific",
	"H5VL_log_file_optional",
	"H5VL_log_file_close",
	"H5VL_log_group_create",
	"H5VL_log_group_open",
	"H5VL_log_group_get",
	"H5VL_log_group_specific",
	"H5VL_log_group_optional",
	"H5VL_log_group_close",
	"H5VL_log_att_create",
	"H5VL_log_att_open",
	"H5VL_log_att_read",
	"H5VL_log_att_write",
	"H5VL_log_att_get",
	"H5VL_log_att_specific",
	"H5VL_log_att_optional",
	"H5VL_log_att_close",
	"H5VL_log_dataset_create",
	"H5VL_log_dataset_open",
	"H5VL_log_dataset_read",
	"H5VL_log_dataset_write",
	"H5VL_log_dataset_write_init",
	"H5VL_log_dataset_write_start_count",
	"H5VL_log_dataset_write_pack",
	"H5VL_log_dataset_write_convert",
	"H5VL_log_dataset_write_finalize",
	"H5VL_log_dataset_get",
	"H5VL_log_dataset_specific",
	"H5VL_log_dataset_optional",
	"H5VL_log_dataset_close",
	"H5VLfile_create",
	"H5VLfile_open",
	"H5VLfile_get",
	"H5VLfile_specific",
	"H5VLfile_optional",
	"H5VLfile_close",
	"H5VLgroup_create",
	"H5VLgroup_open",
	"H5VLgroup_get",
	"H5VLgroup_specific",
	"H5VLgroup_optional",
	"H5VLgroup_close",
	"H5VLatt_create",
	"H5VLatt_open",
	"H5VLatt_read",
	"H5VLatt_write",
	"H5VLatt_get",
	"H5VLatt_specific",
	"H5VLatt_optional",
	"H5VLatt_close",
	"H5VLdataset_create",
	"H5VLdataset_open",
	"H5VLdataset_read",
	"H5VLdataset_write",
	"H5VLdataset_get",
	"H5VLdataset_specific",
	"H5VLdataset_optional",
	"H5VLdataset_close",
	"H5VL_log_filei_flush",
	"H5VL_log_filei_metaflush",
	"H5VL_log_filei_metaflush_init",
	"H5VL_log_filei_metaflush_pack",
	"H5VL_log_filei_metaflush_zip",
	"H5VL_log_filei_metaflush_sync",
	"H5VL_log_filei_metaflush_create",
	"H5VL_log_filei_metaflush_write",
	"H5VL_log_filei_metaflush_close",
	"H5VL_log_filei_metaflush_barrier",
	"H5VL_log_filei_metaflush_finalize",
	"H5VL_log_filei_metaflush_size",
	"H5VL_log_filei_metaflush_size_zip",
	"H5VL_log_filei_metaupdate",
	"H5VL_log_dataseti_readi_gen_rtypes",
	"H5VL_log_dataseti_open_with_uo",
	"H5VL_log_dataseti_wrap",
	"H5VL_logi_get_dataspace_sel_type",
	"H5VL_logi_get_dataspace_selection",
	"H5VL_log_nb_flush_read_reqs",
	"H5VL_log_nb_flush_write_reqs",
	"H5VL_log_nb_flush_write_reqs_size",
};

void H5VL_log_profile_add_time (void *file, int id, double t) {
	H5VL_log_file_t *fp = (H5VL_log_file_t *)file;

	assert (id >= 0 && id < NTIMER);
	fp->tlocal[id] += t;
	fp->clocal[id]++;
}

void H5VL_log_profile_sub_time (void *file, int id, double t) {
	H5VL_log_file_t *fp = (H5VL_log_file_t *)file;

	assert (id >= 0 && id < NTIMER);
	fp->tlocal[id] -= t;
}

// Note: This only work if everyone calls H5Fclose
void H5VL_log_profile_print (void *file) {
	int i;
	int np, rank, flag;
	H5VL_log_file_t *fp = (H5VL_log_file_t *)file;

	MPI_Initialized (&flag);
	if (!flag) { MPI_Init (NULL, NULL); }

	MPI_Comm_size (fp->comm, &np);
	MPI_Comm_rank (fp->comm, &rank);

	MPI_Reduce (fp->tlocal, tmax, NTIMER, MPI_DOUBLE, MPI_MAX, 0, fp->comm);
	MPI_Reduce (fp->tlocal, tmin, NTIMER, MPI_DOUBLE, MPI_MIN, 0, fp->comm);
	MPI_Allreduce (fp->tlocal, tmean, NTIMER, MPI_DOUBLE, MPI_SUM, fp->comm);
	for (i = 0; i < NTIMER; i++) {
		tmean[i] /= np;
		tvar_local[i] = (fp->tlocal[i] - tmean[i]) * (fp->tlocal[i] - tmean[i]);
	}
	MPI_Reduce (tvar_local, tvar, NTIMER, MPI_DOUBLE, MPI_SUM, 0, fp->comm);

	if (rank == 0) {
		for (i = 0; i < NTIMER; i++) {
			printf ("#%%$: %s_time_mean: %lf\n", tname[i], tmean[i]);
			printf ("#%%$: %s_time_max: %lf\n", tname[i], tmax[i]);
			printf ("#%%$: %s_time_min: %lf\n", tname[i], tmin[i]);
			printf ("#%%$: %s_time_var: %lf\n\n", tname[i], tvar[i]);
		}
	}

	MPI_Reduce (fp->clocal, tmax, NTIMER, MPI_DOUBLE, MPI_MAX, 0, fp->comm);
	MPI_Reduce (fp->clocal, tmin, NTIMER, MPI_DOUBLE, MPI_MIN, 0, fp->comm);
	MPI_Allreduce (fp->clocal, tmean, NTIMER, MPI_DOUBLE, MPI_SUM, fp->comm);
	for (i = 0; i < NTIMER; i++) {
		tmean[i] /= np;
		tvar_local[i] = (fp->clocal[i] - tmean[i]) * (fp->clocal[i] - tmean[i]);
	}
	MPI_Reduce (tvar_local, tvar, NTIMER, MPI_DOUBLE, MPI_SUM, 0, fp->comm);

	if (rank == 0) {
		for (i = 0; i < NTIMER; i++) {
			printf ("#%%$: %s_count_mean: %lf\n", tname[i], tmean[i]);
			printf ("#%%$: %s_count_max: %lf\n", tname[i], tmax[i]);
			printf ("#%%$: %s_count_min: %lf\n", tname[i], tmin[i]);
			printf ("#%%$: %s_count_var: %lf\n\n", tname[i], tvar[i]);
		}
	}
}
void H5VL_log_profile_reset (void *file) {
	int i;
	H5VL_log_file_t *fp = (H5VL_log_file_t *)file;

	for (i = 0; i < NTIMER; i++) {
		fp->tlocal[i] = 0;
		fp->clocal[i] = 0;
	}
}
