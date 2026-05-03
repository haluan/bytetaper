// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "apg/context.h"
#include "policy/route_policy.h"
#include "stages/pagination_request_mutation_stage.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::stages {

static policy::RoutePolicy make_route(std::uint32_t def, std::uint32_t max) {
    policy::RoutePolicy rp{};
    rp.route_id = "test-route";
    rp.pagination.enabled = true;
    rp.pagination.mode = policy::PaginationMode::LimitOffset;
    rp.pagination.default_limit = def;
    rp.pagination.max_limit = max;
    rp.pagination.upstream_supports_pagination = true;
    std::strncpy(rp.pagination.limit_param, "limit", 31);
    std::strncpy(rp.pagination.offset_param, "offset", 31);
    return rp;
}

static apg::ApgTransformContext make_ctx(const policy::RoutePolicy* rp, const char* path,
                                         const char* query) {
    apg::ApgTransformContext ctx{};
    ctx.matched_policy = rp;
    std::strncpy(ctx.raw_path, path, sizeof(ctx.raw_path) - 1);
    ctx.raw_path_length = std::strlen(path);
    if (query) {
        std::strncpy(ctx.raw_query, query, sizeof(ctx.raw_query) - 1);
        ctx.raw_query_length = std::strlen(query);
    }
    return ctx;
}

TEST(PaginationRequestMutationStageTest, MissingLimit_MutationApplied) {
    auto rp = make_route(50, 500);
    auto ctx = make_ctx(&rp, "/orders", "");
    auto out = pagination_request_mutation_stage(ctx);
    EXPECT_EQ(out.result, apg::StageResult::Continue);
    EXPECT_TRUE(ctx.request_mutation.applied);
    EXPECT_STREQ(ctx.request_mutation.path, "/orders?limit=50");
}

TEST(PaginationRequestMutationStageTest, ExcessiveLimit_Capped) {
    auto rp = make_route(20, 500);
    auto ctx = make_ctx(&rp, "/orders", "limit=9999");
    auto out = pagination_request_mutation_stage(ctx);
    EXPECT_TRUE(ctx.request_mutation.applied);
    EXPECT_STREQ(ctx.request_mutation.path, "/orders?limit=500");
}

TEST(PaginationRequestMutationStageTest, ValidLimit_NotMutated) {
    auto rp = make_route(20, 500);
    auto ctx = make_ctx(&rp, "/orders", "limit=100");
    pagination_request_mutation_stage(ctx);
    EXPECT_FALSE(ctx.request_mutation.applied);
}

TEST(PaginationRequestMutationStageTest, DisabledPolicy_NotMutated) {
    auto rp = make_route(20, 500);
    rp.pagination.enabled = false;
    auto ctx = make_ctx(&rp, "/orders", "");
    pagination_request_mutation_stage(ctx);
    EXPECT_FALSE(ctx.request_mutation.applied);
}

TEST(PaginationRequestMutationStageTest, NoEnvoyDependency) {
    // Compilation-only assertion: if this file compiles without protobuf
    // headers, the no-Envoy-dependency invariant holds.
    SUCCEED();
}

} // namespace bytetaper::stages
