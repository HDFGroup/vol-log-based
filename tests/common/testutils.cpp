/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */

#include <stdlib.h> /* getenv() */
#include <string.h> /* strcasestr() */

#include "testutils.hpp"

void check_env(vol_env *env) {
    char *env_str;

    env->connector = 0;
    env->log_env   = 0;
    env->cache_env = 0;
    env->async_env = 0;
    env->passthru  = 0;

    env_str = getenv("H5VL_LOG_PASSTHRU");
    if (env_str != NULL && env_str[0] == '1')
        env->passthru = 1;

    env_str = getenv("HDF5_VOL_CONNECTOR");
    if (env_str == NULL)
        /* env HDF5_VOL_CONNECTOR is not set */
        return;

    env->connector = 1;

    if (strcasestr(env_str, "LOG ") != NULL)
        env->log_env = 1;

    if (strcasestr(env_str, "under_vol=512") != NULL)
        env->async_env = 1;

    if (strcasestr(env_str, "under_vol=513") != NULL)
        env->cache_env = 1;
}

