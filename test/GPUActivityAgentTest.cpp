/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <stdlib.h>
#include <map>
#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_agent.h"
#include "geopm_hash.h"

#include "Agent.hpp"
#include "GPUActivityAgent.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Agg.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm_prof.h"
#include "geopm_test.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Sequence;
using ::testing::Return;
using ::testing::AtLeast;
using geopm::GPUActivityAgent;
using geopm::PlatformTopo;

class GPUActivityAgentTest : public :: testing :: Test
{
    protected:
        enum mock_pio_idx_e {
            GPU_CORE_ACTIVITY_IDX,
            GPU_UTILIZATION_IDX,
            GPU_ENERGY_IDX,
            GPU_FREQUENCY_CONTROL_MIN_IDX,
            GPU_FREQUENCY_CONTROL_MAX_IDX,
            TIME_IDX
        };
        enum policy_idx_e {
            FREQ_MAX = 0,
            FREQ_EFFICIENT = 1,
            PHI = 2,
        };

        void SetUp();
        void TearDown();
        void set_up_val_policy_expectations();

        void test_adjust_platform(std::vector<double> &policy,
                                  double mock_active,
                                  double mock_util,
                                  double expected_freq);
        static const int M_NUM_CPU = 1;
        static const int M_NUM_BOARD = 1;
        static const int M_NUM_GPU = 1;
        const double M_FREQ_MIN = 0135000000.0;
        const double M_FREQ_MAX = 1530000000.0;
        std::vector<double> M_DEFAULT_POLICY;
        size_t m_num_policy;
        std::unique_ptr<GPUActivityAgent> m_agent;
        std::unique_ptr<MockPlatformIO> m_platform_io;
        std::unique_ptr<MockPlatformTopo> m_platform_topo;
};


void GPUActivityAgentTest::SetUp()
{
        static constexpr const double M_FREQ_MIN = 0135000000.0;
        static constexpr const double M_FREQ_MAX = 1530000000.0;
        static const std::vector<double> M_DEFAULT_POLICY;
    GPUActivityAgentTest::M_DEFAULT_POLICY = {M_FREQ_MAX, M_FREQ_MIN, NAN};

    m_platform_io = geopm::make_unique<MockPlatformIO>();
    m_platform_topo = geopm::make_unique<MockPlatformTopo>();

    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD))
        .WillByDefault(Return(M_NUM_BOARD));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_GPU))
        .WillByDefault(Return(M_NUM_GPU));

    ON_CALL(*m_platform_io, push_signal("GPU_CORE_ACTIVITY", _, _))
        .WillByDefault(Return(GPU_CORE_ACTIVITY_IDX));
    ON_CALL(*m_platform_io, push_signal("GPU_UTILIZATION", _, _))
        .WillByDefault(Return(GPU_UTILIZATION_IDX));
    ON_CALL(*m_platform_io, push_signal("GPU_ENERGY", _, _))
        .WillByDefault(Return(GPU_ENERGY_IDX));
    ON_CALL(*m_platform_io, push_signal("TIME", _, _))
        .WillByDefault(Return(TIME_IDX));

    ON_CALL(*m_platform_io, push_control("GPU_CORE_FREQUENCY_MIN_CONTROL", _, _))
        .WillByDefault(Return(GPU_FREQUENCY_CONTROL_MIN_IDX));
    ON_CALL(*m_platform_io, push_control("GPU_CORE_FREQUENCY_MAX_CONTROL", _, _))
        .WillByDefault(Return(GPU_FREQUENCY_CONTROL_MAX_IDX));
    ON_CALL(*m_platform_io, agg_function(_))
        .WillByDefault(Return(geopm::Agg::average));

    ON_CALL(*m_platform_io, control_domain_type("GPU_CORE_FREQUENCY_MIN_CONTROL"))
        .WillByDefault(Return(GEOPM_DOMAIN_GPU));
    ON_CALL(*m_platform_io, control_domain_type("GPU_CORE_FREQUENCY_MAX_CONTROL"))
        .WillByDefault(Return(GEOPM_DOMAIN_GPU));
    ON_CALL(*m_platform_io, signal_domain_type("GPU_CORE_ACTIVITY"))
        .WillByDefault(Return(GEOPM_DOMAIN_GPU));
    ON_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(M_FREQ_MIN));
    ON_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(M_FREQ_MAX));

    ASSERT_LT(M_FREQ_MIN, 0.2e9);
    ASSERT_LT(1.4e9, M_FREQ_MAX);

    m_agent = geopm::make_unique<GPUActivityAgent>(*m_platform_io, *m_platform_topo);
    m_num_policy = m_agent->policy_names().size();

    // leaf agent
    m_agent->init(0, {}, false);
}

