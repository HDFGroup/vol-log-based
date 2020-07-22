#include <stdlib.h>
#include "logvol_profiling.hpp"
#include "logvol_internal.hpp"



static double tmax[NTIMER], tmin[NTIMER], tmean[NTIMER], tvar[NTIMER], tvar_local[NTIMER];


const char * const tname[] = { 
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
                                    "H5VL_log_dataset_create",
                                    "H5VL_log_dataset_open",
                                    "H5VL_log_dataset_read",
                                    "H5VL_log_dataset_write",
                                    "H5VL_log_dataset_get",
                                    "H5VL_log_dataset_specific",
                                    "H5VL_log_dataset_optional",
                                    "H5VL_log_dataset_close",
                                    "H5VL_log_att_create",
                                    "H5VL_log_att_open",
                                    "H5VL_log_att_read",
                                    "H5VL_log_att_write",
                                    "H5VL_log_att_get",
                                    "H5VL_log_att_specific",
                                    "H5VL_log_att_optional",
                                    "H5VL_log_att_close",
                                    };

void H5VL_log_profile_add_time(H5VL_log_file_t *fp, int id, double t){
    if (id > NTIMER){
        return;
    }
    fp->tlocal[id] += t;
    fp->clocal[id]++;
}

void H5VL_log_profile_sub_time(H5VL_log_file_t *fp, int id, double t){
    if (id > NTIMER){
        return;
    }
    fp->tlocal[id] -= t;
}

// Note: This only work if everyone calls H5Fclose
void H5VL_log_profile_print(H5VL_log_file_t *fp){
    int i;
    int np, rank, flag;

    MPI_Initialized(&flag);
    if (!flag){
        MPI_Init(NULL, NULL);
    }

    MPI_Comm_size(fp->comm, &np);
    MPI_Comm_rank(fp->comm, &rank);

    MPI_Reduce(fp->tlocal, tmax, NTIMER, MPI_DOUBLE, MPI_MAX, 0, fp->comm);
    if (rank == 0){
        for(i = 0; i < NTIMER; i++){
            printf("#%%$: %s_time_mean: %lf\n", tname[i], tmean[i]);
            printf("#%%$: %s_time_max: %lf\n", tname[i], tmax[i]);
            printf("#%%$: %s_time_min: %lf\n", tname[i], tmin[i]);
            printf("#%%$: %s_time_var: %lf\n\n", tname[i], tvar[i]);
        }
    }

    MPI_Reduce(fp->clocal, tmax, NTIMER, MPI_DOUBLE, MPI_MAX, 0, fp->comm);
    MPI_Reduce(fp->clocal, tmin, NTIMER, MPI_DOUBLE, MPI_MIN, 0, fp->comm);
    MPI_Allreduce(fp->clocal, tmean, NTIMER, MPI_DOUBLE, MPI_SUM, fp->comm);
    for(i = 0; i < NTIMER; i++){
        tmean[i] /= np;
        tvar_local[i] = (fp->clocal[i] - tmean[i]) * (fp->clocal[i] - tmean[i]);
    }
    MPI_Reduce(tvar_local, tvar, NTIMER, MPI_DOUBLE, MPI_SUM, 0, fp->comm);

    if (rank == 0){
        for(i = 0; i < NTIMER; i++){
            printf("#%%$: %s_count_mean: %lf\n", tname[i], tmean[i]);
            printf("#%%$: %s_count_max: %lf\n", tname[i], tmax[i]);
            printf("#%%$: %s_count_min: %lf\n", tname[i], tmin[i]);
            printf("#%%$: %s_count_var: %lf\n\n", tname[i], tvar[i]);
        }
    }
}
void H5VL_log_profile_reset(H5VL_log_file_t *fp){
    int i;
    
    for(i = 0; i < NTIMER; i++){
        fp->tlocal[i] = 0;
        fp->clocal[i] = 0;
    }
}
