// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_EXTPROC_PROTO_BOUNDARY_H
#define BYTETAPER_EXTPROC_PROTO_BOUNDARY_H

namespace bytetaper::extproc {

// Verifies adapter-level linkage against generated Envoy ext_proc protobuf messages.
bool verify_proto_linkage();

} // namespace bytetaper::extproc

#endif // BYTETAPER_EXTPROC_PROTO_BOUNDARY_H
