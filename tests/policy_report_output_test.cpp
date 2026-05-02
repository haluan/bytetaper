// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/policy_reporter.h"

#include <gtest/gtest.h>

namespace bytetaper::policy {

TEST(PolicyReportOutputTest, RouteMatchKindStr) {
    EXPECT_STREQ(route_match_kind_str(RouteMatchKind::Prefix), "prefix");
    EXPECT_STREQ(route_match_kind_str(RouteMatchKind::Exact), "exact");
}

TEST(PolicyReportOutputTest, MutationModeStr) {
    EXPECT_STREQ(mutation_mode_str(MutationMode::Disabled), "disabled");
    EXPECT_STREQ(mutation_mode_str(MutationMode::HeadersOnly), "headers_only");
    EXPECT_STREQ(mutation_mode_str(MutationMode::Full), "full");
}

TEST(PolicyReportOutputTest, HttpMethodStr) {
    EXPECT_STREQ(http_method_str(HttpMethod::Any), "any");
    EXPECT_STREQ(http_method_str(HttpMethod::Get), "get");
    EXPECT_STREQ(http_method_str(HttpMethod::Post), "post");
    EXPECT_STREQ(http_method_str(HttpMethod::Put), "put");
    EXPECT_STREQ(http_method_str(HttpMethod::Delete), "delete");
    EXPECT_STREQ(http_method_str(HttpMethod::Patch), "patch");
}

TEST(PolicyReportOutputTest, CacheBehaviorStr) {
    EXPECT_STREQ(cache_behavior_str(CacheBehavior::Default), "default");
    EXPECT_STREQ(cache_behavior_str(CacheBehavior::Bypass), "bypass");
    EXPECT_STREQ(cache_behavior_str(CacheBehavior::Store), "store");
}

TEST(PolicyReportOutputTest, FieldFilterModeStr) {
    EXPECT_STREQ(field_filter_mode_str(FieldFilterMode::None), "none");
    EXPECT_STREQ(field_filter_mode_str(FieldFilterMode::Allowlist), "allowlist");
    EXPECT_STREQ(field_filter_mode_str(FieldFilterMode::Denylist), "denylist");
}

} // namespace bytetaper::policy
