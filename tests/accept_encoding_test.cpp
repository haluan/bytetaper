// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "compression/accept_encoding.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::compression {

TEST(AcceptEncodingTest, GzipDetected) {
    const char* h = "gzip";
    auto r = parse_accept_encoding(h, std::strlen(h));
    EXPECT_TRUE(r.supports_gzip);
    EXPECT_FALSE(r.supports_br);
}

TEST(AcceptEncodingTest, BrotliDetected) {
    const char* h = "br";
    auto r = parse_accept_encoding(h, std::strlen(h));
    EXPECT_TRUE(r.supports_br);
}

TEST(AcceptEncodingTest, ZstdDetected) {
    const char* h = "zstd";
    auto r = parse_accept_encoding(h, std::strlen(h));
    EXPECT_TRUE(r.supports_zstd);
}

TEST(AcceptEncodingTest, CommaSeparatedAll) {
    const char* h = "br, gzip, deflate";
    auto r = parse_accept_encoding(h, std::strlen(h));
    EXPECT_TRUE(r.supports_br);
    EXPECT_TRUE(r.supports_gzip);
    EXPECT_TRUE(r.supports_deflate);
    EXPECT_FALSE(r.supports_zstd);
}

TEST(AcceptEncodingTest, MissingHeader_Safe) {
    auto r = parse_accept_encoding(nullptr, 0);
    EXPECT_FALSE(r.supports_gzip);
    EXPECT_FALSE(r.supports_br);
    EXPECT_FALSE(r.supports_zstd);
}

TEST(AcceptEncodingTest, QValues_Ignored_PresenceDetected) {
    // q-values are stripped; presence is still recorded.
    const char* h = "gzip;q=0.9, br;q=1.0, zstd;q=0.5";
    auto r = parse_accept_encoding(h, std::strlen(h));
    EXPECT_TRUE(r.supports_gzip);
    EXPECT_TRUE(r.supports_br);
    EXPECT_TRUE(r.supports_zstd);
}

TEST(AcceptEncodingTest, EmptyString_Safe) {
    auto r = parse_accept_encoding("", 0);
    EXPECT_FALSE(r.supports_gzip);
    EXPECT_FALSE(r.supports_br);
}

} // namespace bytetaper::compression
