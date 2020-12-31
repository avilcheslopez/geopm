/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ProfileIOGroup.hpp"

#include "PlatformTopo.hpp"
#include "ApplicationSampler.hpp"
#include "Helper.hpp"
#include "ProfileIOSample.hpp"
#include "Exception.hpp"
#include "Agg.hpp"
#include "geopm_hash.h"
#include "geopm_time.h"
#include "geopm_internal.h"
#include "config.h"

#define GEOPM_PROFILE_IO_GROUP_PLUGIN_NAME "PROFILE"

namespace geopm
{
    ProfileIOGroup::ProfileIOGroup()
        : ProfileIOGroup(platform_topo(), ApplicationSampler::application_sampler())
    {

    }

    ProfileIOGroup::ProfileIOGroup(const PlatformTopo &topo, ApplicationSampler &application_sampler)
        : m_application_sampler(application_sampler)
        , m_platform_topo(topo)
        , m_num_cpu(m_platform_topo.num_domain(GEOPM_DOMAIN_CPU))
        , m_do_read(M_NUM_SIGNAL, false)
        , m_is_batch_read(false)
        , m_per_cpu_hash(m_num_cpu, GEOPM_REGION_HASH_INVALID)
        , m_per_cpu_hint(m_num_cpu, GEOPM_REGION_HINT_UNSET)
        , m_per_cpu_progress(m_num_cpu, NAN)
        , m_is_connected(false)
        , m_is_pushed(false)
    {
        std::vector<std::pair<std::string, int> > aliases {
            {"REGION_HASH", M_SIGNAL_REGION_HASH},
            {"REGION_HINT", M_SIGNAL_REGION_HINT},
            {"REGION_THREAD_PROGRESS", M_SIGNAL_THREAD_PROGRESS},
            {"TIME_HINT_UNKNOWN", M_SIGNAL_TIME_HINT_UNKNOWN},
            {"TIME_HINT_UNSET", M_SIGNAL_TIME_HINT_UNSET},
            {"TIME_HINT_COMPUTE", M_SIGNAL_TIME_HINT_COMPUTE},
            {"TIME_HINT_MEMORY", M_SIGNAL_TIME_HINT_MEMORY},
            {"TIME_HINT_NETWORK", M_SIGNAL_TIME_HINT_NETWORK},
            {"TIME_HINT_IO", M_SIGNAL_TIME_HINT_IO},
            {"TIME_HINT_SERIAL", M_SIGNAL_TIME_HINT_SERIAL},
            {"TIME_HINT_PARALLEL", M_SIGNAL_TIME_HINT_PARALLEL},
            {"TIME_HINT_IGNORE", M_SIGNAL_TIME_HINT_IGNORE}
        };
        // same signal index for aliases and underlying signal
        for (const auto &name : aliases) {
            m_signal_idx_map[name.first] = name.second;
            m_signal_idx_map[plugin_name() + "::" + name.first] = name.second;
        }

    }

    ProfileIOGroup::~ProfileIOGroup()
    {

    }

    void ProfileIOGroup::connect(void)
    {
        m_cpu_rank = m_application_sampler.per_cpu_process();
        m_is_connected = true;
    }

    std::set<std::string> ProfileIOGroup::signal_names(void) const
    {
        std::set<std::string> result;
        if (m_is_connected) {
            for (const auto &sv : m_signal_idx_map) {
                result.insert(sv.first);
            }
        }
        return result;
    }

    std::set<std::string> ProfileIOGroup::control_names(void) const
    {
        return {};
    }

