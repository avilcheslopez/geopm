#include "gtest/gtest.h"

#include "geopm_hint.h"
#include "geopm_test.hpp"

TEST(GEOPMHintTest, check_hint)
{
    uint64_t hint = GEOPM_SENTINEL_REGION_HINT;
    GEOPM_EXPECT_THROW_MESSAGE(check_hint(hint),
                               GEOPM_ERROR_INVALID,
                               "hint out of range");
    hint = 1ULL << 32;
    GEOPM_EXPECT_THROW_MESSAGE(check_hint(hint),
                               GEOPM_ERROR_INVALID,
                               "invalid hint");
}
