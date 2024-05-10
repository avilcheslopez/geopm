#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


from integration.experiment.energy_efficiency import power_balancer_energy
from integration.apps.nekbone import nekbone


if __name__ == '__main__':

    app_conf = nekbone.NekboneAppConf()
    power_balancer_energy.main(app_conf)

