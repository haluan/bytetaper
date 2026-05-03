// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "pagination/pagination_mutation.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::pagination {

static PaginationMutationResult mutate(const char* path, const char* query, bool apply,
                                       std::uint32_t limit, const char* param, char* buf,
                                       std::size_t sz) {
    PaginationDecision d{ apply, limit };
    return apply_pagination_mutation(path, std::strlen(path), query, std::strlen(query), d, param,
                                     buf, sz);
}

TEST(DefaultLimitMutationTest, NoQuery_DefaultLimitAdded) {
    char buf[256]{};
    auto r = mutate("/orders", "", true, 50, "limit", buf, sizeof(buf));
    EXPECT_TRUE(r.mutated);
    EXPECT_STREQ(buf, "/orders?limit=50");
}

TEST(DefaultLimitMutationTest, ExistingQuery_DefaultLimitAppended) {
    char buf[256]{};
    auto r = mutate("/orders", "status=open", true, 50, "limit", buf, sizeof(buf));
    EXPECT_TRUE(r.mutated);
    EXPECT_STREQ(buf, "/orders?status=open&limit=50");
}

TEST(DefaultLimitMutationTest, NoMutation_QueryPreserved) {
    char buf[256]{};
    auto r = mutate("/orders", "limit=99", false, 50, "limit", buf, sizeof(buf));
    EXPECT_FALSE(r.mutated);
    EXPECT_STREQ(buf, "/orders?limit=99");
}

TEST(DefaultLimitMutationTest, CustomLimitParam) {
    char buf[256]{};
    auto r = mutate("/items", "", true, 25, "page_size", buf, sizeof(buf));
    EXPECT_TRUE(r.mutated);
    EXPECT_STREQ(buf, "/items?page_size=25");
}

TEST(DefaultLimitMutationTest, OutputDeterministic) {
    char buf1[256]{}, buf2[256]{};
    PaginationDecision d{ true, 10 };
    apply_pagination_mutation("/x", 2, "a=1", 3, d, "limit", buf1, sizeof(buf1));
    apply_pagination_mutation("/x", 2, "a=1", 3, d, "limit", buf2, sizeof(buf2));
    EXPECT_STREQ(buf1, buf2);
}

} // namespace bytetaper::pagination
