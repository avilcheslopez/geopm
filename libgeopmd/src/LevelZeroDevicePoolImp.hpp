/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LEVELZERODEVICEPOOLIMP_HPP_INCLUDE
#define LEVELZERODEVICEPOOLIMP_HPP_INCLUDE

#include <string>
#include <cstdint>
#include <map>

#include "LevelZeroDevicePool.hpp"
#include "LevelZero.hpp"

#include "geopm_time.h"

namespace geopm
{
    class LevelZeroDevicePoolImp : public LevelZeroDevicePool
    {
        public:
            LevelZeroDevicePoolImp();
            LevelZeroDevicePoolImp(const LevelZero &levelzero);
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
            double active_time(int domain, unsigned int device_idx,
                               int l0_domain) const override;
            double active_time_timestamp(int domain, unsigned int device_idx,
                                         int l0_domain) const override;
            int32_t power_limit_tdp(int domain, unsigned int domain_idx,
                                    int l0_domain) const override;
            int32_t power_limit_min(int domain, unsigned int domain_idx,
                                    int l0_domain) const override;
            int32_t power_limit_max(int domain, unsigned int domain_idx,
                                    int l0_domain) const override;
            std::pair<uint64_t, uint64_t> energy_pair(int domain, unsigned int domain_idx,
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
            double ras_reset_count_correctable(int domain, unsigned int domain_idx,
                                   int l0_domain) const override;
            double ras_programming_errcount_correctable(int domain, unsigned int domain_idx,
                                            int l0_domain) const override;
            double ras_driver_errcount_correctable(int domain, unsigned int domain_idx,
                                       int l0_domain) const override;
            double ras_compute_errcount_correctable(int domain, unsigned int domain_idx,
                                        int l0_domain) const override;
            double ras_noncompute_errcount_correctable(int domain, unsigned int domain_idx,
                                           int l0_domain) const override;
            double ras_cache_errcount_correctable(int domain, unsigned int domain_idx,
                                      int l0_domain) const override;
            double ras_display_errcount_correctable(int domain, unsigned int domain_idx,
                                        int l0_domain) const override;
            double ras_reset_count_uncorrectable(int domain, unsigned int domain_idx,
                                   int l0_domain) const override;
            double ras_programming_errcount_uncorrectable(int domain, unsigned int domain_idx,
                                            int l0_domain) const override;
            double ras_driver_errcount_uncorrectable(int domain, unsigned int domain_idx,
                                       int l0_domain) const override;
            double ras_compute_errcount_uncorrectable(int domain, unsigned int domain_idx,
                                        int l0_domain) const override;
            double ras_noncompute_errcount_uncorrectable(int domain, unsigned int domain_idx,
                                           int l0_domain) const override;
            double ras_cache_errcount_uncorrectable(int domain, unsigned int domain_idx,
                                      int l0_domain) const override;
            double ras_display_errcount_uncorrectable(int domain, unsigned int domain_idx,
                                        int l0_domain) const override;
        private:
            const LevelZero &m_levelzero;

            void check_idx_range(int domain, unsigned int domain_idx) const;
            void check_domain_exists(int size, const char *func, int line) const;
            std::pair<unsigned int, unsigned int> subdevice_device_conversion(unsigned int idx) const;
            mutable std::map<int, std::vector<uint64_t> > m_active_time_last; // Map from l0_domain to vector over gpu chips
            mutable std::map<int, std::vector<uint64_t> > m_active_time_rollover; // Map from l0_domain to vector over gpu chips
    };
}
#endif
