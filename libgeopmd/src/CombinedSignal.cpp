/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "CombinedSignal.hpp"

#include <cmath>
#include <numeric>
#include <algorithm>

#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"

namespace geopm
{
    CombinedSignal::CombinedSignal()
        : CombinedSignal(Agg::sum)
    {

    }

    CombinedSignal::CombinedSignal(std::function<double(const std::vector<double> &)> func)
        : m_agg_function(std::move(func))
    {

    }

    double CombinedSignal::sample(const std::vector<double> &values)
    {
        return m_agg_function(values);
    }
}
