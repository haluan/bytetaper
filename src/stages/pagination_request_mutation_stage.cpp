// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "stages/pagination_request_mutation_stage.h"

#include "pagination/pagination_decision.h"
#include "pagination/pagination_mutation.h"
#include "pagination/pagination_query.h"

namespace bytetaper::stages {

apg::StageOutput pagination_request_mutation_stage(apg::ApgTransformContext& context) {
    if (context.matched_policy == nullptr) {
        return { apg::StageResult::Continue, "no-policy" };
    }

    const auto& pag = context.matched_policy->pagination;

    if (!pag.enabled) {
        return { apg::StageResult::Continue, "pagination-disabled" };
    }

    const char* validate_err = policy::validate_pagination_policy(pag);
    if (validate_err != nullptr) {
        return { apg::StageResult::Continue, validate_err };
    }

    pagination::PaginationQueryResult qr = pagination::parse_pagination_query(
        context.raw_query, context.raw_query_length, pag.limit_param, pag.offset_param, pag.mode);

    pagination::PaginationDecision decision = pagination::make_pagination_decision(pag, qr);

    if (!decision.should_apply_default_limit && !decision.should_cap_limit) {
        return { apg::StageResult::Continue,
                 decision.reason != nullptr ? decision.reason : "no-pagination-mutation" };
    }

    auto mr = pagination::apply_pagination_mutation(
        context.raw_path, context.raw_path_length, context.raw_query, context.raw_query_length,
        decision, pag.limit_param, context.request_mutation.path,
        sizeof(context.request_mutation.path));

    if (!mr.mutated) {
        return { apg::StageResult::Continue, "pagination-mutation-overflow" };
    }

    context.request_mutation.path_length = mr.written;
    context.request_mutation.applied = true;
    context.request_mutation.reason = decision.reason;

    return { apg::StageResult::Continue, decision.reason };
}

} // namespace bytetaper::stages