void GPUActivityAgentTest::TearDown()
{
}

void GPUActivityAgentTest::set_up_val_policy_expectations()
{
    EXPECT_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MIN_AVAIL", _, _)).WillRepeatedly(
                Return(M_FREQ_MIN));
    EXPECT_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MAX_AVAIL", _, _)).WillRepeatedly(
                Return(M_FREQ_MAX));
    EXPECT_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD)).WillRepeatedly(Return(M_NUM_BOARD));
}

TEST_F(GPUActivityAgentTest, name)
{
    EXPECT_EQ("gpu_activity", m_agent->plugin_name());
    EXPECT_NE("bad_string", m_agent->plugin_name());
}

TEST_F(GPUActivityAgentTest, validate_policy)
{
    set_up_val_policy_expectations();

    const std::vector<double> empty(m_num_policy, NAN);

    // default policy is accepted
    // load default policy
    std::vector<double> policy = M_DEFAULT_POLICY;
    EXPECT_NO_THROW(m_agent->validate_policy(policy));
    // validate policy is unmodified except Phi
    ASSERT_EQ(m_num_policy, policy.size());
    EXPECT_EQ(M_FREQ_MAX, policy[FREQ_MAX]);
    EXPECT_EQ(M_FREQ_MIN, policy[FREQ_EFFICIENT]);
    // Default value when NAN is passed is 0.5
    EXPECT_EQ(0.5, policy[PHI]);

    // all-NAN policy is accepted
    // setup & load NAN policy
    policy = empty;
    EXPECT_NO_THROW(m_agent->validate_policy(policy));
    // validate policy defaults are applied
    ASSERT_EQ(m_num_policy, policy.size());
    EXPECT_EQ(M_FREQ_MAX, policy[FREQ_MAX]);
    EXPECT_EQ((policy[FREQ_MAX] + M_FREQ_MIN) / 2, policy[FREQ_EFFICIENT]);
    EXPECT_EQ(0.5, policy[PHI]);

    // non-default policy is accepted
    // setup & load policy
    policy[FREQ_MAX] = M_FREQ_MAX;
    policy[FREQ_EFFICIENT] = M_FREQ_MAX / 2;
    policy[PHI] = 0.1;
    EXPECT_NO_THROW(m_agent->validate_policy(policy));

    // validate policy is modified as expected
    // as phi --> 0 FREQ_EFFICIENT --> FREQ_MAX
    ASSERT_EQ(m_num_policy, policy.size());
    EXPECT_EQ(M_FREQ_MAX, policy[FREQ_MAX]);
    EXPECT_GE(policy[FREQ_EFFICIENT], M_FREQ_MAX / 2);
    EXPECT_LE(policy[FREQ_EFFICIENT], M_FREQ_MAX);
    EXPECT_EQ(0.1, policy[PHI]);

    //Fe > Fmax --> Error
    policy[FREQ_MAX] = NAN;
    policy[FREQ_EFFICIENT] = M_FREQ_MAX + 1;
    policy[PHI] = NAN;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                              "GPU_FREQ_EFFICIENT out of range");

    //Fe < Fmin --> Error
    policy[FREQ_MAX] = NAN;
    policy[FREQ_EFFICIENT] = M_FREQ_MIN - 1;
    policy[PHI] = NAN;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "GPU_FREQ_EFFICIENT out of range");

    //Fe > Policy Fmax --> Error
    policy[FREQ_MAX] = M_FREQ_MAX - 2;
    policy[FREQ_EFFICIENT] = M_FREQ_MAX - 1;
    policy[PHI] = NAN;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "value exceeds GPU_FREQ_MAX");

    //Policy Fmax > Fmax --> Error
    policy[FREQ_MAX] = M_FREQ_MAX + 1;
    policy[FREQ_EFFICIENT] = NAN;
    policy[PHI] = NAN;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "GPU_FREQ_MAX out of range");

    //Policy Fmax < Fmin --> Error
    policy[FREQ_MAX] = M_FREQ_MIN - 1;
    policy[FREQ_EFFICIENT] = NAN;
    policy[PHI] = NAN;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "GPU_FREQ_MAX out of range");

    //Policy Phi < 0 --> Error
    policy[FREQ_MAX] = NAN;
    policy[FREQ_EFFICIENT] = NAN;
    policy[PHI] = -1;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "POLICY_GPU_PHI value out of range");

    //Policy Phi > 1.0 --> Error
    policy[FREQ_MAX] = NAN;
    policy[FREQ_EFFICIENT] = NAN;
    policy[PHI] = 1.1;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "POLICY_GPU_PHI value out of range");
}

