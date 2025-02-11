/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "LevelZeroSignal.hpp"

#include <cmath>
#include "geopm/Exception.hpp"

namespace geopm
{
    LevelZeroSignal::LevelZeroSignal(std::function<double (unsigned int)> devpool_func,
                                     unsigned int domain_idx, double scalar)
      : m_devpool_func(std::move(devpool_func))
        , m_domain_idx(domain_idx)
        , m_scalar(scalar)
        , m_is_batch_ready(false)
        , m_value(NAN)
        , m_is_sampled(false)
    {
    }

    void LevelZeroSignal::setup_batch(void)
    {
        if (!m_is_batch_ready) {
            m_is_batch_ready = true;
        }
    }

    void LevelZeroSignal::set_sample(double value)
    {
        m_value = value;
        m_is_sampled = false;
    }

    bool LevelZeroSignal::is_sampled(void) const
    {
        return m_is_sampled;
    }

    double LevelZeroSignal::sample(void)
    {
        if (!m_is_batch_ready) {
            throw Exception("setup_batch() must be called before sample().",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_is_sampled = true;
        return m_value;
    }

    double LevelZeroSignal::read(void) const
    {
        m_is_sampled = true;
        return m_devpool_func(m_domain_idx) * m_scalar;
    }
}
