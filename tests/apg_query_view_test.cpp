// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "apg/query_view.h"

#include <cstdio>
#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::apg {

TEST(ApgQueryViewTest, CanonicalQuery) {
    const char* q = "limit=10&offset=5&fields=id,name";
    RequestQueryView view{};
    parse_query_view(q, std::strlen(q), &view);

    EXPECT_EQ(view.count, 3);

    EXPECT_EQ(view.params[0].key_len, 5);
    EXPECT_TRUE(std::memcmp(view.params[0].key, "limit", 5) == 0);
    EXPECT_EQ(view.params[0].value_len, 2);
    EXPECT_TRUE(std::memcmp(view.params[0].value, "10", 2) == 0);

    EXPECT_EQ(view.params[1].key_len, 6);
    EXPECT_TRUE(std::memcmp(view.params[1].key, "offset", 6) == 0);
    EXPECT_EQ(view.params[1].value_len, 1);
    EXPECT_TRUE(std::memcmp(view.params[1].value, "5", 1) == 0);

    EXPECT_EQ(view.params[2].key_len, 6);
    EXPECT_TRUE(std::memcmp(view.params[2].key, "fields", 6) == 0);
    EXPECT_EQ(view.params[2].value_len, 7);
    EXPECT_TRUE(std::memcmp(view.params[2].value, "id,name", 7) == 0);
}

TEST(ApgQueryViewTest, SegmentsWithoutEquals) {
    const char* q = "flag1&flag2=true&flag3";
    RequestQueryView view{};
    parse_query_view(q, std::strlen(q), &view);

    EXPECT_EQ(view.count, 3);

    EXPECT_EQ(view.params[0].key_len, 5);
    EXPECT_TRUE(std::memcmp(view.params[0].key, "flag1", 5) == 0);
    EXPECT_EQ(view.params[0].value, nullptr);
    EXPECT_EQ(view.params[0].value_len, 0);

    EXPECT_EQ(view.params[1].key_len, 5);
    EXPECT_TRUE(std::memcmp(view.params[1].key, "flag2", 5) == 0);
    EXPECT_EQ(view.params[1].value_len, 4);
    EXPECT_TRUE(std::memcmp(view.params[1].value, "true", 4) == 0);

    EXPECT_EQ(view.params[2].key_len, 5);
    EXPECT_TRUE(std::memcmp(view.params[2].key, "flag3", 5) == 0);
    EXPECT_EQ(view.params[2].value, nullptr);
    EXPECT_EQ(view.params[2].value_len, 0);
}

TEST(ApgQueryViewTest, EmptyValues) {
    const char* q = "fields=&limit=";
    RequestQueryView view{};
    parse_query_view(q, std::strlen(q), &view);

    EXPECT_EQ(view.count, 2);

    EXPECT_EQ(view.params[0].key_len, 6);
    EXPECT_TRUE(std::memcmp(view.params[0].key, "fields", 6) == 0);
    EXPECT_EQ(view.params[0].value_len, 0);

    EXPECT_EQ(view.params[1].key_len, 5);
    EXPECT_TRUE(std::memcmp(view.params[1].key, "limit", 5) == 0);
    EXPECT_EQ(view.params[1].value_len, 0);
}

TEST(ApgQueryViewTest, BoundedTruncation) {
    char q[1024] = {};
    char* p = q;
    for (int i = 0; i < 35; ++i) {
        p += std::sprintf(p, "k%d=v%d&", i, i);
    }
    q[std::strlen(q) - 1] = '\0'; // strip trailing &

    RequestQueryView view{};
    parse_query_view(q, std::strlen(q), &view);

    EXPECT_EQ(view.count, 32);

    for (int i = 0; i < 32; ++i) {
        char expected_key[16];
        int klen = std::sprintf(expected_key, "k%d", i);
        char expected_val[16];
        int vlen = std::sprintf(expected_val, "v%d", i);

        EXPECT_EQ(view.params[i].key_len, klen);
        EXPECT_TRUE(std::memcmp(view.params[i].key, expected_key, klen) == 0);
        EXPECT_EQ(view.params[i].value_len, vlen);
        EXPECT_TRUE(std::memcmp(view.params[i].value, expected_val, vlen) == 0);
    }
}

TEST(ApgQueryViewTest, EmptyOrNullString) {
    RequestQueryView view{};
    parse_query_view(nullptr, 0, &view);
    EXPECT_EQ(view.count, 0);

    parse_query_view("", 0, &view);
    EXPECT_EQ(view.count, 0);
}

} // namespace bytetaper::apg
