// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#pragma once

#include "apg/context.h"
#include "apg/stage.h"

namespace bytetaper::stages {

apg::StageOutput compression_decision_stage(apg::ApgTransformContext& context);

} // namespace bytetaper::stages
