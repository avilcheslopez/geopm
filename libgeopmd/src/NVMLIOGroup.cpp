/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "NVMLIOGroup.hpp"

#include <cmath>

#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>
#include <sched.h>
#include <errno.h>
#include <unistd.h>

#include "geopm/IOGroup.hpp"
#include "geopm/PlatformTopo.hpp"
#include "NVMLDevicePool.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"
#include "geopm/Helper.hpp"
#include "geopm/SaveControl.hpp"

namespace geopm
{
    const std::string NVMLIOGroup::M_PLUGIN_NAME = "NVML";
    const std::string NVMLIOGroup::M_NAME_PREFIX = M_PLUGIN_NAME + "::";

    NVMLIOGroup::NVMLIOGroup()
        : NVMLIOGroup(platform_topo(),
                      nvml_device_pool(platform_topo().num_domain(GEOPM_DOMAIN_CPU)),
                      nullptr)
    {
    }

    // Set up mapping between signal and control names and corresponding indices
    NVMLIOGroup::NVMLIOGroup(const PlatformTopo &platform_topo,
                             const NVMLDevicePool &device_pool,
                             std::shared_ptr<SaveControl> save_control_test)
        : m_platform_topo(platform_topo)
        , m_nvml_device_pool(device_pool)
        , m_is_batch_read(false)
        , m_frequency_max_control_request(m_platform_topo.num_domain(GEOPM_DOMAIN_GPU), NAN)
        , m_frequency_min_control_request(m_platform_topo.num_domain(GEOPM_DOMAIN_GPU), NAN)
        , m_initial_power_limit(m_platform_topo.num_domain(GEOPM_DOMAIN_GPU), 0)
        , m_signal_available({{M_NAME_PREFIX + "GPU_CORE_FREQUENCY_STATUS", {
                                  "Streaming Multiprocessor frequency",
                                  {},
                                  GEOPM_DOMAIN_GPU,
                                  M_UNITS_HERTZ,
                                  Agg::average,
                                  IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                  string_format_double
                                  }},
                              {M_NAME_PREFIX + "GPU_UTILIZATION", {
                                  "Fraction of time the GPU operated on a kernel in the last set of driver samples",
                                  {},
                                  GEOPM_DOMAIN_GPU,
                                  M_UNITS_NONE,
                                  Agg::average,
                                  IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                  string_format_double
                                  }},
                              {M_NAME_PREFIX + "GPU_POWER", {
                                  "GPU power usage",
                                  {},
                                  GEOPM_DOMAIN_GPU,
                                  M_UNITS_WATTS,
                                  Agg::sum,
                                  IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                  string_format_double
                                  }},
                              {M_NAME_PREFIX + "GPU_POWER_LIMIT_CONTROL", {
                                  "GPU power limit",
                                  {},
                                  GEOPM_DOMAIN_GPU,
                                  M_UNITS_WATTS,
                                  Agg::sum,
                                  IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                  string_format_double
                                  }},
                              {M_NAME_PREFIX + "GPU_UNCORE_FREQUENCY_STATUS", {
                                  "GPU memory frequency",
                                  {},
                                  GEOPM_DOMAIN_GPU,
                                  M_UNITS_HERTZ,
                                  Agg::average,
                                  IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                  string_format_double
                                  }},
                              {M_NAME_PREFIX + "GPU_CORE_THROTTLE_REASONS", {
                                  "GPU clock throttling reasons",
                                  {},
                                  GEOPM_DOMAIN_GPU,
                                  M_UNITS_NONE,
                                  Agg::integer_bitwise_or,
                                  IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                  string_format_double
                                  }},
                              {M_NAME_PREFIX + "GPU_TEMPERATURE", {
                                  "GPU temperature",
                                  {},
                                  GEOPM_DOMAIN_GPU,
                                  M_UNITS_CELSIUS,
                                  Agg::average,
                                  IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                  string_format_double
                                  }},
                              {M_NAME_PREFIX + "GPU_ENERGY_CONSUMPTION_TOTAL", {
                                  "GPU energy consumption since the driver was loaded",
                                  {},
                                  GEOPM_DOMAIN_GPU,
                                  M_UNITS_JOULES,
                                  Agg::sum,
                                  IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
                                  string_format_double
                                  }},
                              {M_NAME_PREFIX + "GPU_PERFORMANCE_STATE", {
                                  "GPU performance state, defined by the NVML API as a value from 0 to 15"
                                  "\n  with 0 being maximum performance, 15 being minimum performance, and 32 being unknown",
                                  {},
                                  GEOPM_DOMAIN_GPU,
                                  M_UNITS_NONE,
                                  Agg::expect_same,
                                  IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                  string_format_double
                                  }},
                              {M_NAME_PREFIX + "GPU_PCIE_RX_THROUGHPUT", {
                                  "GPU PCIE receive throughput in bytes per second over a 20 millisecond period",
                                  {},
                                  GEOPM_DOMAIN_GPU,
                                  M_UNITS_HERTZ,
                                  Agg::sum,
                                  IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                  string_format_double
                                  }},
                              {M_NAME_PREFIX + "GPU_PCIE_TX_THROUGHPUT", {
                                  "GPU PCIE transmit throughput in bytes per second over a 20 millisecond period",
                                  {},
                                  GEOPM_DOMAIN_GPU,
                                  M_UNITS_HERTZ,
                                  Agg::sum,
                                  IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                  string_format_double
                                  }},
                              {M_NAME_PREFIX + "GPU_CPU_ACTIVE_AFFINITIZATION", {
                                  "Returns the associated GPU for a given CPU as determined by running processes."
                                  "\n  If no GPUs map to the CPU then -1 is returned"
                                  "\n  If multiple GPUs map to the CPU NAN is returned",
                                  {},
                                  GEOPM_DOMAIN_CPU,
                                  M_UNITS_NONE,
                                  Agg::expect_same,
                                  IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                  string_format_double
                                  }},
                              {M_NAME_PREFIX + "GPU_UNCORE_UTILIZATION", {
                                  "Fraction of time the GPU memory was accessed in the last set of driver samples",
                                  {},
                                  GEOPM_DOMAIN_GPU,
                                  M_UNITS_NONE,
                                  Agg::average,
                                  IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                  string_format_double
                                  }},
                              {M_NAME_PREFIX + "GPU_CORE_FREQUENCY_MAX_AVAIL", {
                                  "Streaming Multiprocessor Maximum frequency",
                                  {},
                                  GEOPM_DOMAIN_GPU,
                                  M_UNITS_HERTZ,
                                  Agg::expect_same,
                                  IOGroup::M_SIGNAL_BEHAVIOR_CONSTANT,
                                  string_format_double
                                  }},
                              {M_NAME_PREFIX + "GPU_CORE_FREQUENCY_MIN_AVAIL", {
                                  "Streaming Multiprocessor Minimum frequency",
                                  {},
                                  GEOPM_DOMAIN_GPU,
                                  M_UNITS_HERTZ,
                                  Agg::expect_same,
                                  IOGroup::M_SIGNAL_BEHAVIOR_CONSTANT,
                                  string_format_double
                                  }},
                              {M_NAME_PREFIX + "GPU_CORE_FREQUENCY_STEP", {
                                  "The average Streaming Multiprocessor frequency step size\n"
                                  "                  The average step size is provided if not constant.",
                                  {},
                                  GEOPM_DOMAIN_GPU,
                                  M_UNITS_HERTZ,
                                  Agg::expect_same,
                                  IOGroup::M_SIGNAL_BEHAVIOR_CONSTANT,
                                  string_format_double
                                  }},
                              {M_NAME_PREFIX + "GPU_CORE_FREQUENCY_MAX_CONTROL", {
                                  "Latest frequency maximum control request",
                                  {},
                                  GEOPM_DOMAIN_GPU,
                                  M_UNITS_HERTZ,
                                  Agg::expect_same,
                                  IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                  string_format_double
                                  }},
                              {M_NAME_PREFIX + "GPU_CORE_FREQUENCY_MIN_CONTROL", {
                                  "Latest frequency minimum control request",
                                  {},
                                  GEOPM_DOMAIN_GPU,
                                  M_UNITS_HERTZ,
                                  Agg::expect_same,
                                  IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                  string_format_double
                                  }},
                              {M_NAME_PREFIX + "GPU_CORE_FREQUENCY_RESET_CONTROL", {
                                  "Resets Streaming Multiprocessor frequency min and max limits to default values.",
                                  {},
                                  GEOPM_DOMAIN_GPU,
                                  M_UNITS_NONE,
                                  Agg::expect_same,
                                  IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                  string_format_double
                                  }}
                             })
        , m_control_available({{M_NAME_PREFIX + "GPU_CORE_FREQUENCY_MAX_CONTROL", {
                                    "Sets Streaming Multiprocessor frequency max",
                                    {},
                                    GEOPM_DOMAIN_GPU,
                                    M_UNITS_HERTZ,
                                    Agg::expect_same,
                                    string_format_double
                                    }},
                               {M_NAME_PREFIX + "GPU_CORE_FREQUENCY_MIN_CONTROL", {
                                    "Sets Streaming Multiprocessor frequency min",
                                    {},
                                    GEOPM_DOMAIN_GPU,
                                    M_UNITS_HERTZ,
                                    Agg::expect_same,
                                    string_format_double
                                    }},
                               {M_NAME_PREFIX + "GPU_CORE_FREQUENCY_RESET_CONTROL", {
                                    "Resets Streaming Multiprocessor frequency min and max limits to default values\n"
                                    "                  Parameter provided is unused.",
                                    {},
                                    GEOPM_DOMAIN_GPU,
                                    M_UNITS_NONE,
                                    Agg::average,
                                    string_format_double
                                    }},
                               {M_NAME_PREFIX + "GPU_POWER_LIMIT_CONTROL", {
                                    "Sets GPU power limit",
                                    {},
                                    GEOPM_DOMAIN_GPU,
                                    M_UNITS_WATTS,
                                    Agg::sum,
                                    string_format_double
                                    }}
                              })
        , m_mock_save_ctl(std::move(save_control_test))
    {
        // populate signals for each domain
        for (auto &sv : m_signal_available) {
            std::vector<std::shared_ptr<signal_s> > result;
            for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(signal_domain_type(sv.first)); ++domain_idx) {
                std::shared_ptr<signal_s> sgnl = std::make_shared<signal_s>(signal_s{0, false});
                result.push_back(std::move(sgnl));
            }
            sv.second.signals = std::move(result);
        }

