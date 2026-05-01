// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "json_transform/content_type.h"

#include <gtest/gtest.h>

namespace bytetaper::json_transform {

namespace {

struct DetectionCase {
    const char* content_type;
    JsonResponseKind expected_kind;
};

} // namespace

TEST(JsonTransformContentTypeTest, DetectsEligibleJsonContentTypes) {
    const DetectionCase cases[] = {
        { "application/json", JsonResponseKind::EligibleJson },
        { "application/json; charset=utf-8", JsonResponseKind::EligibleJson },
        { "Application/Json", JsonResponseKind::EligibleJson },
        { " application/json \t; charset=utf-8", JsonResponseKind::EligibleJson },
        { "application/vnd.api+json", JsonResponseKind::EligibleJson },
        { "application/problem+json; charset=utf-8", JsonResponseKind::EligibleJson },
    };

    for (const DetectionCase& detection_case : cases) {
        JsonResponseKind kind = JsonResponseKind::InvalidInput;
        EXPECT_TRUE(detect_application_json_response(detection_case.content_type, &kind));
        EXPECT_EQ(kind, detection_case.expected_kind);
    }
}

TEST(JsonTransformContentTypeTest, SkipsUnsupportedContentTypesSafely) {
    const DetectionCase cases[] = {
        { "text/plain", JsonResponseKind::SkipUnsupported },
        { "application/xml", JsonResponseKind::SkipUnsupported },
        { "", JsonResponseKind::SkipUnsupported },
        { " \t ", JsonResponseKind::SkipUnsupported },
        { "application/+json", JsonResponseKind::SkipUnsupported },
        { nullptr, JsonResponseKind::SkipUnsupported },
    };

    for (const DetectionCase& detection_case : cases) {
        JsonResponseKind kind = JsonResponseKind::InvalidInput;
        EXPECT_TRUE(detect_application_json_response(detection_case.content_type, &kind));
        EXPECT_EQ(kind, detection_case.expected_kind);
    }
}

TEST(JsonTransformContentTypeTest, ReturnsFalseForNullOutputPointer) {
    EXPECT_FALSE(detect_application_json_response("application/json", nullptr));
    EXPECT_FALSE(detect_application_json_response(nullptr, nullptr));
}

} // namespace bytetaper::json_transform