void GPUActivityAgentTest::test_adjust_platform(std::vector<double> &policy,
                                                double mock_active,
                                                double mock_util,
                                                double expected_freq)
{
    set_up_val_policy_expectations();

    EXPECT_NO_THROW(m_agent->validate_policy(policy));

    //Sample
    std::vector<double> tmp;
    EXPECT_CALL(*m_platform_io, sample(GPU_CORE_ACTIVITY_IDX))
                .WillRepeatedly(Return(mock_active));
    EXPECT_CALL(*m_platform_io, sample(GPU_UTILIZATION_IDX))
                .WillRepeatedly(Return(mock_util));
    EXPECT_CALL(*m_platform_io, sample(GPU_ENERGY_IDX))
                .WillRepeatedly(Return(123456789));
    EXPECT_CALL(*m_platform_io, sample(TIME_IDX))
                .Times(1);
    m_agent->sample_platform(tmp);

    //Adjust
    //Check frequency
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQUENCY_CONTROL_MIN_IDX, expected_freq)).Times(M_NUM_GPU);
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQUENCY_CONTROL_MAX_IDX, expected_freq)).Times(M_NUM_GPU);

    m_agent->adjust_platform(policy);

    //Check a frequency decision resulted in write batch being true
    EXPECT_TRUE(m_agent->do_write_batch());
}

TEST_F(GPUActivityAgentTest, adjust_platform_high)
{
    std::vector<double> policy = M_DEFAULT_POLICY;
    double mock_active = 1.0;
    double mock_util = 1.0;
    test_adjust_platform(policy, mock_active, mock_util, M_FREQ_MAX);
}

TEST_F(GPUActivityAgentTest, adjust_platform_medium)
{
    std::vector<double> policy = M_DEFAULT_POLICY;
    double mock_active = 0.5;
    double mock_util = 1.0;
    double expected_freq = policy[FREQ_EFFICIENT] +
            (M_FREQ_MAX - policy[FREQ_EFFICIENT]) * mock_active;
    test_adjust_platform(policy, mock_active, mock_util, expected_freq);
}

TEST_F(GPUActivityAgentTest, adjust_platform_low)
{
    std::vector<double> policy = M_DEFAULT_POLICY;
    double mock_active = 0.1;
    double mock_util = 1.0;
    double expected_freq = policy[FREQ_EFFICIENT] +
            (M_FREQ_MAX - policy[FREQ_EFFICIENT]) * mock_active;
    test_adjust_platform(policy, mock_active, mock_util, expected_freq);
}

TEST_F(GPUActivityAgentTest, adjust_platform_zero)
{
    std::vector<double> policy = M_DEFAULT_POLICY;
    double mock_active = 0.0;
    double mock_util = 1.0;
    test_adjust_platform(policy, mock_active, mock_util, M_FREQ_MIN);
}

TEST_F(GPUActivityAgentTest, adjust_platform_signal_out_of_bounds_high)
{
    std::vector<double> policy = M_DEFAULT_POLICY;
    double mock_active = 987654321;
    double mock_util = 1.0;
    test_adjust_platform(policy, mock_active, mock_util, M_FREQ_MAX);
}

TEST_F(GPUActivityAgentTest, adjust_platform_signal_out_of_bounds_low)
{
    std::vector<double> policy = M_DEFAULT_POLICY;
    double mock_active = -12345;
    double mock_util = 1.0;
    test_adjust_platform(policy, mock_active, mock_util, M_FREQ_MIN);

}
