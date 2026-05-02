// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_APG_PIPELINE_H
#define BYTETAPER_APG_PIPELINE_H

#include "apg/stage.h"

#include <cstddef>

namespace bytetaper::apg {

StageOutput run_pipeline(const ApgStage* stages, std::size_t count, ApgTransformContext& context);

} // namespace bytetaper::apg

#endif // BYTETAPER_APG_PIPELINE_H
