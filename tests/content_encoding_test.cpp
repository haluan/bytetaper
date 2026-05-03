// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "compression/content_encoding.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::compression {

TEST(ContentEncodingTest, Gzip_AlreadyEncoded) {
    const char* h = "gzip";
    auto r = detect_content_encoding(h, std::strlen(h));
    EXPECT_TRUE(r.already_encoded);
    EXPECT_EQ(r.encoding, policy::CompressionAlgorithm::Gzip);
}

TEST(ContentEncodingTest, Brotli_AlreadyEncoded) {
    const char* h = "br";
    auto r = detect_content_encoding(h, std::strlen(h));
    EXPECT_TRUE(r.already_encoded);
    EXPECT_EQ(r.encoding, policy::CompressionAlgorithm::Brotli);
}

TEST(ContentEncodingTest, Zstd_AlreadyEncoded) {
    const char* h = "zstd";
    auto r = detect_content_encoding(h, std::strlen(h));
    EXPECT_TRUE(r.already_encoded);
    EXPECT_EQ(r.encoding, policy::CompressionAlgorithm::Zstd);
}

TEST(ContentEncodingTest, MissingHeader_NotEncoded) {
    auto r = detect_content_encoding(nullptr, 0);
    EXPECT_FALSE(r.already_encoded);
    EXPECT_EQ(r.encoding, policy::CompressionAlgorithm::None);
}

TEST(ContentEncodingTest, EmptyHeader_NotEncoded) {
    auto r = detect_content_encoding("", 0);
    EXPECT_FALSE(r.already_encoded);
}

TEST(ContentEncodingTest, UnknownEncoding_AlreadyEncodedNoneAlgorithm) {
    // deflate is encoded but not a preferred algorithm — still blocks re-compression.
    const char* h = "deflate";
    auto r = detect_content_encoding(h, std::strlen(h));
    EXPECT_TRUE(r.already_encoded);
    EXPECT_EQ(r.encoding, policy::CompressionAlgorithm::None);
}

} // namespace bytetaper::compression
