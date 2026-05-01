// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/method_matcher.h"
#include "policy/route_policy.h"

#include <gtest/gtest.h>

namespace bytetaper::policy {

TEST(MethodMatcherTest, AnyMatchesGet) {
    EXPECT_TRUE(match_method(HttpMethod::Any, "GET"));
}

TEST(MethodMatcherTest, AnyMatchesPost) {
    EXPECT_TRUE(match_method(HttpMethod::Any, "POST"));
}

TEST(MethodMatcherTest, AnyMatchesNullMethod) {
    // Actually, match_method should handle null guards first.
    EXPECT_FALSE(match_method(HttpMethod::Any, nullptr));
}

TEST(MethodMatcherTest, GetMatchesGet) {
    EXPECT_TRUE(match_method(HttpMethod::Get, "GET"));
}

TEST(MethodMatcherTest, GetDoesNotMatchPost) {
    EXPECT_FALSE(match_method(HttpMethod::Get, "POST"));
}

TEST(MethodMatcherTest, PostMatchesPost) {
    EXPECT_TRUE(match_method(HttpMethod::Post, "POST"));
}

TEST(MethodMatcherTest, PutMatchesPut) {
    EXPECT_TRUE(match_method(HttpMethod::Put, "PUT"));
}

TEST(MethodMatcherTest, DeleteMatchesDelete) {
    EXPECT_TRUE(match_method(HttpMethod::Delete, "DELETE"));
}

TEST(MethodMatcherTest, PatchMatchesPatch) {
    EXPECT_TRUE(match_method(HttpMethod::Patch, "PATCH"));
}

TEST(MethodMatcherTest, CaseInsensitiveGetLower) {
    EXPECT_TRUE(match_method(HttpMethod::Get, "get"));
}

TEST(MethodMatcherTest, CaseInsensitivePostMixed) {
    EXPECT_TRUE(match_method(HttpMethod::Post, "Post"));
}

TEST(MethodMatcherTest, NullMethodReturnsFalse) {
    EXPECT_FALSE(match_method(HttpMethod::Get, nullptr));
}

TEST(MethodMatcherTest, DefaultAllowedMethodIsAny) {
    RoutePolicy policy{};
    EXPECT_EQ(policy.allowed_method, HttpMethod::Any);
}

} // namespace bytetaper::policy
