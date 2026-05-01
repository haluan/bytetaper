// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "extproc/proto_boundary.h"

int main() {
    if (!bytetaper::extproc::verify_proto_linkage()) {
        return 1;
    }
    return 0;
}
