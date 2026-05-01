// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_FIELD_SELECTION_REQUEST_TARGET_H
#define BYTETAPER_FIELD_SELECTION_REQUEST_TARGET_H

#include "apg/context.h"

namespace bytetaper::field_selection {

bool extract_raw_path_and_query(const char* request_target, apg::ApgTransformContext* context);

} // namespace bytetaper::field_selection

#endif // BYTETAPER_FIELD_SELECTION_REQUEST_TARGET_H
