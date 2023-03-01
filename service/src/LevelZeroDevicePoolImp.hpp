/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LEVELZERODEVICEPOOLIMP_HPP_INCLUDE
#define LEVELZERODEVICEPOOLIMP_HPP_INCLUDE

#include <string>
#include <cstdint>

#include "LevelZeroDevicePool.hpp"
#include "LevelZero.hpp"

#include "geopm_time.h"

namespace geopm
{
    class LevelZeroDevicePoolImp : public LevelZeroDevicePool
    {
        public:
            LevelZeroDevicePoolImp();
            LevelZeroDevicePoolImp(LevelZero &levelzero);
            virtual ~LevelZeroDevicePoolImp() = default;
            int num_gpu(int domain_type) const override;

            double frequency_status(int domain, unsigned int domain_idx,
                                    int l0_domain) const override;
            double frequency_efficient(int domain, unsigned int domain_idx,
                                       int l0_domain) const override;
            double frequency_min(int domain, unsigned int domain_idx,
                                 int l0_domain) const override;
            double frequency_max(int domain, unsigned int domain_idx,
                                 int l0_domain) const override;
            double frequency_step(int domain, unsigned int domain_idx,
                                  int l0_domain) const override;
            uint32_t frequency_throttle_reasons(int domain, unsigned int domain_idx,
                                                int l0_domain) const override;
            std::pair <double, double> frequency_range(int domain,
                                                       unsigned int domain_idx,
                                                       int l0_domain) const override;

            double temperature_max(int domain, unsigned int domain_idx,
                                   int l0_domain) const override;

            std::pair<uint64_t, uint64_t> active_time_pair(int domain,
                                                           unsigned int device_idx,
                                                           int l0_domain) const override;
            uint64_t active_time(int domain, unsigned int device_idx,
                                 int l0_domain) const override;
            uint64_t active_time_timestamp(int domain, unsigned int device_idx,
                                           int l0_domain) const override;
            int32_t power_limit_tdp(int domain, unsigned int domain_idx,
                                    int l0_domain) const override;
            int32_t power_limit_min(int domain, unsigned int domain_idx,
                                    int l0_domain) const override;
            int32_t power_limit_max(int domain, unsigned int domain_idx,
                                    int l0_domain) const override;
            std::pair<uint64_t, uint64_t> energy_pair(int domain,
                                                      unsigned int domain_idx,
                                                      int l0_domain) const override;
            uint64_t energy(int domain, unsigned int domain_idx, int l0_domain) const override;
            uint64_t energy_timestamp(int domain, unsigned int domain_idx,
                                      int l0_domain) const override;
            double performance_factor(int domain,
                                      unsigned int domain_idx,
                                      int l0_domain) const override;

            void frequency_control(int domain, unsigned int domain_idx,
                                   int l0_domain, double range_min,
                                   double range_max) const override;
            void performance_factor_control(int domain, unsigned int domain_idx,
                                            int l0_domain,
                                            double setting) const override;

            //ZET Metrics
            double metric_sample(int domain, unsigned int domain_idx,
                                 std::string metric_name) const override;
            uint32_t metric_update_rate(int domain, unsigned int domain_idx) const override;

            void metric_read(int domain, unsigned int domain_idx) const override;
//            void metric_polling_disable(void) override;
            void metric_update_rate_control(int domain, unsigned int domain_idx,
                                            uint32_t setting) const override;

        private:
            LevelZero &m_levelzero;

            void check_idx_range(int domain, unsigned int domain_idx) const;
            void check_domain_exists(int size, const char *func, int line) const;
            std::pair<unsigned int, unsigned int> subdevice_device_conversion(unsigned int idx) const;
    };
}
#endif
