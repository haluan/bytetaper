// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#pragma once

#include "policy/compression_policy.h"

#include <cstddef>

namespace bytetaper::compression {

// Returns true if content_type is eligible for compression per policy.
// Strips MIME parameters (e.g., "; charset=utf-8") before matching.
// Returns false when content_type is nullptr or empty.
// When policy.eligible_content_type_count == 0, all non-empty content types
// are considered eligible (policy does not restrict by type).
bool is_content_type_eligible(const policy::CompressionPolicy& policy, const char* content_type,
                              std::size_t len);

} // namespace bytetaper::compression
