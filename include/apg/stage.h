// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_APG_STAGE_H
#define BYTETAPER_APG_STAGE_H

#include "apg/context.h"

namespace bytetaper::apg {

using ApgStage = bool (*)(ApgTransformContext&);

} // namespace bytetaper::apg

#endif // BYTETAPER_APG_STAGE_H
