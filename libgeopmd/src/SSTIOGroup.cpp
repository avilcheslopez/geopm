/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "SSTIOGroup.hpp"

#include <algorithm>
#include <cinttypes>
#include <iomanip>
#include <iterator>
#include <sstream>

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "MSR.hpp"
#include "MSRFieldSignal.hpp"
#include "geopm/PlatformTopo.hpp"
#include "SSTControl.hpp"
#include "SSTIO.hpp"
#include "SSTSignal.hpp"
#include "geopm_debug.hpp"
#include "geopm_topo.h"
#include "geopm/SaveControl.hpp"

namespace geopm
{
    const std::map<std::string, SSTIOGroup::sst_signal_mailbox_raw_s> SSTIOGroup::sst_signal_mbox_info = {
        { "SST::CONFIG_LEVEL",
          { SSTIOGroup::SSTMailboxCommand::M_TURBO_FREQUENCY, 0x00,
            {{ "LEVEL", { 0x00, 16, 23, 1.0, M_UNITS_NONE,
                            "SST configuration level", M_SIGNAL_BEHAVIOR_CONSTANT } }}
          } },
        { "SST::TURBOFREQ_SUPPORT",
          { SSTIOGroup::SSTMailboxCommand::M_TURBO_FREQUENCY, 0x01,
            {{ "SUPPORTED", { 0x00, 0, 0, 1.0, M_UNITS_NONE,
                              "SST-TF is supported",
                              M_SIGNAL_BEHAVIOR_CONSTANT,
                              Agg::sum } }}
          } },
        { "SST::HIGHPRIORITY_NCORES",
          { SSTIOGroup::SSTMailboxCommand::M_TURBO_FREQUENCY, 0x10,
            {{ "0", { 0x0000, 0, 7, 1.0, M_UNITS_NONE,
                      "Count of high-priority turbo frequency cores in bucket 0",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "1", { 0x0000, 8, 15, 1.0, M_UNITS_NONE,
                      "Count of high-priority turbo frequency cores in bucket 1",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "2", { 0x0000, 16, 23, 1.0, M_UNITS_NONE,
                      "Count of high-priority turbo frequency cores in bucket 2",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "3", { 0x0000, 24, 31, 1.0, M_UNITS_NONE,
                      "Count of high-priority turbo frequency cores in bucket 3",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "4", { 0x0100, 0, 7, 1.0, M_UNITS_NONE,
                      "Count of high-priority turbo frequency cores in bucket 4",
                       M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "5", { 0x0100, 8, 15, 1.0, M_UNITS_NONE,
                      "Count of high-priority turbo frequency cores in bucket 5",
                       M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "6", { 0x0100, 16, 23, 1.0, M_UNITS_NONE,
                      "Count of high-priority turbo frequency cores in bucket 6",
                       M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "7", { 0x0100, 24, 31, 1.0, M_UNITS_NONE,
                      "Count of high-priority turbo frequency cores in bucket 7",
                      M_SIGNAL_BEHAVIOR_CONSTANT } }}
          } },
        { "SST::HIGHPRIORITY_FREQUENCY_SSE",
          { SSTIOGroup::SSTMailboxCommand::M_TURBO_FREQUENCY, 0x11,
            {
             { "0", { 0x000000, 0, 7, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 0 at the SSE license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "1", { 0x000000, 8, 15, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 1 at the SSE license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "2", { 0x000000, 16, 23, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 2 at the SSE license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "3", { 0x000000, 24, 31, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 3 at the SSE license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "4", { 0x000100, 0, 7, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 4 at the SSE license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "5", { 0x000100, 8, 15, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 5 at the SSE license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "6", { 0x000100, 16, 23, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 6 at the SSE license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "7", { 0x000100, 24, 31, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 7 at the SSE license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } }}
          } },
        { "SST::HIGHPRIORITY_FREQUENCY_AVX2",
          { SSTIOGroup::SSTMailboxCommand::M_TURBO_FREQUENCY, 0x11,
            {{ "0", { 0x010000, 0, 7, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 0 at the AVX2 license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "1", { 0x010000, 8, 15, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 1 at the AVX2 license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "2", { 0x010000, 16, 23, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 2 at the AVX2 license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "3", { 0x010000, 24, 31, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 3 at the AVX2 license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "4", { 0x010100, 0, 7, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 4 at the AVX2 license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "5", { 0x010100, 8, 15, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 5 at the AVX2 license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "6", { 0x010100, 16, 23, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 6 at the AVX2 license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "7", { 0x010100, 24, 31, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 7 at the AVX2 license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } }}
          } },
        { "SST::HIGHPRIORITY_FREQUENCY_AVX512",
          { SSTIOGroup::SSTMailboxCommand::M_TURBO_FREQUENCY, 0x11,
            {{ "0", { 0x020000, 0, 7, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 0 at the AVX2 license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "1", { 0x020000, 8, 15, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 1 at the AVX2 license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "2", { 0x020000, 16, 23, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 2 at the AVX2 license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "3", { 0x020000, 24, 31, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 3 at the AVX2 license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "4", { 0x020100, 0, 7, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 4 at the AVX2 license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "5", { 0x020100, 8, 15, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 5 at the AVX2 license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "6", { 0x020100, 16, 23, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 6 at the AVX2 license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "7", { 0x020100, 24, 31, 1e8, M_UNITS_HERTZ,
                      "High-priority turbo frequency for bucket 7 at the AVX2 license level",
                      M_SIGNAL_BEHAVIOR_CONSTANT } }}
          } },
        { "SST::LOWPRIORITY_FREQUENCY",
          { SSTIOGroup::SSTMailboxCommand::M_TURBO_FREQUENCY, 0x12,
            {{ "SSE", { 0x00, 0, 7, 1e8, M_UNITS_HERTZ,
                        "Low-priority turbo frequency at the SSE license level",
                        M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "AVX2", { 0x00, 8, 15, 1e8, M_UNITS_HERTZ,
                         "Low-priority turbo frequency at the AVX2 license level",
                         M_SIGNAL_BEHAVIOR_CONSTANT } },
             { "AVX512", { 0x00, 16, 23, 1e8, M_UNITS_HERTZ,
                           "Low-priority turbo frequency at the AVX512 license level",
                           M_SIGNAL_BEHAVIOR_CONSTANT } }}
          } },
        { "SST::COREPRIORITY_SUPPORT",
          { SSTIOGroup::SSTMailboxCommand::M_SUPPORT_CAPABILITIES, 0x03,
            {{ "CAPABILITIES", { 0x00, 0, 0, 1.0, M_UNITS_NONE,
                                 "SST-CP is supported",
                                 M_SIGNAL_BEHAVIOR_CONSTANT,
                                 Agg::sum } }}
          } }
    };

    const std::map<std::string, SSTIOGroup::sst_control_mailbox_raw_s> SSTIOGroup::sst_control_mbox_info = {
        { "SST::TURBO_ENABLE",
            { SSTIOGroup::SSTMailboxCommand::M_TURBO_FREQUENCY,
                // Control
                0x02, 0x00 /* N/A */,
                {{ "ENABLE", { 0x01, 16, 16, M_UNITS_NONE,
                               "SST-TF is enabled. Enabling this also enables SST::COREPRIORITY_ENABLE:ENABLE.",
                               Agg::sum } }},
                // Signal
                0x01, 0x00
            },
        },
        { "SST::COREPRIORITY_ENABLE",
            // 0x03 when enabling; 0x01 when disabling
            { SSTIOGroup::SSTMailboxCommand::M_CORE_PRIORITY,
                // Control
                0x02, 0x100,
                {{ "ENABLE", { 0x01, 1, 1, M_UNITS_NONE,
                               "SST-CP is enabled. Disabling this also disables SST::TURBO_ENABLE:ENABLE.",
                               Agg::sum } }},
                // Signal
                0x02, 0x00
            },
        },
    };

    const std::map<std::string, SSTIOGroup::sst_control_mmio_raw_s> SSTIOGroup::sst_control_mmio_info = {
        { "SST::COREPRIORITY:0",
          { GEOPM_DOMAIN_PACKAGE, 0x08,
            { { "PRIORITY", { 4, 7, 15.0, M_UNITS_NONE, "Proportional priority for core priority level 0, "
                                                        "ranging from 0 to 1. A lower value indicates a "
                                                        "desire to receive a greater share of surplus power "
                                                        "than priority groups with a higher value." } },
              { "FREQUENCY_MIN", { 8, 15, 1e-8, M_UNITS_HERTZ, "Minimum frequency of core priority level 0"  } },
              { "FREQUENCY_MAX", { 16, 23, 1e-8, M_UNITS_HERTZ, "Maximum frequency of core priority level 0"  } } } } },
        { "SST::COREPRIORITY:1",
          { GEOPM_DOMAIN_PACKAGE, 0x0c,
            { { "PRIORITY", { 4, 7, 15.0, M_UNITS_NONE, "Proportional priority for core priority level 1, "
                                                        "ranging from 0 to 1. A lower value indicates a "
                                                        "desire to receive a greater share of surplus power "
                                                        "than priority groups with a higher value." } },
              { "FREQUENCY_MIN", { 8, 15, 1e-8, M_UNITS_HERTZ, "Minimum frequency of core priority level 1"  } },
              { "FREQUENCY_MAX", { 16, 23, 1e-8, M_UNITS_HERTZ, "Maximum frequency of core priority level 1"  } } } } },
        { "SST::COREPRIORITY:2",
          { GEOPM_DOMAIN_PACKAGE, 0x10,
            { { "PRIORITY", { 4, 7, 15.0, M_UNITS_NONE, "Proportional priority for core priority level 2, "
                                                        "ranging from 0 to 1. A lower value indicates a "
                                                        "desire to receive a greater share of surplus power "
                                                        "than priority groups with a higher value." } },
              { "FREQUENCY_MIN", { 8, 15, 1e-8, M_UNITS_HERTZ, "Minimum frequency of core priority level 2"  } },
              { "FREQUENCY_MAX", { 16, 23, 1e-8, M_UNITS_HERTZ, "Maximum frequency of core priority level 2"  } } } } },
        { "SST::COREPRIORITY:3",
          { GEOPM_DOMAIN_PACKAGE, 0x14,
            { { "PRIORITY", { 4, 7, 15.0, M_UNITS_NONE, "Proportional priority for core priority level 3, "
                                                        "ranging from 0 to 1. A lower value indicates a "
                                                        "desire to receive a greater share of surplus power "
                                                        "than priority groups with a higher value." } },
              { "FREQUENCY_MIN", { 8, 15, 1e-8, M_UNITS_HERTZ, "Minimum frequency of core priority level 3"  } },
              { "FREQUENCY_MAX", { 16, 23, 1e-8, M_UNITS_HERTZ, "Maximum frequency of core priority level 3"  } } } } },
        { "SST::COREPRIORITY",
          { GEOPM_DOMAIN_CORE, 0x20, /* offset will be augmented by core index */
            { { "ASSOCIATION", { 16, 17, 1.0, M_UNITS_NONE, "Assigned core priority level"  } } } } },
    };

    SSTIOGroup::SSTIOGroup()
        : SSTIOGroup(platform_topo(), nullptr, nullptr)
    {

    }

    SSTIOGroup::SSTIOGroup(const PlatformTopo &topo, std::shared_ptr<SSTIO> sstio, std::shared_ptr<SaveControl> save_control)
        : m_topo(topo)
        , m_sstio(std::move(sstio))
        , m_is_read(false)
        , m_signal_available()
        , m_control_available()
        , m_signal_pushed()
        , m_control_pushed()
        , m_mock_save_ctl(std::move(save_control))
    {
        if (m_sstio == nullptr) {
            m_sstio = SSTIO::make_shared(m_topo.num_domain(GEOPM_DOMAIN_CPU));
        }

        // Directly register MBOX-based signals
        for (const auto &kv : sst_signal_mbox_info) {
            const auto &raw_name = kv.first;
            const auto &raw_desc = kv.second;

            add_mbox_signals(raw_name, raw_desc.command, raw_desc.subcommand,
                             raw_desc.fields);
        }

        // For MBOX-based controls, register both a control and a signal. The
        // control needs to be aware of how the signal reads are performed so
        // it can do software read/modify/write.
        for (const auto &kv : sst_control_mbox_info) {
            const auto &raw_name = kv.first;
            const auto &raw_desc = kv.second;

            // Create a read mask for pre-write reads in the control. The mask
            // is a union of all known fields.
            uint32_t control_read_mask = 0;

            std::map<std::string, sst_signal_mailbox_field_s> fields;
            for (const auto& field : raw_desc.fields)
            {
                fields.emplace(field.first,
                               sst_signal_mailbox_field_s{
                                   raw_desc.read_request_data, field.second.begin_bit,
                                   field.second.end_bit, 1.0, field.second.units,
                                   field.second.description, M_SIGNAL_BEHAVIOR_VARIABLE });
                auto bit_count = field.second.end_bit - field.second.begin_bit + 1;
                auto field_mask = ((1ull << bit_count) - 1) << field.second.begin_bit;
                control_read_mask |= field_mask;
            }

            add_mbox_signals(raw_name, raw_desc.command,
                             raw_desc.read_subcommand, fields);
            add_mbox_controls(raw_name, raw_desc.command, raw_desc.subcommand,
                              raw_desc.write_param, raw_desc.fields,
                              raw_desc.read_subcommand,
                              raw_desc.read_request_data, control_read_mask);
        }

        // Both SST-TF and SST-CP controls should have been added now. Make them
        // refer into each other for enables/disables.
        // This is needed because the driver will fail to commit the request if
        // we try to enable SST-TF while SST-CP is disabled, or if we try to
        // disable SST-CP while SST-TF is enabled.
        auto sst_tf_enable_it = m_control_available.find("SST::TURBO_ENABLE:ENABLE");
        auto sst_cp_enable_it = m_control_available.find("SST::COREPRIORITY_ENABLE:ENABLE");
        GEOPM_DEBUG_ASSERT(
                sst_tf_enable_it != m_control_available.end()
                && sst_cp_enable_it != m_control_available.end(),
                "Do not have controls to enable SST-TF and SST-CP");
        GEOPM_DEBUG_ASSERT(
                sst_tf_enable_it->second.domain == sst_cp_enable_it->second.domain,
                "SST-TF and SST-CP cannot be enabled in the same domain");
        if (sst_tf_enable_it != m_control_available.end() && sst_cp_enable_it != m_control_available.end()) {
            const auto domain_count = std::min(
                    sst_tf_enable_it->second.controls.size(),
                    sst_cp_enable_it->second.controls.size());
            for (size_t i = 0; i < domain_count; ++i) {
                auto sst_tf_control = std::static_pointer_cast<SSTControl>(sst_tf_enable_it->second.controls[i]);
                auto sst_cp_control = std::static_pointer_cast<SSTControl>(sst_cp_enable_it->second.controls[i]);
                sst_tf_control->set_write_dependency(1, sst_cp_control, 1);
                sst_cp_control->set_write_dependency(0, sst_tf_control, 0);
            }
        }

        // This IOGroup currently has no MMIO-based signals, except for those
        // that are registered with their related controls below.

        // For MMIO-based controls, register both a control and a signal.
        for (const auto &kv : sst_control_mmio_info) {
            const auto &raw_name = kv.first;
            const auto &raw_desc = kv.second;

            uint32_t control_read_mask = 0;

            std::map<std::string, sst_signal_mmio_field_s> fields;
            for (const auto& field : raw_desc.fields)
            {
                fields.emplace(field.first,
                               sst_signal_mmio_field_s{
                                   0, field.second.begin_bit, field.second.end_bit,
                                   1 / field.second.multiplier, field.second.units,
                                   field.second.description, M_SIGNAL_BEHAVIOR_VARIABLE });
                auto bit_count = field.second.end_bit - field.second.begin_bit + 1;
                auto field_mask = ((1ull << bit_count) - 1) << field.second.begin_bit;
                control_read_mask |= field_mask;
            }

            add_mmio_signals(raw_name, raw_desc.domain_type, raw_desc.register_offset, fields);
            add_mmio_controls(raw_name, raw_desc.domain_type, raw_desc.register_offset,
                              raw_desc.fields, control_read_mask);
        }

        // Attempt to read the priority of each core. On a system that does not
        // support this action, the read_signal will throw. Let that exception
        // bubble up to indicated to the owner of this IOGroup that this
        // IOGroup should not be loaded.
        for (int core = 0; core < m_topo.num_domain(GEOPM_DOMAIN_CORE); ++core) {
            read_signal("SST::COREPRIORITY:ASSOCIATION", GEOPM_DOMAIN_CORE, core);
        }
    }

    std::set<std::string> SSTIOGroup::signal_names(void) const
    {
        std::set<std::string> result;
        for (const auto &kv : m_signal_available) {
            result.insert(kv.first);
        }
        return result;
    }

    std::set<std::string> SSTIOGroup::control_names(void) const
    {
        std::set<std::string> names;
        for (const auto &kv : m_control_available) {
            names.insert(kv.first);
        }
        return names;
    }

    bool SSTIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return (m_signal_available.find(signal_name) != m_signal_available.end());
    }

    bool SSTIOGroup::is_valid_control(const std::string &control_name) const
    {
        return m_control_available.find(control_name) != m_control_available.end();
    }

    int SSTIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        auto it = m_signal_available.find(signal_name);
        if (it != m_signal_available.end()) {
            result = it->second.domain;
        }
        return result;
    }

    int SSTIOGroup::control_domain_type(const std::string &control_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        auto it = m_control_available.find(control_name);
        if (it != m_control_available.end()) {
            result = it->second.domain;
        }
        return result;
    }

    int SSTIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        int result = -1;
        auto it = m_signal_available.find(signal_name);
        if (it != m_signal_available.end()) {
            if (domain_type != it->second.domain) {
                throw Exception("SSTIOGroup::push_signal(): wrong domain type",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            auto signal = it->second.signals[domain_idx];
            auto already_pushed_signal = std::find(m_signal_pushed.begin(),
                                                   m_signal_pushed.end(), signal);
            if (already_pushed_signal == m_signal_pushed.end()) {
                result = m_signal_pushed.size();
                m_signal_pushed.push_back(signal);
                signal->setup_batch();
            }
            else {
                result = std::distance(m_signal_pushed.begin(), already_pushed_signal);
            }
        }
        else {
            throw Exception("SSTIOGroup::push_signal(): invalid signal",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    int SSTIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
    {
        int result = -1;
        if (control_name == "SST::TURBO_ENABLE:ENABLE"
                || control_name == "SST::COREPRIORITY_ENABLE:ENABLE") {
            throw Exception("SSTIOGroup::push_control(): SST::TURBO_ENABLE:ENABLE "
                            "and SST::COREPRIORITY_ENABLE:ENABLE cannot be pushed "
                            "in batch writes.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto it = m_control_available.find(control_name);
        if (it != m_control_available.end()) {
            if (domain_type != it->second.domain) {
                throw Exception("SSTIOGroup::push_control(): wrong domain type",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            auto control = it->second.controls[domain_idx];
            auto already_pushed_control = std::find(
                m_control_pushed.begin(), m_control_pushed.end(), control);
            if (already_pushed_control == m_control_pushed.end()) {
                result = m_control_pushed.size();
                m_control_pushed.push_back(control);
                control->setup_batch();
            }
            else {
                result = std::distance(m_control_pushed.begin(), already_pushed_control);
            }
        }
        else {
            throw Exception("SSTIOGroup::push_control(): invalid control",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    void SSTIOGroup::read_batch(void)
    {
        m_sstio->read_batch();
        m_is_read = true;
    }

    void SSTIOGroup::write_batch(void)
    {
        m_sstio->write_batch();
    }

    double SSTIOGroup::sample(int batch_idx)
    {
        if (batch_idx < 0 || batch_idx >= static_cast<int>(m_signal_pushed.size())) {
            throw Exception("SSTIOGroup::sample(): batch_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (!m_is_read) {
            throw Exception("SSTIOGroup::sample() called before the signal was read.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return m_signal_pushed[batch_idx]->sample();
    }

    void SSTIOGroup::adjust(int batch_idx, double setting)
    {
        m_control_pushed[batch_idx]->adjust(setting);
    }

    double SSTIOGroup::read_signal(const std::string &signal_name,
                                   int domain_type, int domain_idx)
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("SSTIOGroup::read_signal(): invalid signal",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (domain_type != it->second.domain) {
            throw Exception("SSTIOGroup::read_signal(): wrong domain type",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        return it->second.signals[domain_idx]->read();
    }

    void SSTIOGroup::write_control(const std::string &control_name,
                                   int domain_type, int domain_idx, double setting)
    {
        auto it = m_control_available.find(control_name);
        if (it == m_control_available.end()) {
            throw Exception("SSTIOGroup::write_control(): invalid control",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (domain_type != it->second.domain) {
            throw Exception("SSTIOGroup::write_control(): wrong domain type",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        it->second.controls[domain_idx]->write(setting);
    }

    void SSTIOGroup::save_control(void)
    {
        // Try both safe/restore at the time of save. We can detect potential
        // restore failures before we actually need to restore later. If any
        // failures are encountered now, remove the control from our list of
        // available controls.
        std::vector<std::string> unallowed_controls;
        for (auto &control : m_control_available) {
            try {
                for (auto &domain_control : control.second.controls) {
                    domain_control->save();
                    domain_control->restore();
                }
            }
            catch (...) {
                unallowed_controls.push_back(control.first);
            }
        }
        for (auto &control : unallowed_controls) {
            m_control_available.erase(control);
        }
    }

    void SSTIOGroup::restore_control(void)
    {
        for (auto &control : m_control_available) {
            for (auto &domain_control : control.second.controls) {
                domain_control->restore();
            }
        }
    }

    std::string SSTIOGroup::name(void) const
    {
        return plugin_name();
    }

    std::string SSTIOGroup::plugin_name(void)
    {
        return "SST";
    }

    std::unique_ptr<IOGroup> SSTIOGroup::make_plugin(void)
    {
        return geopm::make_unique<SSTIOGroup>();
    }

    std::function<double(const std::vector<double> &)> SSTIOGroup::agg_function(const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("SSTIOGroup::agg_function(): " + signal_name +
                            "not valid for SSTIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second.agg_function;
    }

    std::function<std::string(double)> SSTIOGroup::format_function(const std::string &signal_name) const
    {
        std::function<std::string(double)> result = string_format_double;
        if (!is_valid_signal(signal_name)) {
            throw Exception("SSTIOGroup::format_function(): " + signal_name +
                            "not valid for SSTIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (string_ends_with(signal_name, "#")) {
            result = string_format_hex;
        }
        return result;
    }

    std::string SSTIOGroup::signal_description(const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("SSTIOGroup::signal_description(): " + signal_name +
                                "not valid for SSTIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::ostringstream oss;
        oss << "    description: " << it->second.description << "\n"
            << "    units: " << IOGroup::units_to_string(it->second.units) << "\n"
            << "    aggregation: " << Agg::function_to_name(it->second.agg_function) << "\n"
            << "    domain: " << platform_topo().domain_type_to_name(it->second.domain) << "\n"
            << "    iogroup: SSTIOGroup";

        return oss.str();
    }

    std::string SSTIOGroup::control_description(const std::string &control_name) const
    {
        auto it = m_control_available.find(control_name);
        if (it == m_control_available.end()) {
            throw Exception("SSTIOGroup::control_description(): " + control_name +
                                "not valid for SSTIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::ostringstream oss;
        oss << "    description: " << it->second.description << "\n"
            << "    units: " << IOGroup::units_to_string(it->second.units) << "\n"
            << "    aggregation: " << Agg::function_to_name(it->second.agg_function) << "\n"
            << "    domain: " << platform_topo().domain_type_to_name(it->second.domain) << "\n"
            << "    iogroup: SSTIOGroup";

        return oss.str();
    }

    int SSTIOGroup::signal_behavior(const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("SSTIOGroup::signal_behavior(): " + signal_name +
                                "not valid for SSTIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        return it->second.behavior;
    }

    void SSTIOGroup::save_control(const std::string &save_path)
    {
        std::shared_ptr<SaveControl> save_ctl = m_mock_save_ctl;
        if (save_ctl == nullptr) {
            save_ctl = SaveControl::make_unique(*this);
        }
        save_ctl->write_json(save_path);
    }

    void SSTIOGroup::restore_control(const std::string &save_path)
    {
        std::shared_ptr<SaveControl> save_ctl = m_mock_save_ctl;
        if (save_ctl == nullptr) {
            save_ctl = SaveControl::make_unique(geopm::read_file(save_path));
        }
        save_ctl->restore(*this);
    }

    void SSTIOGroup::add_mbox_signals(
        const std::string &raw_name, SSTIOGroup::SSTMailboxCommand command,
        uint16_t subcommand, const std::map<std::string, sst_signal_mailbox_field_s> &fields)

    {
        int domain_type = GEOPM_DOMAIN_PACKAGE;
        int num_domain = m_topo.num_domain(domain_type);

        for (const auto &ff : fields) {
            const auto &field_name = ff.first;
            const auto &field_desc = ff.second;
            auto request_data = field_desc.request_data;
            auto begin_bit = field_desc.begin_bit;
            auto end_bit = field_desc.end_bit;
            auto &description = field_desc.description;
            auto behavior = field_desc.behavior;
            auto units = field_desc.units;
            double multiplier = field_desc.multiplier;
            auto &agg_function = field_desc.agg_function;

            char hex[32];
            snprintf(hex, 32, "0x%05" PRIx32, request_data);
            std::string raw_signal_name = raw_name + "_" + hex + "#";

            // add raw signal for every domain index
            auto it = m_signal_available.find(raw_signal_name);
            if (it == m_signal_available.end()) {
                std::vector<std::shared_ptr<Signal> > signals;
                for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                    auto cpus = m_topo.domain_nested(GEOPM_DOMAIN_CPU, domain_type, domain_idx);
                    int cpu_idx = *(cpus.begin());
                    auto raw_sst = std::make_shared<SSTSignal>(
                        m_sstio, SSTSignal::M_MBOX, cpu_idx, static_cast<uint16_t>(command),
                        subcommand, request_data, 0 /* interface parameter */);
                    signals.push_back(std::move(raw_sst));
                }
                m_signal_available[raw_signal_name] = {
                    .signals = std::move(signals),
                    .domain = domain_type,
                    .units = units,
                    .agg_function = agg_function,
                    .description = description,
                    .behavior = behavior
                };
            }
            std::string field_signal_name = raw_name + ":" + field_name;
            auto raw_sst_list = m_signal_available.at(raw_signal_name).signals;
            std::vector<std::shared_ptr<Signal> > signals;
            for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                auto field_signal = std::make_shared<MSRFieldSignal>(
                    raw_sst_list[domain_idx], begin_bit, end_bit,
                    MSR::M_FUNCTION_SCALE, multiplier);
                signals.push_back(field_signal);
            }
            m_signal_available[field_signal_name] = {
                .signals = std::move(signals),
                .domain = domain_type,
                .units = units,
                .agg_function = agg_function,
                .description = description,
                .behavior = behavior
            };
        }
    }

    void SSTIOGroup::add_mbox_controls(
        const std::string &raw_name, SSTMailboxCommand command,
        uint16_t subcommand, uint32_t write_param,
        const std::map<std::string, sst_control_mailbox_field_s> &fields,
        uint16_t read_subcommand, uint32_t read_request_data, uint32_t read_mask)
    {
        int domain_type = GEOPM_DOMAIN_PACKAGE;
        int num_domain = m_topo.num_domain(domain_type);

        for (const auto &ff : fields) {
            const auto &field_name = ff.first;
            const auto &field_description = ff.second;
            auto write_data = field_description.write_data;
            auto begin_bit = field_description.begin_bit;
            auto end_bit = field_description.end_bit;
            auto &description = field_description.description;
            auto units = field_description.units;
            auto &agg_function = field_description.agg_function;

            std::string field_control_name = raw_name + ":" + field_name;

            // add raw control for every domain index
            auto it = m_control_available.find(field_control_name);
            if (it == m_control_available.end()) {
                std::vector<std::shared_ptr<Control> > controls;
                for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                    auto cpus = m_topo.domain_nested(GEOPM_DOMAIN_CPU,
                                                     domain_type, domain_idx);
                    int cpu_idx = *(cpus.begin());

                    auto raw_sst = std::make_shared<SSTControl>(
                        m_sstio, SSTControl::M_MBOX, cpu_idx, static_cast<uint16_t>(command),
                        subcommand, write_param, write_data, begin_bit, end_bit,
                        1.0, read_subcommand, read_request_data, read_mask);

                    controls.push_back(std::move(raw_sst));
                }
                m_control_available[field_control_name] = {
                    .controls = std::move(controls),
                    .domain = domain_type,
                    .units = units,
                    .agg_function = agg_function,
                    .description = description
                };
            }
        }
    }

    void SSTIOGroup::add_mmio_signals(const std::string &raw_name,
                                      int domain_type, uint32_t register_offset,
                                      const std::map<std::string, sst_signal_mmio_field_s> &fields)
    {
        int num_domain = m_topo.num_domain(domain_type);

        for (const auto &ff : fields) {
            const auto &field_name = ff.first;
            const auto &field_desc = ff.second;
            auto write_value = field_desc.write_value;
            auto begin_bit = field_desc.begin_bit;
            auto end_bit = field_desc.end_bit;
            auto &description = field_desc.description;
            auto behavior = field_desc.behavior;
            auto units = field_desc.units;
            double multiplier = field_desc.multiplier;
            auto &agg_function = field_desc.agg_function;

            char hex[32];
            snprintf(hex, 32, "0x%05" PRIx32, register_offset);
            std::string raw_signal_name = raw_name + "_" + hex + "#";

            // add raw signal for every domain index
            auto it = m_signal_available.find(raw_signal_name);
            if (it == m_signal_available.end()) {
                std::vector<std::shared_ptr<Signal> > signals;
                for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                    auto cpus = m_topo.domain_nested(GEOPM_DOMAIN_CPU, domain_type, domain_idx);
                    int cpu_idx = *(cpus.begin());
                    uint32_t augmented_offset = domain_type == GEOPM_DOMAIN_CORE
                        ? register_offset + m_sstio->get_punit_from_cpu(domain_idx) * 4
                        : register_offset;
                    auto raw_sst = std::make_shared<SSTSignal>(
                        m_sstio, SSTSignal::M_MMIO, cpu_idx, 0x00, 0x00, augmented_offset, write_value);

                    signals.push_back(std::move(raw_sst));
                }
                m_signal_available[raw_signal_name] = {
                    .signals = std::move(signals),
                    .domain = domain_type,
                    .units = units,
                    .agg_function = agg_function,
                    .description = description,
                    .behavior = behavior
                };
            }

            std::string field_signal_name = raw_name + ":" + field_name;
            auto raw_sst_list = m_signal_available.at(raw_signal_name).signals;
            std::vector<std::shared_ptr<Signal> > signals;
            for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                // This isn't necessarily an MSR, but these signals have the
                // same need for read masks and scaling as MSR's need, so
                // we are able to use the same helper class here.
                auto field_signal = std::make_shared<MSRFieldSignal>(
                    raw_sst_list[domain_idx], begin_bit, end_bit,
                    MSR::M_FUNCTION_SCALE, multiplier);
                signals.push_back(field_signal);
            }
            m_signal_available[field_signal_name] = {
                .signals = std::move(signals),
                .domain = domain_type,
                .units = units,
                .agg_function = agg_function,
                .description = description,
                .behavior = behavior
            };
        }
    }

    void SSTIOGroup::add_mmio_controls(
        const std::string &raw_name, int domain_type, uint32_t register_offset,
        const std::map<std::string, sst_control_mmio_field_s> &fields, uint32_t read_mask)
    {
        int num_domain = m_topo.num_domain(domain_type);

        for (const auto &ff : fields) {
            const auto &field_name = ff.first;
            const auto &field_desc = ff.second;
            auto begin_bit = field_desc.begin_bit;
            auto end_bit = field_desc.end_bit;
            auto &description = field_desc.description;
            auto units = field_desc.units;
            double multiplier = field_desc.multiplier;
            auto &agg_function = field_desc.agg_function;

            // add raw control for every domain index
            std::string raw_control_name = raw_name + ":" + field_name;
            auto it = m_control_available.find(raw_control_name);
            if (it == m_control_available.end()) {
                std::vector<std::shared_ptr<Control> > controls;
                for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                    auto cpus = m_topo.domain_nested(GEOPM_DOMAIN_CPU, domain_type, domain_idx);
                    int cpu_idx = *(cpus.begin());
                    uint32_t augmented_offset = domain_type == GEOPM_DOMAIN_CORE
                        ? register_offset + m_sstio->get_punit_from_cpu(domain_idx) * 4
                        : register_offset;
                    auto control = std::make_shared<SSTControl>(
                        m_sstio, SSTControl::M_MMIO, cpu_idx, 0x00, 0x00, augmented_offset,
                        0x00 /* Write value. adjust later */, begin_bit,
                        end_bit, multiplier, 0x00, 0x00, read_mask);

                    controls.push_back(control);
                }
                m_control_available[raw_control_name] = {
                    .controls = std::move(controls),
                    .domain = domain_type,
                    .units = units,
                    .agg_function = agg_function,
                    .description = description
                };
            }
        }
    }
}
