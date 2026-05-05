// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_key.h"
#include "policy/route_policy.h"
#include "stages/cache_key_prepare_stage.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::stages {

class CacheKeyPrepareStageTest : public ::testing::Test {
protected:
    apg::ApgTransformContext context{};
    policy::RoutePolicy policy{};

    void SetUp() override {
        context.matched_policy = &policy;
        policy.route_id = "test-route";
        policy.cache.behavior = policy::CacheBehavior::Store;
        context.request_method = policy::HttpMethod::Get;
        std::strncpy(context.raw_path, "/api/test", sizeof(context.raw_path) - 1);
    }
};

TEST_F(CacheKeyPrepareStageTest, PrepareStage_CacheEligible) {
    auto result = cache_key_prepare_stage(context);
    EXPECT_EQ(result.result, apg::StageResult::Continue);
    EXPECT_TRUE(context.cache_eligible);
    EXPECT_TRUE(context.cache_key_ready);
    EXPECT_NE(context.cache_key[0], '\0');
}

TEST_F(CacheKeyPrepareStageTest, PrepareStage_NoPolicySkips) {
    context.matched_policy = nullptr;
    auto result = cache_key_prepare_stage(context);
    EXPECT_EQ(result.result, apg::StageResult::Continue);
    EXPECT_FALSE(context.cache_eligible);
    EXPECT_FALSE(context.cache_key_ready);
}

TEST_F(CacheKeyPrepareStageTest, PrepareStage_CacheDisabledSkips) {
    policy.cache.behavior = policy::CacheBehavior::Bypass;
    auto result = cache_key_prepare_stage(context);
    EXPECT_FALSE(context.cache_eligible);
    EXPECT_FALSE(context.cache_key_ready);
}

TEST_F(CacheKeyPrepareStageTest, PrepareStage_NonGetSkips) {
    context.request_method = policy::HttpMethod::Post;
    auto result = cache_key_prepare_stage(context);
    EXPECT_FALSE(context.cache_eligible);
    EXPECT_FALSE(context.cache_key_ready);
}

TEST_F(CacheKeyPrepareStageTest, PrepareStage_KeyIsDeterministic) {
    cache_key_prepare_stage(context);
    char first_key[cache::kCacheKeyMaxLen];
    std::memcpy(first_key, context.cache_key, cache::kCacheKeyMaxLen);

    // Run again
    cache_key_prepare_stage(context);
    EXPECT_STREQ(first_key, context.cache_key);
}

TEST_F(CacheKeyPrepareStageTest, PrepareStage_SharedKeyProof) {
    cache_key_prepare_stage(context);

    // Build key directly using the same logic
    cache::CacheKeyInput ki{};
    ki.method = context.request_method;
    ki.route_id = context.matched_policy->route_id;
    ki.path = context.raw_path;
    ki.query = context.raw_query;
    ki.selected_fields = context.selected_fields;
    ki.selected_field_count = context.selected_field_count;
    ki.policy_version = context.matched_policy->route_id;

    char expected_key[cache::kCacheKeyMaxLen] = {};
    cache::build_cache_key(ki, expected_key, sizeof(expected_key));

    EXPECT_STREQ(expected_key, context.cache_key);
}

TEST_F(CacheKeyPrepareStageTest, PrepareStage_AlreadyReadyIsIdempotent) {
    cache_key_prepare_stage(context);
    EXPECT_TRUE(context.cache_key_ready);

    auto result = cache_key_prepare_stage(context);
    EXPECT_EQ(result.result, apg::StageResult::Continue);
    EXPECT_TRUE(context.cache_key_ready);
}

} // namespace bytetaper::stages
