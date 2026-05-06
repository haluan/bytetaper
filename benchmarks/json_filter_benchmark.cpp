// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "apg/context.h"
#include "json_transform/content_type.h"
#include "policy/field_filter_policy.h"

#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace bytetaper::json_transform;

struct BenchmarkResult {
    std::string scenario;
    std::string fixture_size;
    std::string selection;
    std::size_t iterations;
    double total_ns;
    double ns_per_op;
    std::size_t bytes_per_op;
    std::string status;
};

void set_benchmark_fields(bytetaper::apg::ApgTransformContext* context, const std::vector<const char*>& fields) {
    context->selected_field_count = 0;
    for (std::size_t index = 0; index < bytetaper::policy::kMaxFields; ++index) {
        context->selected_fields[index][0] = '\0';
    }
    for (std::size_t i = 0; i < fields.size() && i < bytetaper::policy::kMaxFields; ++i) {
        if (fields[i] == nullptr) {
            continue;
        }
        std::strncpy(context->selected_fields[context->selected_field_count], fields[i],
                     bytetaper::policy::kMaxFieldNameLen - 1);
        context->selected_fields[context->selected_field_count][bytetaper::policy::kMaxFieldNameLen - 1] = '\0';
        context->selected_field_count++;
    }
}

template <typename F>
BenchmarkResult run_benchmark(const std::string& scenario, const std::string& fixture_size,
                              const std::string& selection, std::size_t iterations, F&& func) {
    // 1. Warmup
    constexpr std::size_t kWarmupIterations = 1000;
    for (std::size_t i = 0; i < kWarmupIterations; ++i) {
        auto res = func();
        (void)res;
    }

    // 2. Measure
    auto start = std::chrono::high_resolution_clock::now();
    std::size_t bytes_produced = 0;
    std::string final_status = "Ok";
    for (std::size_t i = 0; i < iterations; ++i) {
        auto [status, len] = func();
        bytes_produced = len;
        if (status != "Ok") {
            final_status = status;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto total_ns = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());

    BenchmarkResult result;
    result.scenario = scenario;
    result.fixture_size = fixture_size;
    result.selection = selection;
    result.iterations = iterations;
    result.total_ns = total_ns;
    result.ns_per_op = total_ns / static_cast<double>(iterations);
    result.bytes_per_op = bytes_produced;
    result.status = final_status;
    return result;
}

int main() {
    std::cout << "Starting JSON filtering microbenchmarks..." << std::endl;

    constexpr std::size_t kIterations = 50000;
    std::vector<BenchmarkResult> results;

    // --- 1. FLAT OBJECT FIXTURES ---
    const char* flat_small = R"({"id":123,"name":"Alice","active":true})";
    const char* flat_medium = R"({"id":123,"name":"Alice","active":true,"email":"alice@example.com","age":30,"city":"San Jose","state":"CA","zip":"95112","country":"USA","company":"DeepMind"})";
    const char* flat_large = R"({"id":123,"name":"Alice","active":true,"email":"alice@example.com","age":30,"city":"San Jose","state":"CA","zip":"95112","country":"USA","company":"DeepMind","title":"AI Engineer","department":"Core Research","team":"Antigravity","manager":"John Doe","office":"Building 1","phone":"555-1234","slack":"alice_dm","twitter":"@alice_ai","github":"alice_git","website":"https://alice.ai"})";

    struct FlatFixture {
        std::string size;
        const char* json;
    };
    std::vector<FlatFixture> flat_fixtures = {
        {"small", flat_small},
        {"medium", flat_medium},
        {"large", flat_large}
    };

    for (const auto& f : flat_fixtures) {
        ParsedFlatJsonObject parsed{};
        auto parse_status = parse_flat_json_object(f.json, JsonResponseKind::EligibleJson, &parsed);
        if (parse_status != FlatJsonParseStatus::Ok) {
            std::cerr << "Failed to parse flat object for size " << f.size << std::endl;
            continue;
        }

        // Standard selection
        bytetaper::apg::ApgTransformContext ctx_standard{};
        set_benchmark_fields(&ctx_standard, {"id", "name", "email", "active"});

        results.push_back(run_benchmark("flat", f.size, "standard", kIterations, [&]() {
            char output[1024] = {};
            std::size_t out_len = 0;
            auto status = filter_flat_json_by_selected_fields(parsed, ctx_standard, output, sizeof(output), &out_len);
            std::string status_str = (status == FlatJsonFilterStatus::Ok) ? "Ok" : "Error";
            return std::make_pair(status_str, out_len);
        }));

        // Control (no-op selection)
        bytetaper::apg::ApgTransformContext ctx_control{};
        set_benchmark_fields(&ctx_control, {});

        results.push_back(run_benchmark("flat", f.size, "control", kIterations, [&]() {
            char output[1024] = {};
            std::size_t out_len = 0;
            auto status = filter_flat_json_by_selected_fields(parsed, ctx_control, output, sizeof(output), &out_len);
            std::string status_str = (status == FlatJsonFilterStatus::Ok) ? "Ok" : "Error";
            return std::make_pair(status_str, out_len);
        }));
    }

    // --- 2. NESTED OBJECT FIXTURES ---
    const char* nested_small = R"({"user":{"name":"Alice"},"id":123})";
    const char* nested_medium = R"({"id":123,"user":{"name":"Alice","profile":{"email":"alice@example.com","phone":"555-1234"},"age":30},"metadata":{"created_at":1714953600}})";
    const char* nested_large = R"({"id":123,"user":{"name":"Alice","profile":{"email":"alice@example.com","phone":"555-1234","address":{"street":"123 Main St","city":"San Jose","state":"CA","zip":"95112"}},"age":30},"metadata":{"created_at":1714953600,"updated_at":1714953600,"version":2,"status":"active","tier":"gold","flags":{"beta":true,"premium":false}}})";

    std::vector<FlatFixture> nested_fixtures = {
        {"small", nested_small},
        {"medium", nested_medium},
        {"large", nested_large}
    };

    for (const auto& f : nested_fixtures) {
        ParsedFlatJsonObject parsed{};
        // Nested JSON defaults to SkipUnsupported, which triggers streaming/tokenization parser fallback
        auto parse_status = parse_flat_json_object(f.json, JsonResponseKind::EligibleJson, &parsed);

        // Standard selection
        bytetaper::apg::ApgTransformContext ctx_standard{};
        set_benchmark_fields(&ctx_standard, {"user.profile.email", "id", "user.name"});

        results.push_back(run_benchmark("nested", f.size, "standard", kIterations, [&]() {
            char output[1024] = {};
            std::size_t out_len = 0;
            auto status = transform_flat_json_with_filter_toggle(f.json, parse_status, &parsed, ctx_standard, true, output, sizeof(output), &out_len);
            std::string status_str = (status == FlatJsonFilterStatus::Ok) ? "Ok" : "Error";
            return std::make_pair(status_str, out_len);
        }));

        // Control (no-op selection)
        bytetaper::apg::ApgTransformContext ctx_control{};
        set_benchmark_fields(&ctx_control, {});

        results.push_back(run_benchmark("nested", f.size, "control", kIterations, [&]() {
            char output[1024] = {};
            std::size_t out_len = 0;
            auto status = transform_flat_json_with_filter_toggle(f.json, parse_status, &parsed, ctx_control, true, output, sizeof(output), &out_len);
            std::string status_str = (status == FlatJsonFilterStatus::Ok) ? "Ok" : "Error";
            return std::make_pair(status_str, out_len);
        }));
    }

    // --- 3. OBJECT-ARRAY FIXTURES ---
    const char* array_small = R"({"id":123,"items":[{"name":"A"}]})";
    const char* array_medium = R"({"id":123,"items":[{"name":"A","price":1.99},{"name":"B","price":2.99},{"name":"C","price":3.99}]})";
    const char* array_large = R"({"id":123,"items":[{"name":"A","price":1.99,"qty":10},{"name":"B","price":2.99,"qty":5},{"name":"C","price":3.99,"qty":20},{"name":"D","price":4.99,"qty":1},{"name":"E","price":5.99,"qty":15},{"name":"F","price":6.99,"qty":8},{"name":"G","price":7.99,"qty":12},{"name":"H","price":8.99,"qty":3}]})";

    std::vector<FlatFixture> array_fixtures = {
        {"small", array_small},
        {"medium", array_medium},
        {"large", array_large}
    };

    for (const auto& f : array_fixtures) {
        ParsedFlatJsonObject parsed{};
        auto parse_status = parse_flat_json_object(f.json, JsonResponseKind::EligibleJson, &parsed);

        // Standard selection
        bytetaper::apg::ApgTransformContext ctx_standard{};
        set_benchmark_fields(&ctx_standard, {"items[].name", "id", "items[].qty"});

        results.push_back(run_benchmark("array", f.size, "standard", kIterations, [&]() {
            char output[1024] = {};
            std::size_t out_len = 0;
            auto status = transform_flat_json_with_filter_toggle(f.json, parse_status, &parsed, ctx_standard, true, output, sizeof(output), &out_len);
            std::string status_str = (status == FlatJsonFilterStatus::Ok) ? "Ok" : "Error";
            return std::make_pair(status_str, out_len);
        }));

        // Control (no-op selection)
        bytetaper::apg::ApgTransformContext ctx_control{};
        set_benchmark_fields(&ctx_control, {});

        results.push_back(run_benchmark("array", f.size, "control", kIterations, [&]() {
            char output[1024] = {};
            std::size_t out_len = 0;
            auto status = transform_flat_json_with_filter_toggle(f.json, parse_status, &parsed, ctx_control, true, output, sizeof(output), &out_len);
            std::string status_str = (status == FlatJsonFilterStatus::Ok) ? "Ok" : "Error";
            return std::make_pair(status_str, out_len);
        }));
    }

    // --- 4. EMIT CSV RESULTS ---
    std::string report_dir = "reports/benchmarks/json-filtering";
    std::filesystem::create_directories(report_dir);
    std::string report_path = report_dir + "/benchmark_results.csv";

    std::ofstream csv(report_path);
    if (!csv.is_open()) {
        std::cerr << "Failed to open output CSV path: " << report_path << std::endl;
        return 1;
    }

    csv << "scenario,fixture_size,selection,iterations,total_ns,ns_per_op,bytes_per_op,status\n";
    std::cout << "\nBenchmark Results summary:\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << "Scenario\tSize\tSelection\tNs/Op\t\tBytesProduced\tStatus\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    for (const auto& r : results) {
        csv << r.scenario << ","
            << r.fixture_size << ","
            << r.selection << ","
            << r.iterations << ","
            << r.total_ns << ","
            << r.ns_per_op << ","
            << r.bytes_per_op << ","
            << r.status << "\n";

        std::cout << r.scenario << "\t\t"
                  << r.fixture_size << "\t"
                  << r.selection << "\t\t"
                  << r.ns_per_op << "\t\t"
                  << r.bytes_per_op << "\t\t"
                  << r.status << "\n";
    }
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << "Results successfully written to " << report_path << std::endl;

    return 0;
}
