/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LEVELZERODEVICEPOOL_HPP_INCLUDE
#define LEVELZERODEVICEPOOL_HPP_INCLUDE

#include <vector>
#include <string>
#include <cstdint>

#include "geopm_sched.h"
#include "geopm_topo.h"

namespace geopm
{
    class LevelZeroDevicePool
    {
        public:
            LevelZeroDevicePool() = default;
            virtual ~LevelZeroDevicePool() = default;
            /// @brief Number of GPUs on the platform.
            /// @return Number of LevelZero GPUs.
            /// @param [in] domain The GEOPM domain type being targeted
            virtual int num_gpu(int domain_type) const = 0;
            // FREQUENCY SIGNAL FUNCTIONS
            /// @brief Get the LevelZero device actual frequency in MHz
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU device core clock rate in MHz.
            virtual double frequency_status(int domain, unsigned int domain_idx,
                                            int l0_domain) const = 0;
            /// @brief Get the LevelZero device efficient frequency in MHz
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU device efficient clock rate in MHz.
            virtual double frequency_efficient(int domain, unsigned int domain_idx,
                                               int l0_domain) const = 0;
            /// @brief Get the LevelZero device mininmum frequency in MHz
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU minimum frequency in MHz.
            virtual double frequency_min(int domain, unsigned int domain_idx,
                                         int l0_domain) const = 0;
            /// @brief Get the LevelZero device maximum frequency in MHz
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU maximum frequency in MHz.
            virtual double frequency_max(int domain, unsigned int domain_idx,
                                         int l0_domain) const = 0;
            /// @brief Get the LevelZero device frequency step in MHz
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU frequency step in MHz.
            virtual double frequency_step(int domain, unsigned int domain_idx,
                                          int l0_domain) const = 0;
            /// @brief Get the LevelZero device frequency throttle reasons
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. gpu being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Frequency throttle reasons
            virtual uint32_t frequency_throttle_reasons(int domain, unsigned int domain_idx,
                                                        int l0_domain) const = 0;
            virtual std::pair<double, double> frequency_range(int domain,
                                                              unsigned int domain_idx,
                                                              int l0_domain) const = 0;
            /// @brief Get the LevelZero domain maximum temperature in Celsius
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU domain maximum temperature in Celsius.
            virtual double temperature_max(int domain, unsigned int domain_idx,
                                           int l0_domain) const = 0;
            // UTILIZATION SIGNAL FUNCTIONS
            /// @brief Get the LevelZero device active time and timestamp in microseconds
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU active time and timestamp in microseconds.
            virtual std::pair<uint64_t, uint64_t> active_time_pair(int domain, unsigned int domain_idx,
                                                                   int l0_domain) const = 0;
            /// @brief Get the LevelZero device timestamp for the active time value in microseconds
            /// @brief Get the LevelZero device active time in microseconds
            /// @return GPU active time in microseconds.
            virtual double active_time(int domain, unsigned int domain_idx,
                                       int l0_domain) const = 0;
            /// @brief Get the LevelZero device timestamp for the active time value in microseconds
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU device timestamp for the active time value in microseconds.
            virtual double active_time_timestamp(int domain, unsigned int domain_idx,
                                                 int l0_domain) const = 0;
            // POWER SIGNAL FUNCTIONS
            /// @brief Get the LevelZero device default power limit in milliwatts
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU default power limit in milliwatts
            virtual int32_t power_limit_tdp(int domain, unsigned int domain_idx,
                                            int l0_domain) const = 0;
            /// @brief Get the LevelZero device minimum power limit in milliwatts
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU minimum power limit in milliwatts
            virtual int32_t power_limit_min(int domain, unsigned int domain_idx,
                                            int l0_domain) const = 0;
            /// @brief Get the LevelZero device maximum power limit in milliwatts
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU maximum power limit in milliwatts
            virtual int32_t power_limit_max(int domain, unsigned int domain_idx,
                                            int l0_domain) const = 0;
            // ENERGY SIGNAL FUNCTIONS
            /// @brief Get the LevelZero device energy in microjoules and timestamp in microseconds.
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU energy in microjoules and timestamp in microseconds.
            virtual std::pair<uint64_t, uint64_t> energy_pair(int domain, unsigned int domain_idx,
                                                              int l0_domain) const = 0;
            /// @brief Get the LevelZero device energy in microjoules.
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU energy in microjoules.
            virtual uint64_t energy(int domain, unsigned int domain_idx,
                                    int l0_domain) const = 0;
            /// @brief Get the LevelZero device energy timestamp in microseconds
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU energy timestamp in microseconds
            virtual uint64_t energy_timestamp(int domain, unsigned int domain_idx,
                                              int l0_domain) const = 0;
            /// @brief Get the LevelZero device performance factor
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. gpu or chip being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Chip performance value, 0 - 1.0
            virtual double performance_factor(int domain,
                                              unsigned int domain_idx,
                                              int l0_domain) const = 0;
            // FREQUENCY CONTROL FUNCTIONS
            /// @brief Set min and max frequency for LevelZero device.
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @param [in] range_min Min target frequency in MHz.
            /// @param [in] range_max Max target frequency in MHz.
            virtual void frequency_control(int domain, unsigned int domain_idx,
                                           int l0_domain, double range_min,
                                           double range_max) const = 0;
            /// @brief Set performance factor for LevelZero device.
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. accelerator or chip being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @param [in] setting Performance factor target, 0 - 1.0
            virtual void performance_factor_control(int domain, unsigned int domain_idx,
                                                    int l0_domain,
                                                    double setting) const = 0;
            // RAS SIGNAL FUNCTIONS
            /// @brief Get the LevelZero count of number of correctable accelerator engine
            /// resets attempted by the driver.
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Accelerator Engine Reset count
            virtual double ras_reset_count_correctable(int domain, unsigned int domain_idx,
                                           int l0_domain) const = 0;
            /// @brief Get the LevelZero count of number of correctable hardware exceptions
            /// generated by the way workloads have programmed the hardware
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Programming Error Count
            virtual double ras_programming_errcount_correctable(int domain, unsigned int domain_idx,
                                                    int l0_domain) const = 0;
            /// @brief Get the LevelZero count of number of low level driver
            /// communication correctable errors have occurred.
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Programming Error Count
            virtual double ras_driver_errcount_correctable(int domain, unsigned int domain_idx,
                                               int l0_domain) const = 0;
            /// @brief Get the LevelZero count of number of correctable errors that have
            /// occurred in the compute accelerator hardware
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Compute Error Count
            virtual double ras_compute_errcount_correctable(int domain, unsigned int domain_idx,
                                                int l0_domain) const = 0;
            /// @brief Get the LevelZero count of number of correctable errors that have
            /// occurred in the fixed-function accelerator hardware
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Non Compute Error Count
            virtual double ras_noncompute_errcount_correctable(int domain, unsigned int domain_idx,
                                                   int l0_domain) const = 0;
            /// @brief Get the LevelZero count of number of correctable errors that have
            /// occurred in caches (L1/L3/register file/shared local memory/sampler)
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Cache Error Count
            virtual double ras_cache_errcount_correctable(int domain, unsigned int domain_idx,
                                              int l0_domain) const = 0;
            /// @brief Get the LevelZero count of number of correctable errors that have
            ///        occurred in the display.
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Display Error Count
            virtual double ras_display_errcount_correctable(int domain, unsigned int domain_idx,
                                                int l0_domain) const = 0;
            /// @brief Get the LevelZero count of number of uncorrectable accelerator engine
            /// resets attempted by the driver.
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Accelerator Engine Reset count
            virtual double ras_reset_count_uncorrectable(int domain, unsigned int domain_idx,
                                           int l0_domain) const = 0;
            /// @brief Get the LevelZero count of number of uncorrectable hardware exceptions
            /// generated by the way workloads have programmed the hardware
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Programming Error Count
            virtual double ras_programming_errcount_uncorrectable(int domain, unsigned int domain_idx,
                                                    int l0_domain) const = 0;
            /// @brief Get the LevelZero count of number of low level driver
            /// communication uncorrectable errors have occurred.
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Programming Error Count
            virtual double ras_driver_errcount_uncorrectable(int domain, unsigned int domain_idx,
                                               int l0_domain) const = 0;
            /// @brief Get the LevelZero count of number of uncorrectable errors that have
            /// occurred in the compute accelerator hardware
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Compute Error Count
            virtual double ras_compute_errcount_uncorrectable(int domain, unsigned int domain_idx,
                                                int l0_domain) const = 0;
            /// @brief Get the LevelZero count of number of uncorrectable errors that have
            /// occurred in the fixed-function accelerator hardware
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Non Compute Error Count
            virtual double ras_noncompute_errcount_uncorrectable(int domain, unsigned int domain_idx,
                                                   int l0_domain) const = 0;
            /// @brief Get the LevelZero count of number of uncorrectable errors that have
            /// occurred in caches (L1/L3/register file/shared local memory/sampler)
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Cache Error Count
            virtual double ras_cache_errcount_uncorrectable(int domain, unsigned int domain_idx,
                                              int l0_domain) const = 0;
            /// @brief Get the LevelZero count of number of uncorrectable errors that have
            ///        occurred in the display.
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Display Error Count
            virtual double ras_display_errcount_uncorrectable(int domain, unsigned int domain_idx,
                                                int l0_domain) const = 0;
        private:
    };

    const LevelZeroDevicePool &levelzero_device_pool();
}
#endif
