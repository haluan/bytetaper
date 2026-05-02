// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_key.h"
#include "cache/l1_cache.h"
#include "policy/route_policy.h"
#include "stages/l1_cache_store_stage.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::stages {

class L1CacheStoreStageTest : public ::testing::Test {
protected:
    void SetUp() override {
        l1_init(&l1_cache_);
    }

    cache::L1Cache l1_cache_{};
    policy::RoutePolicy policy_{};
};

TEST_F(L1CacheStoreStageTest, EligibleResponseStored) {
    policy_.route_id = "rt1";
    policy_.cache.behavior = policy::CacheBehavior::Store;
    policy_.cache.ttl_seconds = 60;

    apg::ApgTransformContext ctx{};
    ctx.matched_policy = &policy_;
    ctx.l1_cache = &l1_cache_;
    ctx.request_method = policy::HttpMethod::Get;
    ctx.request_epoch_ms = 1000;
    std::strncpy(ctx.raw_path, "/api/items", sizeof(ctx.raw_path) - 1);
    ctx.response_status_code = 200;
    const char body[] = "hello";
    ctx.response_body = body;
    ctx.response_body_len = 5;
    std::strncpy(ctx.response_content_type, "application/json", cache::kCacheContentTypeMaxLen - 1);

    auto out = l1_cache_store_stage(ctx);
    EXPECT_EQ(out.result, apg::StageResult::Continue);
    EXPECT_STREQ(out.note, "stored");

    // Verify retrievable
    char key_buf[cache::kCacheKeyMaxLen] = {};
    cache::CacheKeyInput ki{};
    ki.method = policy::HttpMethod::Get;
    ki.route_id = "rt1";
    ki.path = "/api/items";
    ki.policy_version = "rt1";
    ASSERT_TRUE(cache::build_cache_key(ki, key_buf, sizeof(key_buf)));

    cache::CacheEntry hit{};
    EXPECT_TRUE(cache::l1_get(&l1_cache_, key_buf, 1000, &hit));
    EXPECT_EQ(hit.status_code, 200);
    EXPECT_EQ(hit.body_len, 5u);
    EXPECT_EQ(hit.expires_at_epoch_ms, 1000 + 60 * 1000);
}

TEST_F(L1CacheStoreStageTest, NonGetNotStored) {
    policy_.route_id = "rt1";
    policy_.cache.behavior = policy::CacheBehavior::Store;
    policy_.cache.ttl_seconds = 60;

    apg::ApgTransformContext ctx{};
    ctx.matched_policy = &policy_;
    ctx.l1_cache = &l1_cache_;
    ctx.request_method = policy::HttpMethod::Post; // Non-GET
    ctx.response_status_code = 200;
    ctx.response_body = "body";
    ctx.response_body_len = 4;

    auto out = l1_cache_store_stage(ctx);
    EXPECT_EQ(out.result, apg::StageResult::Continue);
    EXPECT_STREQ(out.note, "non-get");

    // Verify NOT in cache
    char key_buf[cache::kCacheKeyMaxLen] = {};
    cache::CacheKeyInput ki{};
    ki.method = policy::HttpMethod::Post; // build_cache_key fails for non-GET anyway
    ki.route_id = "rt1";
    ki.path = "";
    ki.policy_version = "rt1";
    // We expect l1_get to fail because it wasn't stored.
    // However, build_cache_key for Post would fail, so we'll check manually
    // or just assume if it returns non-get, it didn't store.
    for (std::size_t i = 0; i < cache::kL1SlotCount; ++i) {
        EXPECT_EQ(l1_cache_.generations[i], 0u);
    }
}

TEST_F(L1CacheStoreStageTest, Non2xxNotStored) {
    policy_.route_id = "rt1";
    policy_.cache.behavior = policy::CacheBehavior::Store;
    policy_.cache.ttl_seconds = 60;

    apg::ApgTransformContext ctx{};
    ctx.matched_policy = &policy_;
    ctx.l1_cache = &l1_cache_;
    ctx.request_method = policy::HttpMethod::Get;
    ctx.response_status_code = 404; // Non-2xx
    ctx.response_body = "body";
    ctx.response_body_len = 4;

    auto out = l1_cache_store_stage(ctx);
    EXPECT_EQ(out.result, apg::StageResult::Continue);
    EXPECT_STREQ(out.note, "non-2xx");

    for (std::size_t i = 0; i < cache::kL1SlotCount; ++i) {
        EXPECT_EQ(l1_cache_.generations[i], 0u);
    }
}

TEST_F(L1CacheStoreStageTest, MissingKeyNotStored) {
    policy_.route_id = nullptr; // Fails key build
    policy_.cache.behavior = policy::CacheBehavior::Store;
    policy_.cache.ttl_seconds = 60;

    apg::ApgTransformContext ctx{};
    ctx.matched_policy = &policy_;
    ctx.l1_cache = &l1_cache_;
    ctx.request_method = policy::HttpMethod::Get;
    ctx.response_status_code = 200;
    ctx.response_body = "body";
    ctx.response_body_len = 4;

    auto out = l1_cache_store_stage(ctx);
    EXPECT_EQ(out.result, apg::StageResult::Continue);
    EXPECT_STREQ(out.note, "key-build-failed");

    for (std::size_t i = 0; i < cache::kL1SlotCount; ++i) {
        EXPECT_EQ(l1_cache_.generations[i], 0u);
    }
}

} // namespace bytetaper::stages
