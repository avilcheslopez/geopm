/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "FrequencyGovernorImp.hpp"
#include "geopm/Helper.hpp"
#include "geopm_test.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"

using geopm::FrequencyGovernor;
using geopm::FrequencyGovernorImp;
using testing::NiceMock;
using testing::_;
using testing::Return;
using testing::Throw;
using testing::Invoke;

class FrequencyGovernorTest : public ::testing::Test
{
    protected:
        void SetUp(void);
        NiceMock<MockPlatformIO> m_platio;
        NiceMock<MockPlatformTopo> m_topo;

        std::unique_ptr<FrequencyGovernor> m_gov;

        const int M_CTL_DOMAIN = GEOPM_DOMAIN_CORE;
        const int M_NUM_CORE = 4;
        const double M_PLAT_MAX_FREQ = 3.7e9;
        const double M_PLAT_STICKER_FREQ = 2.0e9;
        const double M_PLAT_MIN_FREQ = 1.0e9;
        const double M_PLAT_STEP_FREQ = 1e8;
        const std::vector<int> M_FREQ_CTL_IDX = {42, 43, 44, 45};
};

void FrequencyGovernorTest::SetUp(void)
{
    ON_CALL(m_platio, control_domain_type("CPU_FREQUENCY_MAX_CONTROL")).WillByDefault(Return(M_CTL_DOMAIN));
    ON_CALL(m_topo, num_domain(GEOPM_DOMAIN_BOARD)).WillByDefault(Return(1));
    ON_CALL(m_topo, num_domain(M_CTL_DOMAIN)).WillByDefault(Return(M_NUM_CORE));
    ON_CALL(m_topo, num_domain(GEOPM_DOMAIN_CPU)).WillByDefault(Return(2*M_NUM_CORE));
    ON_CALL(m_platio, read_signal("CPUINFO::FREQ_STEP", _, _)).WillByDefault(Return(M_PLAT_STEP_FREQ));
    ON_CALL(m_platio, read_signal("CPU_FREQUENCY_MIN_AVAIL", _, _)).WillByDefault(Return(M_PLAT_MIN_FREQ));
    ON_CALL(m_platio, read_signal("CPUINFO::FREQ_STICKER", _, _)).WillByDefault(Return(M_PLAT_STICKER_FREQ));
    ON_CALL(m_platio, read_signal("CPU_FREQUENCY_MAX_AVAIL", _, _)).WillByDefault(Return(M_PLAT_MAX_FREQ));

    ASSERT_EQ(M_NUM_CORE, (int)M_FREQ_CTL_IDX.size());
    for (int idx = 0; idx < M_NUM_CORE; ++idx) {
        ON_CALL(m_platio, push_control("CPU_FREQUENCY_MAX_CONTROL", M_CTL_DOMAIN, idx)).
            WillByDefault(Return(M_FREQ_CTL_IDX[idx]));
    }
    ON_CALL(m_platio, push_control("CPU_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_CPU, _))
        .WillByDefault(Throw(geopm::Exception("invalid domain for frequency control",
                                              GEOPM_ERROR_INVALID, __FILE__, __LINE__)));

    m_gov = geopm::make_unique<FrequencyGovernorImp>(m_platio, m_topo);
    m_gov->init_platform_io();
}

TEST_F(FrequencyGovernorTest, frequency_control_domain_default)
{
    auto gov = geopm::make_unique<FrequencyGovernorImp>(m_platio, m_topo);
    gov->init_platform_io();
    int domain = gov->frequency_domain_type();
    EXPECT_EQ(M_CTL_DOMAIN, domain);
}

TEST_F(FrequencyGovernorTest, adjust_platform)
{
    std::vector<double> request;
    int domain = m_gov->frequency_domain_type();
    EXPECT_EQ(M_CTL_DOMAIN, domain);
    // todo: should caller use platform topo, or get this through governor?
    int num_domain = m_topo.num_domain(domain);
    EXPECT_EQ(M_NUM_CORE, num_domain);
    request = {1.1e9, 1.2e9, 1.5e9, 1.7e9};
    ASSERT_EQ(num_domain, (int)request.size());
    // check that controls are actually applied
    for (int idx = 0; idx < num_domain; ++idx) {
        EXPECT_CALL(m_platio, adjust(M_FREQ_CTL_IDX[idx], request[idx]));
    }
    m_gov->adjust_platform(request);
    bool result = m_gov->do_write_batch();
    EXPECT_TRUE(result);
    EXPECT_EQ(m_gov->get_clamp_count(), 0);
}

TEST_F(FrequencyGovernorTest, adjust_platform_clamping)
{
    std::vector<double> request;
    int domain = m_gov->frequency_domain_type();
    EXPECT_EQ(M_CTL_DOMAIN, domain);
    int num_domain = m_topo.num_domain(domain);
    EXPECT_EQ(M_NUM_CORE, num_domain);
    request = {4.1e9, 1.2e9, 1.5e9, 0.7e9};
    ASSERT_EQ(num_domain, (int)request.size());
    std::vector<double> expected = {M_PLAT_MAX_FREQ, 1.2e9, 1.5e9, M_PLAT_MIN_FREQ};
    // check that controls are actually applied
    for (int idx = 0; idx < num_domain; ++idx) {
        EXPECT_CALL(m_platio, adjust(M_FREQ_CTL_IDX[idx], expected[idx]));
    }
    m_gov->adjust_platform(request);
    bool result = m_gov->do_write_batch();
    EXPECT_TRUE(result);
    EXPECT_EQ(m_gov->get_clamp_count(), 2);
}

TEST_F(FrequencyGovernorTest, adjust_platform_error)
{
    std::vector<double> request;
    GEOPM_EXPECT_THROW_MESSAGE(m_gov->adjust_platform(request),
                               GEOPM_ERROR_INVALID,
                               "size of request vector");
}

TEST_F(FrequencyGovernorTest, frequency_bounds_in_range)
{
    // default settings
    EXPECT_DOUBLE_EQ(M_PLAT_MIN_FREQ, m_gov->get_frequency_min());
    EXPECT_DOUBLE_EQ(M_PLAT_MAX_FREQ, m_gov->get_frequency_max());
    EXPECT_DOUBLE_EQ(M_PLAT_STEP_FREQ, m_gov->get_frequency_step());

    // change settings
    double new_min = M_PLAT_MIN_FREQ + M_PLAT_STEP_FREQ;
    double new_max = M_PLAT_MAX_FREQ - M_PLAT_STEP_FREQ;
    bool result = m_gov->set_frequency_bounds(new_min, new_max);
    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(new_min, m_gov->get_frequency_min());
    EXPECT_DOUBLE_EQ(new_max, m_gov->get_frequency_max());

    // same settings
    result = m_gov->set_frequency_bounds(new_min, new_max);
    EXPECT_FALSE(result);
    EXPECT_DOUBLE_EQ(new_min, m_gov->get_frequency_min());
    EXPECT_DOUBLE_EQ(new_max, m_gov->get_frequency_max());

    EXPECT_EQ(m_gov->get_clamp_count(), 0);
}

TEST_F(FrequencyGovernorTest, frequency_bounds_invalid)
{
    GEOPM_EXPECT_THROW_MESSAGE(m_gov->set_frequency_bounds(M_PLAT_MIN_FREQ - 1, M_PLAT_MAX_FREQ),
                               GEOPM_ERROR_INVALID,
                               "invalid frequency bounds");
    GEOPM_EXPECT_THROW_MESSAGE(m_gov->set_frequency_bounds(M_PLAT_MIN_FREQ, M_PLAT_MAX_FREQ + 1),
                               GEOPM_ERROR_INVALID,
                               "invalid frequency bounds");
    GEOPM_EXPECT_THROW_MESSAGE(m_gov->set_frequency_bounds(M_PLAT_MAX_FREQ, M_PLAT_MIN_FREQ),
                               GEOPM_ERROR_INVALID,
                               "invalid frequency bounds");
}

TEST_F(FrequencyGovernorTest, validate_policy)
{
    double min = NAN;
    double max = NAN;
    m_gov->validate_policy(min, max);
    EXPECT_FALSE(std::isnan(min));
    EXPECT_FALSE(std::isnan(max));

    min = M_PLAT_MIN_FREQ + 1;
    max = M_PLAT_MAX_FREQ - 1;
    m_gov->validate_policy(min, max);
    EXPECT_DOUBLE_EQ(M_PLAT_MIN_FREQ + 1, min);
    EXPECT_DOUBLE_EQ(M_PLAT_MAX_FREQ - 1, max);

    min = M_PLAT_MIN_FREQ - 1;
    max = M_PLAT_MAX_FREQ + 1;
    m_gov->validate_policy(min, max);
    // clamp to min and max
    EXPECT_DOUBLE_EQ(M_PLAT_MIN_FREQ, min);
    EXPECT_DOUBLE_EQ(M_PLAT_MAX_FREQ, max);
}

TEST_F(FrequencyGovernorTest, set_domain_type)
{
    m_gov = geopm::make_unique<FrequencyGovernorImp>(m_platio, m_topo);
    EXPECT_CALL(m_topo, is_nested_domain(_, _)).WillOnce(Return(false));
    GEOPM_EXPECT_THROW_MESSAGE(
            m_gov->set_domain_type(GEOPM_DOMAIN_CPU),
            GEOPM_ERROR_INVALID,
            "Attempted to set a frequency control domain that does not contain "
            "the native domain for CPU_FREQUENCY_MAX_CONTROL.");

    m_gov->init_platform_io();
    GEOPM_EXPECT_THROW_MESSAGE(
            m_gov->set_domain_type(GEOPM_DOMAIN_CPU),
            GEOPM_ERROR_INVALID,
            "Attempted to set a new frequency control domain after calling "
            "FrequencyGovernorImp::init_platform_io");

    // Set to native granularity (no effect, but no error)
    m_gov = geopm::make_unique<FrequencyGovernorImp>(m_platio, m_topo);
    EXPECT_CALL(m_topo, is_nested_domain(_, _)).WillOnce(Return(true));
    EXPECT_CALL(m_platio, push_control("CPU_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_CORE, _))
        .Times(M_NUM_CORE)
        .WillRepeatedly(Invoke([](const std::string&, int, int idx){ return idx; }));
    m_gov->set_domain_type(GEOPM_DOMAIN_CORE);
    m_gov->init_platform_io();

    // Set to more coarse granularity. Should use that, and not the native domain.
    m_gov = geopm::make_unique<FrequencyGovernorImp>(m_platio, m_topo);
    EXPECT_CALL(m_topo, is_nested_domain(_, _)).WillOnce(Return(true));
    EXPECT_CALL(m_platio, push_control("CPU_FREQUENCY_MAX_CONTROL", M_CTL_DOMAIN, _))
        .Times(0);
    EXPECT_CALL(m_platio, push_control("CPU_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_BOARD, _))
        .WillOnce(Return(0));
    m_gov->set_domain_type(GEOPM_DOMAIN_BOARD);
    m_gov->init_platform_io();
}
