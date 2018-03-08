/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include <set>

#include "ProfileIOSample.hpp"
#include "ProfileIO.hpp"
#include "CircularBuffer.hpp"
#include "config.h"

namespace geopm
{
    ProfileIOSample::ProfileIOSample(const std::vector<int> &cpu_rank)
    {
        m_rank_idx_map = ProfileIO::rank_to_node_local_rank(cpu_rank);
        m_cpu_rank = ProfileIO::rank_to_node_local_rank_per_cpu(cpu_rank);
        m_num_rank = m_rank_idx_map.size();

        // 2 samples for linear interpolation
        m_rank_sample_buffer.resize(m_num_rank, CircularBuffer<struct m_rank_sample_s>(2));
        m_region_id.resize(m_num_rank, GEOPM_REGION_ID_UNMARKED);
    }

    ProfileIOSample::~ProfileIOSample()
    {

    }

    void ProfileIOSample::update(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_begin,
                                 std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_end)
    {
        for (auto sample_it = prof_sample_begin; sample_it != prof_sample_end; ++sample_it) {
            if (!geopm_region_id_is_epoch(sample_it->second.region_id) &&
                sample_it->second.region_id != GEOPM_REGION_ID_UNMARKED) {
                struct m_rank_sample_s rank_sample;
                rank_sample.timestamp = sample_it->second.timestamp;
                rank_sample.progress = sample_it->second.progress;
                auto rank_idx_it = m_rank_idx_map.find(sample_it->second.rank);
#ifdef GEOPM_DEBUG
                if (rank_idx_it == m_rank_idx_map.end()) {
                    throw Exception("ProfileIOSample::update(): invalid profile sample data",
                                    GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
                }
#endif
                size_t rank_idx = rank_idx_it->second;
                if (sample_it->second.region_id != m_region_id[rank_idx]) {
                    m_rank_sample_buffer[rank_idx].clear();
                }
                if (rank_sample.progress == 1.0) {
                    m_region_id[rank_idx] = GEOPM_REGION_ID_UNMARKED;
                }
                else {
                    m_region_id[rank_idx] = sample_it->second.region_id;
                }
                m_rank_sample_buffer[rank_idx].insert(rank_sample);
            }
        }
    }

    std::vector<double> ProfileIOSample::per_cpu_progress(const struct geopm_time_s &extrapolation_time)
    {
        std::vector<double> result(m_cpu_rank.size(), 0.0);
        std::vector<double> rank_progress = per_rank_progress(extrapolation_time);
        int cpu_idx = 0;
        for (auto it : m_cpu_rank) {
            result[cpu_idx] = rank_progress[it];
            ++cpu_idx;
        }
        return result;
    }

    std::vector<double> ProfileIOSample::per_rank_progress(const struct geopm_time_s &extrapolation_time)
    {
        double delta;
        double factor;
        double dsdt;
        geopm_time_s timestamp_prev[2];
        std::vector<double> result(m_num_rank);
#ifdef GEOPM_DEBUG
        if (m_rank_sample_buffer.size() != m_num_rank) {
            throw Exception("m_rank_sample_buffer was wrong size", GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif

        auto result_it = result.begin();
        for (auto sample_it = m_rank_sample_buffer.begin();
             sample_it != m_rank_sample_buffer.end();
             ++sample_it, ++result_it) {
            switch(sample_it->size()) {
                case M_INTERP_TYPE_NONE:
                    *result_it = 0.0;
                    break;
                case M_INTERP_TYPE_NEAREST:
                    // if there is only one sample insert it directly
                    *result_it = sample_it->value(0).progress;
                    break;
                case M_INTERP_TYPE_LINEAR:
                    // if there are two samples, extrapolate to the given timestamp
                    timestamp_prev[0] = sample_it->value(0).timestamp;
                    timestamp_prev[1] = sample_it->value(1).timestamp;
                    delta = geopm_time_diff(timestamp_prev + 1, &extrapolation_time);
                    factor = 1.0 / geopm_time_diff(timestamp_prev, timestamp_prev + 1);
                    dsdt = (sample_it->value(1).progress - sample_it->value(0).progress) * factor;
                    dsdt = dsdt > 0.0 ? dsdt : 0.0; // progress does not decrease over time
                    if (sample_it->value(1).progress == 1.0) {
                        *result_it = 1.0;
                    }
                    else if (sample_it->value(0).progress == 0.0) {
                        // so we don't miss region entry
                        *result_it = 0.0;
                    }
                    else {
                        *result_it = sample_it->value(1).progress + dsdt * delta;
                        *result_it = *result_it >= 0.0 ? *result_it : 1e-9;
                        *result_it = *result_it <= 1.0 ? *result_it : 1 - 1e-9;
                    }
                    break;
                default:
#ifdef GEOPM_DEBUG
                    throw Exception("ProfileIOSample::align_prof() CircularBuffer has more than two values",
                                    GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
                    break;
            }
        }
        return result;
    }

    std::vector<uint64_t> ProfileIOSample::per_cpu_region_id(void)
    {
        std::vector<uint64_t> result(m_cpu_rank.size(), GEOPM_REGION_ID_UNMARKED);
        int cpu_idx = 0;
        for (auto rank : m_cpu_rank) {
            result[cpu_idx] = m_region_id[rank];
            ++cpu_idx;
        }
        return result;
    }
}
