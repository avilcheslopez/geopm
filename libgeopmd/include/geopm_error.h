/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_ERROR_H_INCLUDE
#define GEOPM_ERROR_H_INCLUDE

#include <stdlib.h>

#include "geopm_public.h"

#ifdef __cplusplus
extern "C" {
#endif

enum geopm_error_e {
    GEOPM_ERROR_RUNTIME = -1,
    GEOPM_ERROR_LOGIC = -2,
    GEOPM_ERROR_INVALID = -3,
    GEOPM_ERROR_FILE_PARSE = -4,
    GEOPM_ERROR_LEVEL_RANGE = -5,
    GEOPM_ERROR_NOT_IMPLEMENTED = -6,
    GEOPM_ERROR_PLATFORM_UNSUPPORTED = -7,
    GEOPM_ERROR_MSR_OPEN = -8,
    GEOPM_ERROR_MSR_READ = -9,
    GEOPM_ERROR_MSR_WRITE = -10,
    GEOPM_ERROR_AGENT_UNSUPPORTED = -11,
    GEOPM_ERROR_AFFINITY = -12,
    GEOPM_ERROR_NO_AGENT = -13,
    GEOPM_ERROR_DATA_STORE = -14,
};

/*
 * String length allocated for GEOPM messages (like C error messages
 * derived from C++ exceptions).  Has same value as PATH_MAX from
 * linux/limits.h for historical reasons.
 */
#define GEOPM_MESSAGE_MAX 4096ULL

/* Convert error number into an error message */
void GEOPM_PUBLIC
    geopm_error_message(int err, char *msg, size_t size);

#ifdef __cplusplus
}
#endif
#endif
