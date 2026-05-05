// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_EXTPROC_DEFAULT_PIPELINES_H
#define BYTETAPER_EXTPROC_DEFAULT_PIPELINES_H

#include "apg/stage.h"

#include <cstddef>

namespace bytetaper::extproc {

/**
 * HOT-PATH: request-header pipeline.
 *
 * This pipeline runs synchronously on the Envoy ExtProc request thread.
 * It must NOT contain any stages that perform synchronous disk I/O (e.g., RocksDB get).
 *
 * See docs/runtime/RUNTIME_BOUNDARIES.md for the enforcement contract.
 */
extern const apg::ApgStage kLookupStages[];
extern const std::size_t kLookupStageCount;

/**
 * HOT-PATH: response-body pipeline.
 *
 * This pipeline runs synchronously on the Envoy ExtProc response thread.
 * It must NOT contain any stages that perform synchronous disk I/O (e.g., RocksDB put).
 *
 * See docs/runtime/RUNTIME_BOUNDARIES.md for the enforcement contract.
 */
extern const apg::ApgStage kStoreStages[];
extern const std::size_t kStoreStageCount;

} // namespace bytetaper::extproc

#endif // BYTETAPER_EXTPROC_DEFAULT_PIPELINES_H
