// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_key.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::cache {

static CacheKeyInput make_basic_input() {
    CacheKeyInput input{};
    input.method = policy::HttpMethod::Get;
    input.route_id = "api-v1";
    input.path = "/api/items";
    input.query = nullptr;
    input.selected_fields = nullptr;
    input.selected_field_count = 0;
    input.policy_version = "v1";
    return input;
}

TEST(CacheKeyTest, SameInputSameKey) {
    char key1[512] = {};
    char key2[512] = {};
    CacheKeyInput input = make_basic_input();
    EXPECT_TRUE(build_cache_key(input, key1, sizeof(key1)));
    EXPECT_TRUE(build_cache_key(input, key2, sizeof(key2)));
    EXPECT_STREQ(key1, key2);
}

TEST(CacheKeyTest, DifferentFieldsDifferentKey) {
    char key1[512] = {};
    char key2[512] = {};

    char fields1[1][policy::kMaxFieldNameLen] = {};
    std::strncpy(fields1[0], "id", policy::kMaxFieldNameLen - 1);

    char fields2[2][policy::kMaxFieldNameLen] = {};
    std::strncpy(fields2[0], "id", policy::kMaxFieldNameLen - 1);
    std::strncpy(fields2[1], "name", policy::kMaxFieldNameLen - 1);

    CacheKeyInput input = make_basic_input();

    input.selected_fields = fields1;
    input.selected_field_count = 1;
    EXPECT_TRUE(build_cache_key(input, key1, sizeof(key1)));

    input.selected_fields = fields2;
    input.selected_field_count = 2;
    EXPECT_TRUE(build_cache_key(input, key2, sizeof(key2)));

    EXPECT_NE(std::strcmp(key1, key2), 0);
}

TEST(CacheKeyTest, DifferentPolicyVersionDifferentKey) {
    char key1[512] = {};
    char key2[512] = {};
    CacheKeyInput input = make_basic_input();

    input.policy_version = "v1";
    EXPECT_TRUE(build_cache_key(input, key1, sizeof(key1)));

    input.policy_version = "v2";
    EXPECT_TRUE(build_cache_key(input, key2, sizeof(key2)));

    EXPECT_NE(std::strcmp(key1, key2), 0);
}

TEST(CacheKeyTest, NonGetReturnsFalse) {
    char key[512] = {};
    CacheKeyInput input = make_basic_input();
    input.method = policy::HttpMethod::Post;
    EXPECT_FALSE(build_cache_key(input, key, sizeof(key)));
}

TEST(CacheKeyTest, FieldOrderIndependence) {
    char key1[512] = {};
    char key2[512] = {};

    char fields_ab[2][policy::kMaxFieldNameLen] = {};
    std::strncpy(fields_ab[0], "id", policy::kMaxFieldNameLen - 1);
    std::strncpy(fields_ab[1], "name", policy::kMaxFieldNameLen - 1);

    char fields_ba[2][policy::kMaxFieldNameLen] = {};
    std::strncpy(fields_ba[0], "name", policy::kMaxFieldNameLen - 1);
    std::strncpy(fields_ba[1], "id", policy::kMaxFieldNameLen - 1);

    CacheKeyInput input = make_basic_input();

    input.selected_fields = fields_ab;
    input.selected_field_count = 2;
    EXPECT_TRUE(build_cache_key(input, key1, sizeof(key1)));

    input.selected_fields = fields_ba;
    input.selected_field_count = 2;
    EXPECT_TRUE(build_cache_key(input, key2, sizeof(key2)));

    EXPECT_STREQ(key1, key2);
}

TEST(CacheKeyTest, EmptyFieldsAndQueryStillValid) {
    char key[512] = {};
    CacheKeyInput input = make_basic_input();
    input.query = nullptr;
    input.selected_fields = nullptr;
    input.selected_field_count = 0;
    EXPECT_TRUE(build_cache_key(input, key, sizeof(key)));
    EXPECT_NE(key[0], '\0');
}

} // namespace bytetaper::cache
