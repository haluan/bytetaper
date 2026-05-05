// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "envoy/config/core/v3/base.pb.h"
#include "extproc/header_view.h"

#include <gtest/gtest.h>

namespace bytetaper::extproc {

TEST(HeaderViewTest, RequestScan_PathPresent) {
    envoy::config::core::v3::HeaderMap headers;
    auto* h = headers.add_headers();
    h->set_key(":path");
    h->set_value("/test");

    auto view = scan_request_headers(headers);
    ASSERT_NE(view.path, nullptr);
    EXPECT_STREQ(view.path, "/test");
    EXPECT_EQ(view.path_len, 5);
}

TEST(HeaderViewTest, RequestScan_MethodPresent) {
    envoy::config::core::v3::HeaderMap headers;
    auto* h = headers.add_headers();
    h->set_key(":method");
    h->set_value("GET");

    auto view = scan_request_headers(headers);
    ASSERT_NE(view.method, nullptr);
    EXPECT_STREQ(view.method, "GET");
    EXPECT_EQ(view.method_len, 3);
}

TEST(HeaderViewTest, RequestScan_AcceptEncodingPresent) {
    envoy::config::core::v3::HeaderMap headers;
    auto* h = headers.add_headers();
    h->set_key("accept-encoding");
    h->set_value("gzip, br");

    auto view = scan_request_headers(headers);
    ASSERT_NE(view.accept_encoding, nullptr);
    EXPECT_STREQ(view.accept_encoding, "gzip, br");
}

TEST(HeaderViewTest, RequestScan_MissingHeaders) {
    envoy::config::core::v3::HeaderMap headers;
    auto view = scan_request_headers(headers);
    EXPECT_EQ(view.path, nullptr);
    EXPECT_EQ(view.method, nullptr);
    EXPECT_EQ(view.accept_encoding, nullptr);
}

TEST(HeaderViewTest, RequestScan_IrrelevantHeaderIgnored) {
    envoy::config::core::v3::HeaderMap headers;
    auto* h = headers.add_headers();
    h->set_key("x-irrelevant");
    h->set_value("value");

    auto view = scan_request_headers(headers);
    EXPECT_EQ(view.path, nullptr);
    EXPECT_EQ(view.method, nullptr);
}

TEST(HeaderViewTest, ResponseScan_AllPresent) {
    envoy::config::core::v3::HeaderMap headers;
    {
        auto* h = headers.add_headers();
        h->set_key(":status");
        h->set_value("200");
    }
    {
        auto* h = headers.add_headers();
        h->set_key("content-type");
        h->set_value("application/json");
    }
    {
        auto* h = headers.add_headers();
        h->set_key("content-encoding");
        h->set_value("br");
    }
    {
        auto* h = headers.add_headers();
        h->set_key("content-length");
        h->set_value("123");
    }

    auto view = scan_response_headers(headers);
    ASSERT_NE(view.status, nullptr);
    EXPECT_STREQ(view.status, "200");
    ASSERT_NE(view.content_type, nullptr);
    EXPECT_STREQ(view.content_type, "application/json");
    ASSERT_NE(view.content_encoding, nullptr);
    EXPECT_STREQ(view.content_encoding, "br");
    ASSERT_NE(view.content_length, nullptr);
    EXPECT_STREQ(view.content_length, "123");
}

TEST(HeaderViewTest, ResponseScan_MissingHeaders) {
    envoy::config::core::v3::HeaderMap headers;
    auto view = scan_response_headers(headers);
    EXPECT_EQ(view.status, nullptr);
    EXPECT_EQ(view.content_type, nullptr);
    EXPECT_EQ(view.content_encoding, nullptr);
    EXPECT_EQ(view.content_length, nullptr);
}

TEST(HeaderViewTest, ResponseScan_ContentTypeOnly) {
    envoy::config::core::v3::HeaderMap headers;
    auto* h = headers.add_headers();
    h->set_key("content-type");
    h->set_value("text/plain");

    auto view = scan_response_headers(headers);
    EXPECT_EQ(view.status, nullptr);
    ASSERT_NE(view.content_type, nullptr);
    EXPECT_STREQ(view.content_type, "text/plain");
}

TEST(HeaderViewTest, ResponseScan_RawValuePreferredOverValue) {
    envoy::config::core::v3::HeaderMap headers;
    auto* h = headers.add_headers();
    h->set_key(":status");
    h->set_value("404");
    h->set_raw_value("200");

    auto view = scan_response_headers(headers);
    ASSERT_NE(view.status, nullptr);
    EXPECT_STREQ(view.status, "200");
}

} // namespace bytetaper::extproc