        // Setup FREQUENCY MINIMUM_AVAIL and MAXIMUM_AVAIL signals
        for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(GEOPM_DOMAIN_GPU); ++domain_idx) {
            std::vector<unsigned int> supported_frequency = m_nvml_device_pool.frequency_supported_sm(domain_idx);
            if (supported_frequency.size() == 0) {
                // TODO: Long term this should simply hide the FREQUENCY_MIN and FREQUENCY_MAX signals,
                //       not prevent loading of the IO Group

                throw Exception("NVMLIOGroup::" + std::string(__func__) +
                                ": No supported frequencies found for GPU " + std::to_string(domain_idx),
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }

            // sort guarantees the ordering for min & max calls
            std::sort(supported_frequency.begin(), supported_frequency.end());
            m_supported_freq.push_back(supported_frequency);

            // Since we cannot read the min & max control setting we're defaulting
            // to the min & max supported frequency for the associated signals
            m_frequency_max_control_request.at(domain_idx) = m_supported_freq.at(domain_idx).back() * 1e6;
            m_frequency_min_control_request.at(domain_idx) = m_supported_freq.at(domain_idx).front() * 1e6;

            // Calculated step size if there are 2 or more supported frequencies
            if (supported_frequency.size() >= 2) {
                double step = (double) (supported_frequency.back() - supported_frequency.front())
                              / (supported_frequency.size() - 1);

                m_frequency_step.push_back(step);
            }
            else {
                m_frequency_step.push_back(NAN);
            }

        }

        std::vector <std::string> unsupported_signal_names;
        for (auto &sv : m_signal_available) {
            for (unsigned int domain_idx = 0; domain_idx < sv.second.signals.size(); ++domain_idx) {
                try {
                    read_signal(sv.first, sv.second.domain, domain_idx);
                }
                catch (const geopm::Exception &ex) {
                    if (ex.err_value() != GEOPM_ERROR_RUNTIME &&
                        ex.err_value() != GEOPM_ERROR_INVALID) {
                        throw;
                    }
                    unsupported_signal_names.push_back(sv.first);
                    break;
                }
            }
        }

        for(const auto &name : unsupported_signal_names) {
            m_signal_available.erase(name);
        }

        register_signal_alias("GPU_POWER", M_NAME_PREFIX + "GPU_POWER");
        register_signal_alias("GPU_CORE_FREQUENCY_STATUS", M_NAME_PREFIX + "GPU_CORE_FREQUENCY_STATUS");
        register_signal_alias("GPU_CORE_FREQUENCY_MIN_AVAIL", M_NAME_PREFIX + "GPU_CORE_FREQUENCY_MIN_AVAIL");
        register_signal_alias("GPU_CORE_FREQUENCY_MAX_AVAIL", M_NAME_PREFIX + "GPU_CORE_FREQUENCY_MAX_AVAIL");
        register_signal_alias("GPU_CORE_FREQUENCY_STEP", M_NAME_PREFIX + "GPU_CORE_FREQUENCY_STEP");
        register_signal_alias("GPU_ENERGY", M_NAME_PREFIX + "GPU_ENERGY_CONSUMPTION_TOTAL");
        register_signal_alias("GPU_TEMPERATURE", M_NAME_PREFIX + "GPU_TEMPERATURE");
        register_signal_alias("GPU_UTILIZATION", M_NAME_PREFIX + "GPU_UTILIZATION");

        // populate controls for each domain
        for (auto &sv : m_control_available) {
            std::vector<std::shared_ptr<control_s> > result;
            for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(control_domain_type(sv.first)); ++domain_idx) {
                std::shared_ptr<control_s> ctrl = std::make_shared<control_s>(control_s{0, false});
                result.push_back(std::move(ctrl));
            }
            sv.second.controls = std::move(result);
        }

        // Only a user with elevated privileges will be able to control
        // frequency and power.  Prune the controls if we don't have access.
        if (!m_nvml_device_pool.is_privileged_access()) {
            m_control_available.erase(M_NAME_PREFIX + "GPU_CORE_FREQUENCY_MAX_CONTROL");
            m_control_available.erase(M_NAME_PREFIX + "GPU_CORE_FREQUENCY_MIN_CONTROL");
            m_control_available.erase(M_NAME_PREFIX + "GPU_CORE_FREQUENCY_RESET_CONTROL");
            m_control_available.erase(M_NAME_PREFIX + "GPU_POWER_LIMIT_CONTROL");
        }

        register_control_alias("GPU_POWER_LIMIT_CONTROL", M_NAME_PREFIX + "GPU_POWER_LIMIT_CONTROL");
        register_signal_alias("GPU_POWER_LIMIT_CONTROL", M_NAME_PREFIX + "GPU_POWER_LIMIT_CONTROL");
        register_control_alias("GPU_CORE_FREQUENCY_MAX_CONTROL", M_NAME_PREFIX + "GPU_CORE_FREQUENCY_MAX_CONTROL");
        register_signal_alias("GPU_CORE_FREQUENCY_MAX_CONTROL", M_NAME_PREFIX + "GPU_CORE_FREQUENCY_MAX_CONTROL");
        register_control_alias("GPU_CORE_FREQUENCY_MIN_CONTROL", M_NAME_PREFIX + "GPU_CORE_FREQUENCY_MIN_CONTROL");
        register_signal_alias("GPU_CORE_FREQUENCY_MIN_CONTROL", M_NAME_PREFIX + "GPU_CORE_FREQUENCY_MIN_CONTROL");
    }

    // Extract the set of all signal names from the index map
    std::set<std::string> NVMLIOGroup::signal_names(void) const
    {
        std::set<std::string> result;
        for (const auto &sv : m_signal_available) {
            result.insert(sv.first);
        }
        return result;
    }

    // Extract the set of all control names from the index map
    std::set<std::string> NVMLIOGroup::control_names(void) const
    {
        std::set<std::string> result;
        for (const auto &sv : m_control_available) {
            result.insert(sv.first);
        }
        return result;
    }

    // Check signal name using index map
    bool NVMLIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return m_signal_available.find(signal_name) != m_signal_available.end();
    }

    // Check control name using index map
    bool NVMLIOGroup::is_valid_control(const std::string &control_name) const
    {
        return m_control_available.find(control_name) != m_control_available.end();
    }

    // Return domain for all valid signals
    int NVMLIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        auto it = m_signal_available.find(signal_name);
        if (it != m_signal_available.end()) {
            result = it->second.domain;
        }
        return result;
    }

    // Return domain for all valid controls
    int NVMLIOGroup::control_domain_type(const std::string &control_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        auto it = m_control_available.find(control_name);
        if (it != m_control_available.end()) {
            result = it->second.domain;
        }
        return result;
    }

    // Mark the given signal to be read by read_batch()
    int NVMLIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": signal_name " + signal_name +
                            " not valid for NVMLIOGroup.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != signal_domain_type(signal_name)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": " + signal_name + ": domain_type must be " +
                            std::to_string(signal_domain_type(signal_name)),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(signal_domain_type(signal_name))) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": domain_idx out of range.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (m_is_batch_read) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": cannot push signal after call to read_batch().",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        int result = -1;
        bool is_found = false;
        std::shared_ptr<signal_s> signal = m_signal_available.at(signal_name).signals.at(domain_idx);

        // Check if signal was already pushed
        for (size_t ii = 0; !is_found && ii < m_signal_pushed.size(); ++ii) {
            // same location means this signal or its alias was already pushed
            if (m_signal_pushed[ii].get() == signal.get()) {
                result = ii;
                is_found = true;
            }
        }
        if (!is_found) {
            // If not pushed, add to pushed signals and configure for batch reads
            result = m_signal_pushed.size();
            signal->m_do_read = true;
            m_signal_pushed.push_back(std::move(signal));
        }

        return result;
    }

    // Mark the given control to be written by write_batch()
    int NVMLIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
    {
        if (!is_valid_control(control_name)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": control_name " + control_name +
                            " not valid for NVMLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != control_domain_type(control_name)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": " + control_name + ": domain_type must be " +
                            std::to_string(control_domain_type(control_name)),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(domain_type)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": domain_idx out of range.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        int result = -1;
        bool is_found = false;
        std::shared_ptr<control_s> control = m_control_available.at(control_name).controls.at(domain_idx);

        // Check if control was already pushed
        for (size_t ii = 0; !is_found && ii < m_control_pushed.size(); ++ii) {
            // same location means this control or its alias was already pushed
            if (m_control_pushed[ii] == control) {
                result = ii;
                is_found = true;
            }
        }
        if (!is_found) {
            // If not pushed, add to pushed control
            result = m_control_pushed.size();
            m_control_pushed.push_back(std::move(control));
        }

        return result;
    }

    // The active process list NVML call can be costly, 0.5-2ms per call was seen in early testing on average,
    // with a worst case of 8ms per call.  Because of this we cache the processes in a PID <-> GPU map
    // before using them elsewhere
    std::map<pid_t, double> NVMLIOGroup::gpu_process_map(void) const
    {
        std::map<pid_t,double> gpu_pid_map;

        for (int gpu_idx = 0; gpu_idx < m_platform_topo.num_domain(GEOPM_DOMAIN_GPU); ++gpu_idx) {
            std::vector<int> active_process_list = m_nvml_device_pool.active_process_list(gpu_idx);
            for (auto proc_itr : active_process_list) {
                // If a process is associated with multiple GPUs we have no good means of
                // signaling the user beyond providing an error value (NAN).
                if (!gpu_pid_map.count((pid_t)proc_itr)) {
                    gpu_pid_map[(pid_t)proc_itr] = gpu_idx;
                }
                else {
                    gpu_pid_map[(pid_t)proc_itr] = NAN;
                }
            }
        }
        return gpu_pid_map;
    }

    // Parse PID to CPU affinitzation and use process list --> GPU map to get CPU --> GPU
    double NVMLIOGroup::cpu_gpu_affinity(int cpu_idx, std::map<pid_t, double> process_map) const
    {
        double result = -1;
        size_t num_cpu = m_platform_topo.num_domain(GEOPM_DOMAIN_CPU);
        size_t alloc_size = CPU_ALLOC_SIZE(num_cpu);
	auto proc_cpuset = make_cpu_set(num_cpu, {});
        if (proc_cpuset == NULL) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) +
                            ": failed to allocate process CPU mask",
                            ENOMEM, __FILE__, __LINE__);
        }
        for (auto &proc : process_map) {
            int err = sched_getaffinity(proc.first, alloc_size, proc_cpuset.get());
            if (err == EINVAL || err == EFAULT) {
                throw Exception("NVMLIOGroup::" + std::string(__func__) +
                                ": failed to get affinity mask for process: " +
                                std::to_string(proc.first), err, __FILE__, __LINE__);
            }
            if (!err && CPU_ISSET(cpu_idx, proc_cpuset.get())) {
                result = proc.second;
                // Return first match, w/o break will return last match
                break;
            }
        }
        return result;
    }

    // Parse and update saved values for signals
    void NVMLIOGroup::read_batch(void)
    {
        m_is_batch_read = true;
        for (auto &sv : m_signal_available) {
            if (sv.first == M_NAME_PREFIX + "GPU_CPU_ACTIVE_AFFINITIZATION") {
                std::map<pid_t, double> process_map;
                bool is_map_cached = false;

                for (unsigned int domain_idx = 0; domain_idx < sv.second.signals.size(); ++domain_idx) {
                    if (sv.second.signals.at(domain_idx)->m_do_read) {
                        if (is_map_cached == false) {
                            process_map = gpu_process_map();
                            is_map_cached = true;
                        }
                        sv.second.signals.at(domain_idx)->m_value = cpu_gpu_affinity(domain_idx, process_map);
                    }
                }
            }
            else {
                for (unsigned int domain_idx = 0; domain_idx < sv.second.signals.size(); ++domain_idx) {
                    if (sv.second.signals.at(domain_idx)->m_do_read) {
                        sv.second.signals.at(domain_idx)->m_value = read_signal(sv.first, sv.second.domain, domain_idx);
                    }
                }
            }
        }
    }

    // Write all controls that have been pushed and adjusted
    void NVMLIOGroup::write_batch(void)
    {

        // If there are any two controls that have an ordering requirement, the retry logic in this
        // loop enables them to be written in the proper order.
        // This situation arises due to requirements of minimum settings being less than maximum settings.
        bool do_throw = false;
        bool do_retry = false;
        do {
            // Loop provides one retry attempt.  Setting do_retry to false after the first
            // attempt will end loop after second attempt.
            if (do_throw) {
                do_retry = false;
            }
            for (auto &sv : m_control_available) {
                for (unsigned int domain_idx = 0; domain_idx < sv.second.controls.size(); ++domain_idx) {
                    if (sv.second.controls.at(domain_idx)->m_is_adjusted) {
                        try{
                            write_control(sv.first, sv.second.domain, domain_idx, sv.second.controls.at(domain_idx)->m_setting);
                            sv.second.controls.at(domain_idx)->m_is_adjusted = false;
                        }
                        catch (...) {
                            if (do_throw) {
                                throw;
                            }
                            do_retry = true;
                        }
                    }
                }
            }
            do_throw = true;
        } while (do_retry);
    }

    // Return the latest value read by read_batch()
    double NVMLIOGroup::sample(int batch_idx)
    {
        // Do conversion of signal values stored in read batch
        if (batch_idx < 0 || batch_idx >= (int)m_signal_pushed.size()) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": batch_idx " +std::to_string(batch_idx)+ " out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (!m_is_batch_read) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": signal has not been read.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        return m_signal_pushed[batch_idx]->m_value;
    }

    // Save a setting to be written by a future write_batch()
    void NVMLIOGroup::adjust(int batch_idx, double setting)
    {
        if (batch_idx < 0 || (unsigned)batch_idx >= m_control_pushed.size()) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + "(): batch_idx " +std::to_string(batch_idx)+ " out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        m_control_pushed.at(batch_idx)->m_setting = setting;
        m_control_pushed.at(batch_idx)->m_is_adjusted = true;
    }

    // Read the value of a signal immediately, bypassing read_batch().  Should not modify m_signal_value
    double NVMLIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": " + signal_name +
                            " not valid for NVMLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != signal_domain_type(signal_name)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": " + signal_name + ": domain_type must be " +
                            std::to_string(signal_domain_type(signal_name)),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(signal_domain_type(signal_name))) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": domain_idx out of range.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        double result = NAN;
        if (signal_name == M_NAME_PREFIX + "GPU_CORE_FREQUENCY_STATUS" || signal_name == "GPU_CORE_FREQUENCY_STATUS") {
            result = (double) m_nvml_device_pool.frequency_status_sm(domain_idx) * 1e6;
        }
        else if (signal_name == M_NAME_PREFIX + "GPU_CORE_FREQUENCY_MIN_AVAIL" || signal_name == "GPU_CORE_FREQUENCY_MIN_AVAIL") {
            if (m_supported_freq.at(domain_idx).size() != 0) {
                result = 1e6 * m_supported_freq.at(domain_idx).front();
            }
        }
        else if (signal_name == M_NAME_PREFIX + "GPU_CORE_FREQUENCY_MAX_AVAIL" || signal_name == "GPU_CORE_FREQUENCY_MAX_AVAIL") {
            if (m_supported_freq.at(domain_idx).size() != 0) {
                result = 1e6 * m_supported_freq.at(domain_idx).back();
            }
        }
        else if (signal_name == M_NAME_PREFIX + "GPU_CORE_FREQUENCY_STEP" || signal_name == "GPU_CORE_FREQUENCY_STEP") {
            //  If supported freqs doesn't provide at least two frequencies we won't have a step size
            if (m_supported_freq.at(domain_idx).size() >= 2) {
                result = 1e6 * m_frequency_step.at(domain_idx);
            }
        }
        else if (signal_name == M_NAME_PREFIX + "GPU_UTILIZATION" || signal_name == "GPU_UTILIZATION") {
            result = (double) m_nvml_device_pool.utilization(domain_idx) / 100;
        }
        else if (signal_name == M_NAME_PREFIX + "GPU_CORE_THROTTLE_REASONS") {
            result = (double) m_nvml_device_pool.throttle_reasons(domain_idx);
        }
        else if (signal_name == M_NAME_PREFIX + "GPU_POWER" || signal_name == "GPU_POWER") {
            result = (double) m_nvml_device_pool.power(domain_idx) * 1e-3;
        }
        else if (signal_name == M_NAME_PREFIX + "GPU_POWER_LIMIT_CONTROL" ||
                 signal_name == "GPU_POWER_LIMIT_CONTROL") {
            result = (double) m_nvml_device_pool.power_limit(domain_idx) * 1e-3;
        }
        else if (signal_name == M_NAME_PREFIX + "GPU_UNCORE_FREQUENCY_STATUS") {
            result = (double) m_nvml_device_pool.frequency_status_mem(domain_idx) * 1e6;
        }
        else if (signal_name == M_NAME_PREFIX + "GPU_TEMPERATURE" || signal_name == "GPU_TEMPERATURE") {
            result = (double) m_nvml_device_pool.temperature(domain_idx);
        }
        else if (signal_name == M_NAME_PREFIX + "GPU_ENERGY_CONSUMPTION_TOTAL" || signal_name == "GPU_ENERGY") {
            result = (double) m_nvml_device_pool.energy(domain_idx) * 1e-3;
        }
        else if (signal_name == M_NAME_PREFIX + "GPU_PERFORMANCE_STATE") {
            result = (double) m_nvml_device_pool.performance_state(domain_idx);
        }
        else if (signal_name == M_NAME_PREFIX + "GPU_PCIE_RX_THROUGHPUT") {
            result = (double) m_nvml_device_pool.throughput_rx_pcie(domain_idx) * 1024;
        }
        else if (signal_name == M_NAME_PREFIX + "GPU_PCIE_TX_THROUGHPUT") {
            result = (double) m_nvml_device_pool.throughput_tx_pcie(domain_idx) * 1024;
        }
        else if (signal_name == M_NAME_PREFIX + "GPU_UNCORE_UTILIZATION") {
            result = (double) m_nvml_device_pool.utilization_mem(domain_idx) * 1e-2;
        }
        else if (signal_name == M_NAME_PREFIX + "GPU_CPU_ACTIVE_AFFINITIZATION") {
            std::map<pid_t, double> process_map = gpu_process_map();
            result = cpu_gpu_affinity(domain_idx, std::move(process_map));
        }
        else if (signal_name == M_NAME_PREFIX + "GPU_CORE_FREQUENCY_MAX_CONTROL" || signal_name == "GPU_CORE_FREQUENCY_MAX_CONTROL") {
            result = m_frequency_max_control_request.at(domain_idx);
        }
        else if (signal_name == M_NAME_PREFIX + "GPU_CORE_FREQUENCY_MIN_CONTROL" || signal_name == "GPU_CORE_FREQUENCY_MIN_CONTROL") {
            result = m_frequency_min_control_request.at(domain_idx);
        }
        else if (signal_name == M_NAME_PREFIX + "GPU_CORE_FREQUENCY_RESET_CONTROL") {
            ; // No-op.  Nothing to return.
        }
        else {
    #ifdef GEOPM_DEBUG
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": Handling not defined for " +
                            signal_name, GEOPM_ERROR_LOGIC, __FILE__, __LINE__);

    #endif
        }
        return result;
    }

    // Write to the control immediately, bypassing write_batch()
    void NVMLIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
    {
        if (!is_valid_control(control_name)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": " + control_name +
                            " not valid for NVMLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != control_domain_type(control_name)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": " + control_name + ": domain_type must be " +
                            std::to_string(control_domain_type(control_name)),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(control_domain_type(control_name))) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": domain_idx out of range.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (control_name == M_NAME_PREFIX + "GPU_CORE_FREQUENCY_MAX_CONTROL" || control_name == "GPU_CORE_FREQUENCY_MAX_CONTROL") {
            double min_request;
            if(!std::isnan(m_frequency_min_control_request.at(domain_idx))) {
                min_request = m_frequency_min_control_request.at(domain_idx);
            }
            else {
                min_request = m_supported_freq.at(domain_idx).front() * 1e6;
            }
            m_nvml_device_pool.frequency_control_sm(domain_idx, min_request * 1e-6, setting * 1e-6);
            m_frequency_max_control_request.at(domain_idx) = setting;
        }
        else if (control_name == M_NAME_PREFIX + "GPU_CORE_FREQUENCY_MIN_CONTROL" || control_name == "GPU_CORE_FREQUENCY_MIN_CONTROL") {
            double max_request;
            if(!std::isnan(m_frequency_max_control_request.at(domain_idx))) {
                max_request = m_frequency_max_control_request.at(domain_idx);
            }
            else {
                max_request = m_supported_freq.at(domain_idx).back() * 1e6;
            }
            m_nvml_device_pool.frequency_control_sm(domain_idx, setting * 1e-6, max_request * 1e-6);
            m_frequency_min_control_request.at(domain_idx) = setting;
        }
        else if (control_name == M_NAME_PREFIX + "GPU_CORE_FREQUENCY_RESET_CONTROL") {
            m_nvml_device_pool.frequency_reset_control(domain_idx);
        }
        else if (control_name == M_NAME_PREFIX + "GPU_POWER_LIMIT_CONTROL" || control_name == "GPU_POWER_LIMIT_CONTROL") {
            m_nvml_device_pool.power_control(domain_idx, setting * 1e3);
        }
        else {
    #ifdef GEOPM_DEBUG
                throw Exception("NVMLIOGroup::" + std::string(__func__) + "Handling not defined for "
                                + control_name, GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
    #endif
        }
    }

    // Implemented to allow an IOGroup to save platform settings before starting
    // to adjust them
    void NVMLIOGroup::save_control(void)
    {
        // Read NVML Power Limit
        for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(GEOPM_DOMAIN_GPU); ++domain_idx) {
            m_initial_power_limit.at(domain_idx) = m_nvml_device_pool.power_limit(domain_idx);
        }
    }

    // Implemented to allow an IOGroup to restore previously saved
    // platform settings
    void NVMLIOGroup::restore_control(void)
    {
        // The following calls into the device pool require root privileges
        for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(GEOPM_DOMAIN_GPU); ++domain_idx) {
            try {
                // Write original NVML Power Limit
                m_nvml_device_pool.power_control(domain_idx, m_initial_power_limit.at(domain_idx));
                // Reset NVML Frequency Limit
                m_nvml_device_pool.frequency_reset_control(domain_idx);
            }
            catch (const geopm::Exception &ex) {
#ifdef GEOPM_DEBUG
                std::cerr << "Warning: <geopm> NVMLIOGroup: Failed to "
                             "restore frequency control & power settings for "
                             "GPU domain " << std::to_string(domain_idx)
                             << ".  Exception: " << ex.what()
                             << std::endl;
#endif
            }
        }
    }

    // Hint to Agent about how to aggregate signals from this IOGroup
    std::function<double(const std::vector<double> &)> NVMLIOGroup::agg_function(const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": " + signal_name +
                            "not valid for NVMLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second.agg_function;
    }

    // Specifies how to print signals from this IOGroup
    std::function<std::string(double)> NVMLIOGroup::format_function(const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": " + signal_name +
                            "not valid for NVMLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second.format_function;
    }

    // A user-friendly description of each signal
    std::string NVMLIOGroup::signal_description(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": signal_name " + signal_name +
                            " not valid for NVMLIOGroup.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto it = m_signal_available.find(signal_name);
        std::ostringstream oss;
        oss << "    description: " << it->second.m_description << "\n"
            << "    units: " << IOGroup::units_to_string(it->second.units) << "\n"
            << "    aggregation: " << Agg::function_to_name(it->second.agg_function) << "\n"
            << "    domain: " << platform_topo().domain_type_to_name(it->second.domain) << "\n"
            << "    iogroup: NVMLIOGroup";

        return oss.str();
    }

    // A user-friendly description of each control
    std::string NVMLIOGroup::control_description(const std::string &control_name) const
    {
        if (!is_valid_control(control_name)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": " + control_name +
                            "not valid for NVMLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto it = m_control_available.find(control_name);
        std::ostringstream oss;
        oss << "    description: " << it->second.m_description << "\n"
            << "    units: " << IOGroup::units_to_string(it->second.units) << "\n"
            << "    aggregation: " << Agg::function_to_name(it->second.agg_function) << "\n"
            << "    domain: " << platform_topo().domain_type_to_name(it->second.domain) << "\n"
            << "    iogroup: NVMLIOGroup";

        return oss.str();
    }

    int NVMLIOGroup::signal_behavior(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": signal_name " + signal_name +
                            " not valid for NVMLIOGroup.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_signal_available.at(signal_name).behavior;
    }

    void NVMLIOGroup::save_control(const std::string &save_path)
    {
        std::vector<SaveControl::m_setting_s> settings;
        int num_domains = m_platform_topo.num_domain(GEOPM_DOMAIN_GPU);
        for (int domain_idx = 0; domain_idx < num_domains; ++domain_idx) {
            settings.push_back({M_NAME_PREFIX + "GPU_CORE_FREQUENCY_RESET_CONTROL",
                                GEOPM_DOMAIN_GPU,
                                domain_idx,
                                0});
            double curr_value = read_signal(M_NAME_PREFIX + "GPU_POWER_LIMIT_CONTROL",
                                            GEOPM_DOMAIN_GPU, domain_idx);
            settings.push_back({M_NAME_PREFIX + "GPU_POWER_LIMIT_CONTROL",
                                GEOPM_DOMAIN_GPU,
                                domain_idx,
                                curr_value});
        }

        std::shared_ptr<SaveControl> save_ctl = m_mock_save_ctl;
        if (save_ctl == nullptr) {
            save_ctl = SaveControl::make_unique(settings);
        }
        save_ctl->write_json(save_path);
    }

    void NVMLIOGroup::restore_control(const std::string &save_path)
    {
        std::shared_ptr<SaveControl> save_ctl = m_mock_save_ctl;
        if (save_ctl == nullptr) {
            save_ctl = SaveControl::make_unique(geopm::read_file(save_path));
        }
        save_ctl->restore(*this);
    }

    std::string NVMLIOGroup::name(void) const
    {
        return plugin_name();
    }

    // Name used for registration with the IOGroup factory
    std::string NVMLIOGroup::plugin_name(void)
    {
        return M_PLUGIN_NAME;
    }

    // Function used by the factory to create objects of this type
    std::unique_ptr<IOGroup> NVMLIOGroup::make_plugin(void)
    {
        return geopm::make_unique<NVMLIOGroup>();
    }

    void NVMLIOGroup::register_signal_alias(const std::string &alias_name,
                                            const std::string &signal_name)
    {
        if (m_signal_available.find(alias_name) != m_signal_available.end()) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": signal_name " + alias_name +
                            " was previously registered.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            // skip adding an alias if underlying signal is not found
            return;
        }
        // copy signal info but append to description
        m_signal_available[alias_name] = it->second;
        m_signal_available[alias_name].m_description =
            m_signal_available[signal_name].m_description + '\n' + "    alias_for: " + signal_name;
    }

    void NVMLIOGroup::register_control_alias(const std::string &alias_name,
                                           const std::string &control_name)
    {
        if (m_control_available.find(alias_name) != m_control_available.end()) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": contro1_name " + alias_name +
                            " was previously registered.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto it = m_control_available.find(control_name);
        if (it == m_control_available.end()) {
            // skip adding an alias if underlying control is not found
            return;
        }
        // copy control info but append to description
        m_control_available[alias_name] = it->second;
        m_control_available[alias_name].m_description =
        m_control_available[control_name].m_description + '\n' + "    alias_for: " + control_name;
    }
}
