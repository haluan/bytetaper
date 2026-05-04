// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "coalescing/coalescing_key.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::coalescing {

TEST(CoalescingKeyTest, SuccessIdenticalRequests) {
    char buf1[256];
    char buf2[256];

    CoalescingKeyInput input;
    input.method = policy::HttpMethod::Get;
    input.route_name = "api-route";
    input.normalized_path = "/v1/users";
    input.normalized_query = "id=123";
    input.selected_fields = "name,email";
    input.policy_version = 1;

    EXPECT_TRUE(build_coalescing_key(input, buf1, sizeof(buf1)));
    EXPECT_TRUE(build_coalescing_key(input, buf2, sizeof(buf2)));
    EXPECT_STREQ(buf1, buf2);
    EXPECT_STREQ(buf1, "c_key:api-route:1:/v1/users?id=123&fields=name,email");
}

TEST(CoalescingKeyTest, DifferentQueryCreatesDifferentKey) {
    char buf1[256];
    char buf2[256];

    CoalescingKeyInput input1;
    input1.method = policy::HttpMethod::Get;
    input1.route_name = "api";
    input1.normalized_path = "/v1";
    input1.normalized_query = "a=1";

    CoalescingKeyInput input2 = input1;
    input2.normalized_query = "a=2";

    EXPECT_TRUE(build_coalescing_key(input1, buf1, sizeof(buf1)));
    EXPECT_TRUE(build_coalescing_key(input2, buf2, sizeof(buf2)));
    EXPECT_STRNE(buf1, buf2);
}

TEST(CoalescingKeyTest, DifferentFieldsCreatesDifferentKey) {
    char buf1[256];
    char buf2[256];

    CoalescingKeyInput input1;
    input1.method = policy::HttpMethod::Get;
    input1.route_name = "api";
    input1.normalized_path = "/v1";
    input1.selected_fields = "f1";

    CoalescingKeyInput input2 = input1;
    input2.selected_fields = "f2";

    EXPECT_TRUE(build_coalescing_key(input1, buf1, sizeof(buf1)));
    EXPECT_TRUE(build_coalescing_key(input2, buf2, sizeof(buf2)));
    EXPECT_STRNE(buf1, buf2);
}

TEST(CoalescingKeyTest, NonGetNotEligible) {
    char buf[256];
    CoalescingKeyInput input;
    input.method = policy::HttpMethod::Post;
    input.route_name = "api";
    input.normalized_path = "/v1";

    EXPECT_FALSE(build_coalescing_key(input, buf, sizeof(buf)));
}

TEST(CoalescingKeyTest, RequiredFieldsNull) {
    char buf[256];
    CoalescingKeyInput input;
    input.method = policy::HttpMethod::Get;

    // Missing route_name
    input.route_name = nullptr;
    input.normalized_path = "/v1";
    EXPECT_FALSE(build_coalescing_key(input, buf, sizeof(buf)));

    // Missing path
    input.route_name = "api";
    input.normalized_path = nullptr;
    EXPECT_FALSE(build_coalescing_key(input, buf, sizeof(buf)));
}

TEST(CoalescingKeyTest, BufferOverflow) {
    char small_buf[10];
    CoalescingKeyInput input;
    input.method = policy::HttpMethod::Get;
    input.route_name = "very-long-route-name-that-definitely-overflows";
    input.normalized_path = "/v1";

    EXPECT_FALSE(build_coalescing_key(input, small_buf, sizeof(small_buf)));
}

TEST(CoalescingKeyTest, OptionalFieldsEmpty) {
    char buf[256];
    CoalescingKeyInput input;
    input.method = policy::HttpMethod::Get;
    input.route_name = "api";
    input.normalized_path = "/v1";
    input.normalized_query = nullptr;
    input.selected_fields = nullptr;
    input.policy_version = 0;

    EXPECT_TRUE(build_coalescing_key(input, buf, sizeof(buf)));
    EXPECT_STREQ(buf, "c_key:api:0:/v1?&fields=");
}

} // namespace bytetaper::coalescing