    bool ProfileIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return m_signal_idx_map.find(signal_name) != m_signal_idx_map.end();
    }

    bool ProfileIOGroup::is_valid_control(const std::string &control_name) const
    {
        return false;
    }

    int ProfileIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        if (is_valid_signal(signal_name)) {
            result = GEOPM_DOMAIN_CPU;
        }
        return result;
    }

    int ProfileIOGroup::control_domain_type(const std::string &control_name) const
    {
        return GEOPM_DOMAIN_INVALID;
    }

    int ProfileIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        int result = -1;
        if (m_is_batch_read) {
            throw Exception("ProfileIOGroup::push_signal: cannot push signal after call to read_batch().",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_is_pushed = true;
        int signal_type = check_signal(signal_name, domain_type, domain_idx);

        int signal_idx = 0;
        for (const auto &it : m_active_signal) {
            if (it.signal_type == signal_type &&
                it.domain_type == domain_type &&
                it.domain_idx == domain_idx) {
                result = signal_idx;
            }
            ++signal_idx;
        }
        if (result == -1) {
            result = m_active_signal.size();
            m_active_signal.push_back({signal_type, domain_type, domain_idx});
            m_do_read[signal_type] = true;
        }
        return result;
    }

    int ProfileIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
    {
        throw Exception("ProfileIOGroup::push_control() there are no controls supported by the ProfileIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void ProfileIOGroup::read_batch(void)
    {
        if (!m_is_connected) {
            connect();
        }
        if (!m_is_pushed) {
            return;
        }
        if (m_do_read[M_SIGNAL_REGION_HASH]) {
            // TODO: filter only pushed cpu idx?  copying everything probably cheaper than a branch
            for (int idx = 0; idx < m_num_cpu; ++idx) {
                m_per_cpu_hash[idx] = m_application_sampler.cpu_region_hash(idx);
            }
        }
        if (m_do_read[M_SIGNAL_REGION_HINT]) {
            for (int idx = 0; idx < m_num_cpu; ++idx) {
                m_per_cpu_hint[idx] = m_application_sampler.cpu_hint(idx);
            }
        }
        if (m_do_read[M_SIGNAL_THREAD_PROGRESS]) {
            for (int idx = 0; idx < m_num_cpu; ++idx) {
                m_per_cpu_progress[idx] = m_application_sampler.cpu_progress(idx);
            }
        }
        m_is_batch_read = true;
    }

    void ProfileIOGroup::write_batch(void)
    {

    }

    double ProfileIOGroup::sample(int signal_idx)
    {
        double result = NAN;
        if (signal_idx < 0 || signal_idx >= (int)m_active_signal.size()) {
            throw Exception("ProfileIOGroup::sample(): signal_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (!m_is_batch_read) {
            throw Exception("ProfileIOGroup::sample(): signal has not been read",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        int cpu_idx = m_active_signal[signal_idx].domain_idx;
        switch (m_active_signal[signal_idx].signal_type) {
            case M_SIGNAL_REGION_HASH:
                result = m_per_cpu_hash[cpu_idx];
                break;
            case M_SIGNAL_REGION_HINT:
                result = m_per_cpu_hint[cpu_idx];
                break;
            case M_SIGNAL_THREAD_PROGRESS:
                result = m_per_cpu_progress[cpu_idx];
                break;
            case M_SIGNAL_TIME_HINT_UNKNOWN:
                result = m_application_sampler.cpu_hint_time(cpu_idx, GEOPM_REGION_HINT_UNKNOWN);
                break;
            case M_SIGNAL_TIME_HINT_UNSET:
                result = m_application_sampler.cpu_hint_time(cpu_idx, GEOPM_REGION_HINT_UNSET);
                break;
            case M_SIGNAL_TIME_HINT_COMPUTE:
                result = m_application_sampler.cpu_hint_time(cpu_idx, GEOPM_REGION_HINT_COMPUTE);
                break;
            case M_SIGNAL_TIME_HINT_MEMORY:
                result = m_application_sampler.cpu_hint_time(cpu_idx, GEOPM_REGION_HINT_MEMORY);
                break;
            case M_SIGNAL_TIME_HINT_NETWORK:
                result = m_application_sampler.cpu_hint_time(cpu_idx, GEOPM_REGION_HINT_NETWORK);
                break;
            case M_SIGNAL_TIME_HINT_IO:
                result = m_application_sampler.cpu_hint_time(cpu_idx, GEOPM_REGION_HINT_IO);
                break;
            case M_SIGNAL_TIME_HINT_SERIAL:
                result = m_application_sampler.cpu_hint_time(cpu_idx, GEOPM_REGION_HINT_SERIAL);
                break;
            case M_SIGNAL_TIME_HINT_PARALLEL:
                result = m_application_sampler.cpu_hint_time(cpu_idx, GEOPM_REGION_HINT_PARALLEL);
                break;
            case M_SIGNAL_TIME_HINT_IGNORE:
                result = m_application_sampler.cpu_hint_time(cpu_idx, GEOPM_REGION_HINT_IGNORE);
                break;
            default:
#ifdef GEOPM_DEBUG
                throw Exception("ProfileIOGroup:sample(): Signal was pushed with an invalid signal type",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
                break;
        }

        return result;
    }

    void ProfileIOGroup::adjust(int control_idx, double setting)
    {
        throw Exception("ProfileIOGroup::adjust() there are no controls supported by the ProfileIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    double ProfileIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!m_is_connected) {
            throw Exception("ProfileIOGroup::read_signal() called before connect",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        int signal_type = check_signal(signal_name, domain_type, domain_idx);
        int cpu_idx = domain_idx;
        struct geopm_time_s read_time;
        uint64_t region_id;
        double result = NAN;
        switch (signal_type) {
            case M_SIGNAL_REGION_HASH:
                result = m_application_sampler.cpu_region_hash(cpu_idx);
                break;
            case M_SIGNAL_REGION_HINT:
                result = m_application_sampler.cpu_hint(cpu_idx);
                break;
            case M_SIGNAL_THREAD_PROGRESS:
                result = m_application_sampler.cpu_progress(cpu_idx);
                break;
            case M_SIGNAL_TIME_HINT_UNKNOWN:
                result = m_application_sampler.cpu_hint_time(cpu_idx, GEOPM_REGION_HINT_UNKNOWN);
                break;
            case M_SIGNAL_TIME_HINT_UNSET:
                result = m_application_sampler.cpu_hint_time(cpu_idx, GEOPM_REGION_HINT_UNSET);
                break;
            case M_SIGNAL_TIME_HINT_COMPUTE:
                result = m_application_sampler.cpu_hint_time(cpu_idx, GEOPM_REGION_HINT_COMPUTE);
                break;
            case M_SIGNAL_TIME_HINT_MEMORY:
                result = m_application_sampler.cpu_hint_time(cpu_idx, GEOPM_REGION_HINT_MEMORY);
                break;
            case M_SIGNAL_TIME_HINT_NETWORK:
                result = m_application_sampler.cpu_hint_time(cpu_idx, GEOPM_REGION_HINT_NETWORK);
                break;
            case M_SIGNAL_TIME_HINT_IO:
                result = m_application_sampler.cpu_hint_time(cpu_idx, GEOPM_REGION_HINT_IO);
                break;
            case M_SIGNAL_TIME_HINT_SERIAL:
                result = m_application_sampler.cpu_hint_time(cpu_idx, GEOPM_REGION_HINT_SERIAL);
                break;
            case M_SIGNAL_TIME_HINT_PARALLEL:
                result = m_application_sampler.cpu_hint_time(cpu_idx, GEOPM_REGION_HINT_PARALLEL);
                break;
            case M_SIGNAL_TIME_HINT_IGNORE:
                result = m_application_sampler.cpu_hint_time(cpu_idx, GEOPM_REGION_HINT_IGNORE);
                break;
            default:
#ifdef GEOPM_DEBUG
                throw Exception("ProfileIOGroup:read_signal(): Invalid signal type bug check_signal did not throw",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
                break;
        }
        return result;
    }

    void ProfileIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
    {
        throw Exception("ProfileIOGroup::write_control() there are no controls supported by the ProfileIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void ProfileIOGroup::save_control(void)
    {

    }

    void ProfileIOGroup::restore_control(void)
    {

    }

    std::function<double(const std::vector<double> &)> ProfileIOGroup::agg_function(const std::string &signal_name) const
    {
        static const std::map<std::string, std::function<double(const std::vector<double> &)> > fn_map {
            {"REGION_THREAD_PROGRESS", Agg::min},
            {"PROFILE::REGION_THREAD_PROGRESS", Agg::min},
            {"REGION_HASH", Agg::region_hash},
            {"PROFILE::REGION_HASH", Agg::region_hash},
            {"REGION_HINT", Agg::region_hint},
            {"PROFILE::REGION_HINT", Agg::region_hint},
            {"TIME_HINT_UNKNOWN", Agg::average},
            {"PROFILE::TIME_HINT_UNKNOWN", Agg::average},
            {"TIME_HINT_UNSET", Agg::average},
            {"PROFILE::TIME_HINT_UNSET", Agg::average},
            {"TIME_HINT_COMPUTE", Agg::average},
            {"PROFILE::TIME_HINT_COMPUTE", Agg::average},
            {"TIME_HINT_MEMORY", Agg::average},
            {"PROFILE::TIME_HINT_MEMORY", Agg::average},
            {"TIME_HINT_NETWORK", Agg::average},
            {"PROFILE::TIME_HINT_NETWORK", Agg::average},
            {"TIME_HINT_IO", Agg::average},
            {"PROFILE::TIME_HINT_IO", Agg::average},
            {"TIME_HINT_SERIAL", Agg::average},
            {"PROFILE::TIME_HINT_SERIAL", Agg::average},
            {"TIME_HINT_PARALLEL", Agg::average},
            {"PROFILE::TIME_HINT_PARALLEL", Agg::average},
            {"TIME_HINT_IGNORE", Agg::average},
            {"PROFILE::TIME_HINT_IGNORE", Agg::average},
        };
        auto it = fn_map.find(signal_name);
        if (it == fn_map.end()) {
            throw Exception("ProfileIOGroup::agg_function(): unknown how to aggregate \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second;
    }

    std::function<std::string(double)> ProfileIOGroup::format_function(const std::string &signal_name) const
    {
       static const std::map<std::string, std::function<std::string(double)> > fmt_map {
            {"REGION_THREAD_PROGRESS", string_format_float},
            {"PROFILE::REGION_THREAD_PROGRESS", string_format_float},
            {"REGION_HASH", string_format_hex},
            {"PROFILE::REGION_HASH", string_format_hex},
            {"REGION_HINT", string_format_hex},
            {"PROFILE::REGION_HINT", string_format_hex},
            {"TIME_HINT_UNKNOWN", string_format_double},
            {"PROFILE::TIME_HINT_UNKNOWN", string_format_double},
            {"TIME_HINT_UNSET", string_format_double},
            {"PROFILE::TIME_HINT_UNSET", string_format_double},
            {"TIME_HINT_COMPUTE", string_format_double},
            {"PROFILE::TIME_HINT_COMPUTE", string_format_double},
            {"TIME_HINT_MEMORY", string_format_double},
            {"PROFILE::TIME_HINT_MEMORY", string_format_double},
            {"TIME_HINT_NETWORK", string_format_double},
            {"PROFILE::TIME_HINT_NETWORK", string_format_double},
            {"TIME_HINT_IO", string_format_double},
            {"PROFILE::TIME_HINT_IO", string_format_double},
            {"TIME_HINT_SERIAL", string_format_double},
            {"PROFILE::TIME_HINT_SERIAL", string_format_double},
            {"TIME_HINT_PARALLEL", string_format_double},
            {"PROFILE::TIME_HINT_PARALLEL", string_format_double},
            {"TIME_HINT_IGNORE", string_format_double},
            {"PROFILE::TIME_HINT_IGNORE", string_format_double},
        };
        auto it = fmt_map.find(signal_name);
        if (it == fmt_map.end()) {
            throw Exception("ProfileIOGroup::format_function(): unknown how to format \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second;
    }


    std::string ProfileIOGroup::signal_description(const std::string &signal_name) const
    {
        return "";
    }

    std::string ProfileIOGroup::control_description(const std::string &control_name) const
    {
        return "";
    }

    int ProfileIOGroup::check_signal(const std::string &signal_name, int domain_type, int domain_idx) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("ProfileIOGroup::check_signal(): signal_name " + signal_name +
                            " not valid for ProfileIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != GEOPM_DOMAIN_CPU) {
            throw Exception("ProfileIOGroup::check_signal(): non-CPU domains are not supported",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int cpu_idx = domain_idx;
        if (cpu_idx < 0 || cpu_idx >= m_platform_topo.num_domain(GEOPM_DOMAIN_CPU)) {
            throw Exception("ProfileIOGroup::check_signal(): domain index out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int signal_type = -1;
        auto it = m_signal_idx_map.find(signal_name);
        if (it != m_signal_idx_map.end()) {
            signal_type = it->second;
        }
#ifdef GEOPM_DEBUG
        else {
            throw Exception("ProfileIOGroup::check_signal: is_valid_signal() returned true, but signal name is unknown",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return signal_type;
    }

    std::string ProfileIOGroup::plugin_name(void)
    {
        return GEOPM_PROFILE_IO_GROUP_PLUGIN_NAME;
    }

    std::unique_ptr<IOGroup> ProfileIOGroup::make_plugin(void)
    {
        return geopm::make_unique<ProfileIOGroup>();
    }
}
