/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "geopm_agent.h"
#include "geopm_error.h"

int main(int argc, char *argv[])
{
    int err = geopm_agent_enforce_policy();
    if (err) {
        char err_msg[GEOPM_MESSAGE_MAX];
        geopm_error_message(err, err_msg, GEOPM_MESSAGE_MAX);
        printf("enforce policy failed: %s\n", err_msg);
    }
    return err;
}
