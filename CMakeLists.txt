cmake_minimum_required(VERSION 3.22)

project(ByteTaper LANGUAGES CXX)

include(FetchContent)
include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(RocksDB REQUIRED)
option(BYTETAPER_ENABLE_GTEST_TESTS "Enable GoogleTest-based unit tests" OFF)
option(BYTETAPER_ENABLE_INTEGRATION_TESTS "Enable integration-style extproc adapter tests" OFF)

add_compile_options(
  -fno-exceptions
  -fno-rtti
)

add_executable(bytetaper-extproc
  src/main.cpp
)
target_include_directories(bytetaper-extproc
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(bytetaper-extproc
  PRIVATE
    RocksDB::RocksDB
)

add_executable(bytetaper-validate-policy
  src/policy/validate_policy_main.cpp
)
target_include_directories(bytetaper-validate-policy
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(bytetaper-validate-policy
  PRIVATE
    policy_yaml_loader
)

if(BYTETAPER_ENABLE_INTEGRATION_TESTS)
  find_package(Protobuf REQUIRED)
  find_package(gRPC CONFIG REQUIRED)
  if(NOT gRPC_CPP_PLUGIN_EXECUTABLE)
    find_program(gRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin REQUIRED)
  endif()

  set(BYTETAPER_PROTO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/proto")
  set(BYTETAPER_GENERATED_PROTO_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/extproc")
  set(BYTETAPER_EXTPROC_SERVICE_PROTO "envoy/service/ext_proc/v3/external_processor.proto")

  file(GLOB_RECURSE BYTETAPER_VENDORED_PROTO_RELATIVE
    RELATIVE "${BYTETAPER_PROTO_ROOT}"
    CONFIGURE_DEPENDS
    "${BYTETAPER_PROTO_ROOT}/*.proto"
  )
  set(BYTETAPER_VENDORED_PROTOS_ABSOLUTE "")
  set(BYTETAPER_GENERATED_PROTO_SOURCES "")
  set(BYTETAPER_GENERATED_PROTO_HEADERS "")
  foreach(BYTETAPER_PROTO_FILE IN LISTS BYTETAPER_VENDORED_PROTO_RELATIVE)
    list(APPEND BYTETAPER_VENDORED_PROTOS_ABSOLUTE
      "${BYTETAPER_PROTO_ROOT}/${BYTETAPER_PROTO_FILE}"
    )
    string(REPLACE ".proto" ".pb.cc" BYTETAPER_PROTO_CC "${BYTETAPER_PROTO_FILE}")
    string(REPLACE ".proto" ".pb.h" BYTETAPER_PROTO_H "${BYTETAPER_PROTO_FILE}")
    list(APPEND BYTETAPER_GENERATED_PROTO_SOURCES
      "${BYTETAPER_GENERATED_PROTO_DIR}/${BYTETAPER_PROTO_CC}"
    )
    list(APPEND BYTETAPER_GENERATED_PROTO_HEADERS
      "${BYTETAPER_GENERATED_PROTO_DIR}/${BYTETAPER_PROTO_H}"
    )
  endforeach()

  add_custom_command(
    OUTPUT ${BYTETAPER_GENERATED_PROTO_SOURCES} ${BYTETAPER_GENERATED_PROTO_HEADERS}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS
      --cpp_out=${BYTETAPER_GENERATED_PROTO_DIR}
      -I ${BYTETAPER_PROTO_ROOT}
      ${BYTETAPER_VENDORED_PROTO_RELATIVE}
    WORKING_DIRECTORY ${BYTETAPER_PROTO_ROOT}
    DEPENDS ${BYTETAPER_VENDORED_PROTOS_ABSOLUTE}
    COMMENT "Generating C++ protobuf sources for Envoy ext_proc snapshot"
    VERBATIM
  )

  add_custom_target(bytetaper_extproc_proto_codegen
    DEPENDS ${BYTETAPER_GENERATED_PROTO_SOURCES} ${BYTETAPER_GENERATED_PROTO_HEADERS}
  )

  string(REPLACE ".proto" ".grpc.pb.cc" BYTETAPER_EXTPROC_GRPC_CC "${BYTETAPER_EXTPROC_SERVICE_PROTO}")
  string(REPLACE ".proto" ".grpc.pb.h" BYTETAPER_EXTPROC_GRPC_H "${BYTETAPER_EXTPROC_SERVICE_PROTO}")
  set(BYTETAPER_GENERATED_GRPC_SOURCE "${BYTETAPER_GENERATED_PROTO_DIR}/${BYTETAPER_EXTPROC_GRPC_CC}")
  set(BYTETAPER_GENERATED_GRPC_HEADER "${BYTETAPER_GENERATED_PROTO_DIR}/${BYTETAPER_EXTPROC_GRPC_H}")

  add_custom_command(
    OUTPUT ${BYTETAPER_GENERATED_GRPC_SOURCE} ${BYTETAPER_GENERATED_GRPC_HEADER}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS
      --grpc_out=${BYTETAPER_GENERATED_PROTO_DIR}
      --plugin=protoc-gen-grpc=${gRPC_CPP_PLUGIN_EXECUTABLE}
      -I ${BYTETAPER_PROTO_ROOT}
      ${BYTETAPER_EXTPROC_SERVICE_PROTO}
    WORKING_DIRECTORY ${BYTETAPER_PROTO_ROOT}
    DEPENDS "${BYTETAPER_PROTO_ROOT}/${BYTETAPER_EXTPROC_SERVICE_PROTO}"
    COMMENT "Generating C++ gRPC sources for Envoy ext_proc service"
    VERBATIM
  )

  add_custom_target(bytetaper_extproc_grpc_codegen
    DEPENDS ${BYTETAPER_GENERATED_GRPC_SOURCE} ${BYTETAPER_GENERATED_GRPC_HEADER}
  )

  add_library(bytetaper_extproc_proto STATIC
    ${BYTETAPER_GENERATED_PROTO_SOURCES}
  )
  add_dependencies(bytetaper_extproc_proto bytetaper_extproc_proto_codegen)
  target_include_directories(bytetaper_extproc_proto
    PUBLIC
      ${BYTETAPER_GENERATED_PROTO_DIR}
  )
  target_link_libraries(bytetaper_extproc_proto
    PUBLIC
      protobuf::libprotobuf
  )

  add_library(bytetaper_extproc_grpc STATIC
    ${BYTETAPER_GENERATED_GRPC_SOURCE}
  )
  add_dependencies(bytetaper_extproc_grpc bytetaper_extproc_grpc_codegen bytetaper_extproc_proto_codegen)
  target_include_directories(bytetaper_extproc_grpc
    PUBLIC
      ${BYTETAPER_GENERATED_PROTO_DIR}
  )
  target_link_libraries(bytetaper_extproc_grpc
    PUBLIC
      bytetaper_extproc_proto
      gRPC::grpc++
      protobuf::libprotobuf
  )

  add_library(bytetaper_extproc_proto_boundary STATIC
    src/extproc/proto_boundary.cpp
  )
  target_include_directories(bytetaper_extproc_proto_boundary
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
      ${BYTETAPER_GENERATED_PROTO_DIR}
  )
  target_link_libraries(bytetaper_extproc_proto_boundary
    PRIVATE
      bytetaper_extproc_proto
  )

  add_library(bytetaper_extproc_grpc_server STATIC
    src/extproc/grpc_server.cpp
    src/extproc/request_runtime.cpp
    src/field_selection/request_target.cpp
    src/json_transform/content_type.cpp
    src/json_transform/flat_json.cpp
    src/json_transform/filter_flat_json.cpp
    src/safety/fail_open.cpp
  )
  target_include_directories(bytetaper_extproc_grpc_server
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
      ${BYTETAPER_GENERATED_PROTO_DIR}
  )
  target_link_libraries(bytetaper_extproc_grpc_server
    PRIVATE
      bytetaper_extproc_grpc
  )

  find_package(PkgConfig REQUIRED)
  pkg_check_modules(YAML_CPP REQUIRED yaml-cpp)

  add_library(policy_yaml_loader STATIC
    src/policy/yaml_loader.cpp
  )
  set_source_files_properties(src/policy/yaml_loader.cpp
    PROPERTIES COMPILE_OPTIONS "-fexceptions"
  )
  set_source_files_properties(src/cache/l2_rocksdb_cache.cpp
    PROPERTIES COMPILE_OPTIONS "-fexceptions"
  )
  target_include_directories(policy_yaml_loader
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${YAML_CPP_INCLUDE_DIRS}
  )
  target_link_libraries(policy_yaml_loader
    PUBLIC
      ${YAML_CPP_LIBRARIES}
  )

  find_package(Threads REQUIRED)

  add_library(bytetaper_prometheus_registry STATIC
    src/metrics/prometheus_registry.cpp
  )
  target_include_directories(bytetaper_prometheus_registry
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

  add_library(bytetaper_metrics_http_server STATIC
    src/metrics/metrics_http_server.cpp
  )
  target_include_directories(bytetaper_metrics_http_server
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  target_link_libraries(bytetaper_metrics_http_server
    PRIVATE
      bytetaper_prometheus_registry
      Threads::Threads
  )

  add_executable(bytetaper-extproc-server
    src/extproc/server_main.cpp
  )
  target_include_directories(bytetaper-extproc-server
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${BYTETAPER_GENERATED_PROTO_DIR}
  )
  target_link_libraries(bytetaper-extproc-server
    PRIVATE
      bytetaper_extproc_grpc_server
      bytetaper_extproc_grpc
      policy_yaml_loader
      ${YAML_CPP_LIBRARIES}
  )
endif()

if(BUILD_TESTING)
  add_test(
    NAME bytetaper_extproc_smoke
    COMMAND $<TARGET_FILE:bytetaper-extproc>
  )

  if(BYTETAPER_ENABLE_GTEST_TESTS)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(googletest)

    add_executable(apg_transform_context_test
      tests/apg_transform_context_test.cpp
    )
    target_include_directories(apg_transform_context_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(apg_transform_context_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME apg_transform_context_test
      COMMAND apg_transform_context_test
    )

    add_executable(apg_stage_signature_test
      tests/apg_stage_signature_test.cpp
    )
    target_include_directories(apg_stage_signature_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(apg_stage_signature_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME apg_stage_signature_test
      COMMAND apg_stage_signature_test
    )

    add_executable(apg_result_output_test
      tests/apg_result_output_test.cpp
    )
    target_include_directories(apg_result_output_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(apg_result_output_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME apg_result_output_test
      COMMAND apg_result_output_test
    )

    add_executable(apg_pipeline_runner_test
      tests/apg_pipeline_runner_test.cpp
      src/apg/pipeline.cpp
    )
    target_include_directories(apg_pipeline_runner_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(apg_pipeline_runner_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME apg_pipeline_runner_test
      COMMAND apg_pipeline_runner_test
    )

    add_executable(field_selection_test
      tests/field_selection_test.cpp
      src/field_selection/request_target.cpp
    )
    target_include_directories(field_selection_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(field_selection_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME field_selection_test
      COMMAND field_selection_test
    )

    add_executable(l1_cache_test
      tests/l1_cache_test.cpp
      src/cache/l1_cache.cpp
      src/cache/cache_entry.cpp
    )
    target_include_directories(l1_cache_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(l1_cache_test
      PRIVATE
        gtest_main
        RocksDB::RocksDB
    )

    add_test(
      NAME l1_cache_test
      COMMAND l1_cache_test
    )

    add_executable(l1_cache_lookup_stage_test
      tests/l1_cache_lookup_stage_test.cpp
      src/stages/l1_cache_lookup_stage.cpp
      src/cache/cache_entry.cpp
      src/cache/cache_key.cpp
      src/cache/l1_cache.cpp
    )
    target_include_directories(l1_cache_lookup_stage_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(l1_cache_lookup_stage_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME l1_cache_lookup_stage_test
      COMMAND l1_cache_lookup_stage_test
    )

    add_executable(l2_cache_lookup_stage_test
      tests/l2_cache_lookup_stage_test.cpp
      src/stages/l2_cache_lookup_stage.cpp
      src/cache/l2_rocksdb_cache.cpp
      src/cache/cache_entry_codec.cpp
      src/cache/cache_entry.cpp
      src/cache/cache_key.cpp
      src/cache/cache_ttl.cpp
      src/cache/l1_cache.cpp
    )
    target_include_directories(l2_cache_lookup_stage_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(l2_cache_lookup_stage_test
      PRIVATE
        gtest_main
        RocksDB::RocksDB
    )

    add_test(
      NAME l2_cache_lookup_stage_test
      COMMAND l2_cache_lookup_stage_test
    )

    add_executable(l1_cache_store_stage_test
      tests/l1_cache_store_stage_test.cpp
      src/stages/l1_cache_store_stage.cpp
      src/cache/cache_entry.cpp
      src/cache/cache_key.cpp
      src/cache/l1_cache.cpp
    )
    target_include_directories(l1_cache_store_stage_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(l1_cache_store_stage_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME l1_cache_store_stage_test
      COMMAND l1_cache_store_stage_test
    )

    add_executable(l2_cache_store_stage_test
      tests/l2_cache_store_stage_test.cpp
      src/stages/l2_cache_store_stage.cpp
      src/cache/l2_rocksdb_cache.cpp
      src/cache/cache_entry_codec.cpp
      src/cache/cache_entry.cpp
      src/cache/cache_key.cpp
      src/cache/cache_ttl.cpp
    )
    target_include_directories(l2_cache_store_stage_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(l2_cache_store_stage_test
      PRIVATE
        gtest_main
        RocksDB::RocksDB
    )

    add_test(
      NAME l2_cache_store_stage_test
      COMMAND l2_cache_store_stage_test
    )

    add_executable(l2_promotion_test
      tests/l2_promotion_test.cpp
      src/stages/l2_cache_lookup_stage.cpp
      src/stages/l1_cache_lookup_stage.cpp
      src/cache/l2_rocksdb_cache.cpp
      src/cache/cache_entry_codec.cpp
      src/cache/l1_cache.cpp
      src/cache/cache_entry.cpp
      src/cache/cache_key.cpp
      src/cache/cache_ttl.cpp
    )
    target_include_directories(l2_promotion_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(l2_promotion_test
      PRIVATE
        gtest_main
        RocksDB::RocksDB
    )

    add_test(
      NAME l2_promotion_test
      COMMAND l2_promotion_test
    )

    add_executable(json_transform_content_type_test
      tests/json_transform_content_type_test.cpp
      src/json_transform/content_type.cpp
    )
    target_include_directories(json_transform_content_type_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(json_transform_content_type_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME json_transform_content_type_test
      COMMAND json_transform_content_type_test
    )

    add_executable(json_transform_flat_json_test
      tests/json_transform_flat_json_test.cpp
      src/json_transform/flat_json.cpp
    )
    target_include_directories(json_transform_flat_json_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(json_transform_flat_json_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME json_transform_flat_json_test
      COMMAND json_transform_flat_json_test
    )

    add_executable(json_transform_filter_flat_json_test
      tests/json_transform_filter_flat_json_test.cpp
      src/json_transform/flat_json.cpp
      src/json_transform/filter_flat_json.cpp
    )
    target_include_directories(json_transform_filter_flat_json_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(json_transform_filter_flat_json_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME json_transform_filter_flat_json_test
      COMMAND json_transform_filter_flat_json_test
    )

    add_executable(policy_route_policy_test
      tests/policy_route_policy_test.cpp
    )
    target_include_directories(policy_route_policy_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(policy_route_policy_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME policy_route_policy_test
      COMMAND policy_route_policy_test
    )


    add_executable(policy_yaml_loader_test
      tests/policy_yaml_loader_test.cpp
    )
    target_include_directories(policy_yaml_loader_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(policy_yaml_loader_test
      PRIVATE
        policy_yaml_loader
        gtest_main
    )

    add_test(
      NAME policy_yaml_loader_test
      COMMAND policy_yaml_loader_test
    )

    add_executable(policy_route_matcher_test
      tests/policy_route_matcher_test.cpp
    )
    target_include_directories(policy_route_matcher_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(policy_route_matcher_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME policy_route_matcher_test
      COMMAND policy_route_matcher_test
    )

    add_executable(policy_method_matcher_test
      tests/policy_method_matcher_test.cpp
    )
    target_include_directories(policy_method_matcher_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(policy_method_matcher_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME policy_method_matcher_test
      COMMAND policy_method_matcher_test
    )

    add_executable(policy_yaml_loader_method_test
      tests/policy_yaml_loader_method_test.cpp
    )
    target_include_directories(policy_yaml_loader_method_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(policy_yaml_loader_method_test
      PRIVATE
        policy_yaml_loader
        gtest_main
    )

    add_test(
      NAME policy_yaml_loader_method_test
      COMMAND policy_yaml_loader_method_test
    )

    add_executable(policy_field_filter_test
      tests/policy_field_filter_test.cpp
    )
    target_include_directories(policy_field_filter_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(policy_field_filter_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME policy_field_filter_test
      COMMAND policy_field_filter_test
    )

    add_executable(policy_yaml_loader_field_filter_test
      tests/policy_yaml_loader_field_filter_test.cpp
    )
    target_include_directories(policy_yaml_loader_field_filter_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(policy_yaml_loader_field_filter_test
      PRIVATE
        policy_yaml_loader
        gtest_main
    )

    add_test(
      NAME policy_yaml_loader_field_filter_test
      COMMAND policy_yaml_loader_field_filter_test
    )

    add_executable(policy_max_response_bytes_test
      tests/policy_max_response_bytes_test.cpp
    )
    target_include_directories(policy_max_response_bytes_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(policy_max_response_bytes_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME policy_max_response_bytes_test
      COMMAND policy_max_response_bytes_test
    )

    add_executable(policy_yaml_loader_max_bytes_test
      tests/policy_yaml_loader_max_bytes_test.cpp
    )
    target_include_directories(policy_yaml_loader_max_bytes_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(policy_yaml_loader_max_bytes_test
      PRIVATE
        policy_yaml_loader
        gtest_main
    )

    add_test(
      NAME policy_yaml_loader_max_bytes_test
      COMMAND policy_yaml_loader_max_bytes_test
    )

    add_executable(policy_cache_policy_test
      tests/policy_cache_policy_test.cpp
      src/policy/cache_policy.cpp
    )
    target_include_directories(policy_cache_policy_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(policy_cache_policy_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME policy_cache_policy_test
      COMMAND policy_cache_policy_test
    )

    add_executable(policy_yaml_loader_cache_test
      tests/policy_yaml_loader_cache_test.cpp
    )
    target_include_directories(policy_yaml_loader_cache_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(policy_yaml_loader_cache_test
      PRIVATE
        policy_yaml_loader
        gtest_main
    )

    add_test(
      NAME policy_yaml_loader_cache_test
      COMMAND policy_yaml_loader_cache_test
    )

    add_executable(policy_validate_command_test
      tests/policy_validate_command_test.cpp
    )
    target_include_directories(policy_validate_command_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(policy_validate_command_test
      PRIVATE
        policy_yaml_loader
        gtest_main
    )

    add_test(
      NAME policy_validate_command_test
      COMMAND policy_validate_command_test
    )

    add_test(
      NAME policy_validate_command_example_smoke
      COMMAND bytetaper-validate-policy
              ${CMAKE_CURRENT_SOURCE_DIR}/examples/policy/bytetaper-policy.yaml
    )

    add_executable(policy_report_output_test
      tests/policy_report_output_test.cpp
    )
    target_include_directories(policy_report_output_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(policy_report_output_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME policy_report_output_test
      COMMAND policy_report_output_test
    )

    add_executable(metrics_prometheus_registry_test
      tests/metrics_prometheus_registry_test.cpp
      src/metrics/prometheus_registry.cpp
    )
    target_include_directories(metrics_prometheus_registry_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(metrics_prometheus_registry_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME metrics_prometheus_registry_test
      COMMAND metrics_prometheus_registry_test
    )

    add_executable(safety_fail_open_test
      tests/safety_fail_open_test.cpp
      src/safety/fail_open.cpp
    )
    target_include_directories(safety_fail_open_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(safety_fail_open_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME safety_fail_open_test
      COMMAND safety_fail_open_test
    )

    add_executable(cache_entry_test
      tests/cache_entry_test.cpp
      src/cache/cache_entry.cpp
    )
    target_include_directories(cache_entry_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(cache_entry_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME cache_entry_test
      COMMAND cache_entry_test
    )

    add_executable(cache_key_test
      tests/cache_key_test.cpp
      src/cache/cache_key.cpp
    )
    target_include_directories(cache_key_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(cache_key_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME cache_key_test
      COMMAND cache_key_test
    )

    add_executable(l2_rocksdb_cache_test
      tests/l2_rocksdb_cache_test.cpp
      src/cache/l2_rocksdb_cache.cpp
      src/cache/cache_entry_codec.cpp
      src/cache/cache_entry.cpp
      src/cache/cache_ttl.cpp
    )
    target_include_directories(l2_rocksdb_cache_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(l2_rocksdb_cache_test
      PRIVATE
        gtest_main
        RocksDB::RocksDB
    )

    add_test(
      NAME l2_rocksdb_cache_test
      COMMAND l2_rocksdb_cache_test
    )

    add_executable(cache_ttl_test
      tests/cache_ttl_test.cpp
      src/cache/cache_ttl.cpp
    )
    target_include_directories(cache_ttl_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(cache_ttl_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME cache_ttl_test
      COMMAND cache_ttl_test
    )

    add_executable(cache_auth_safety_test
      tests/cache_auth_safety_test.cpp
      src/cache/cache_safety.cpp
    )
    target_include_directories(cache_auth_safety_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(cache_auth_safety_test
      PRIVATE
        gtest_main
    )

    add_test(
      NAME cache_auth_safety_test
      COMMAND cache_auth_safety_test
    )
  endif()

  if(BYTETAPER_ENABLE_INTEGRATION_TESTS)
    add_executable(extproc_server_smoke_test
      tests/integration/extproc/extproc_server_smoke_test.cpp
    )
    target_include_directories(extproc_server_smoke_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${BYTETAPER_GENERATED_PROTO_DIR}
    )
    target_link_libraries(extproc_server_smoke_test
      PRIVATE
        bytetaper_extproc_grpc_server
        bytetaper_extproc_grpc
    )

    add_test(
      NAME extproc_server_smoke_test
      COMMAND extproc_server_smoke_test
    )

    add_executable(extproc_externalprocessor_service_integration_test
      tests/integration/extproc/externalprocessor_service_integration_test.cpp
    )
    target_include_directories(extproc_externalprocessor_service_integration_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${BYTETAPER_GENERATED_PROTO_DIR}
    )
    target_link_libraries(extproc_externalprocessor_service_integration_test
      PRIVATE
        bytetaper_extproc_grpc_server
        bytetaper_extproc_grpc
    )

    add_test(
      NAME extproc_externalprocessor_service_integration_test
      COMMAND extproc_externalprocessor_service_integration_test
    )

    add_executable(extproc_original_bytes_integration_test
      tests/integration/extproc/extproc_original_bytes_integration_test.cpp
    )
    target_include_directories(extproc_original_bytes_integration_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${BYTETAPER_GENERATED_PROTO_DIR}
    )
    target_link_libraries(extproc_original_bytes_integration_test
      PRIVATE
        bytetaper_extproc_grpc_server
        bytetaper_extproc_grpc
    )

    add_test(
      NAME extproc_original_bytes_integration_test
      COMMAND extproc_original_bytes_integration_test
    )

    add_executable(extproc_optimized_bytes_integration_test
      tests/integration/extproc/extproc_optimized_bytes_integration_test.cpp
    )
    target_include_directories(extproc_optimized_bytes_integration_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${BYTETAPER_GENERATED_PROTO_DIR}
    )
    target_link_libraries(extproc_optimized_bytes_integration_test
      PRIVATE
        bytetaper_extproc_grpc_server
        bytetaper_extproc_grpc
    )

    add_test(
      NAME extproc_optimized_bytes_integration_test
      COMMAND extproc_optimized_bytes_integration_test
    )

    add_executable(extproc_original_bytes_header_integration_test
      tests/integration/extproc/extproc_original_bytes_header_integration_test.cpp
    )
    target_include_directories(extproc_original_bytes_header_integration_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${BYTETAPER_GENERATED_PROTO_DIR}
    )
    target_link_libraries(extproc_original_bytes_header_integration_test
      PRIVATE
        bytetaper_extproc_grpc_server
        bytetaper_extproc_grpc
    )

    add_test(
      NAME extproc_original_bytes_header_integration_test
      COMMAND extproc_original_bytes_header_integration_test
    )

    add_executable(extproc_optimized_bytes_header_integration_test
      tests/integration/extproc/extproc_optimized_bytes_header_integration_test.cpp
    )
    target_include_directories(extproc_optimized_bytes_header_integration_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${BYTETAPER_GENERATED_PROTO_DIR}
    )
    target_link_libraries(extproc_optimized_bytes_header_integration_test
      PRIVATE
        bytetaper_extproc_grpc_server
        bytetaper_extproc_grpc
    )

    add_test(
      NAME extproc_optimized_bytes_header_integration_test
      COMMAND extproc_optimized_bytes_header_integration_test
    )

    add_executable(extproc_transform_applied_integration_test
      tests/integration/extproc/extproc_transform_applied_integration_test.cpp
    )
    target_include_directories(extproc_transform_applied_integration_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${BYTETAPER_GENERATED_PROTO_DIR}
    )
    target_link_libraries(extproc_transform_applied_integration_test
      PRIVATE
        bytetaper_extproc_grpc_server
        bytetaper_extproc_grpc
    )

    add_test(
      NAME extproc_transform_applied_integration_test
      COMMAND extproc_transform_applied_integration_test
    )

    add_executable(extproc_route_policy_integration_test
      tests/integration/extproc/extproc_route_policy_integration_test.cpp
    )
    target_include_directories(extproc_route_policy_integration_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${BYTETAPER_GENERATED_PROTO_DIR}
    )
    target_link_libraries(extproc_route_policy_integration_test
      PRIVATE
        bytetaper_extproc_grpc_server
        bytetaper_extproc_grpc
    )

    add_test(
      NAME extproc_route_policy_integration_test
      COMMAND extproc_route_policy_integration_test
    )

    add_executable(extproc_metrics_endpoint_integration_test
      tests/integration/extproc/extproc_metrics_endpoint_integration_test.cpp
    )
    target_include_directories(extproc_metrics_endpoint_integration_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${BYTETAPER_GENERATED_PROTO_DIR}
    )
    target_link_libraries(extproc_metrics_endpoint_integration_test
      PRIVATE
        bytetaper_extproc_grpc_server
        bytetaper_extproc_grpc
        bytetaper_metrics_http_server
        bytetaper_prometheus_registry
    )

    add_test(
      NAME extproc_metrics_endpoint_integration_test
      COMMAND extproc_metrics_endpoint_integration_test
    )

    add_executable(extproc_fail_closed_integration_test
      tests/integration/extproc/extproc_fail_closed_integration_test.cpp
    )
    target_include_directories(extproc_fail_closed_integration_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${BYTETAPER_GENERATED_PROTO_DIR}
    )
    target_link_libraries(extproc_fail_closed_integration_test
      PRIVATE
        bytetaper_extproc_grpc_server
        bytetaper_extproc_grpc
    )

    add_test(
      NAME extproc_fail_closed_integration_test
      COMMAND extproc_fail_closed_integration_test
    )

    add_executable(extproc_payload_too_large_integration_test
      tests/integration/extproc/extproc_payload_too_large_integration_test.cpp
    )
    target_include_directories(extproc_payload_too_large_integration_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${BYTETAPER_GENERATED_PROTO_DIR}
    )
    target_link_libraries(extproc_payload_too_large_integration_test
      PRIVATE
        bytetaper_extproc_grpc_server
        bytetaper_extproc_grpc
    )

    add_test(
      NAME extproc_payload_too_large_integration_test
      COMMAND extproc_payload_too_large_integration_test
    )

    add_executable(extproc_non_2xx_integration_test
      tests/integration/extproc/extproc_non_2xx_integration_test.cpp
    )
    target_include_directories(extproc_non_2xx_integration_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${BYTETAPER_GENERATED_PROTO_DIR}
    )
    target_link_libraries(extproc_non_2xx_integration_test
      PRIVATE
        bytetaper_extproc_grpc_server
        bytetaper_extproc_grpc
    )

    add_test(
      NAME extproc_non_2xx_integration_test
      COMMAND extproc_non_2xx_integration_test
    )

    add_executable(extproc_non_json_integration_test
      tests/integration/extproc/extproc_non_json_integration_test.cpp
    )
    target_include_directories(extproc_non_json_integration_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${BYTETAPER_GENERATED_PROTO_DIR}
    )
    target_link_libraries(extproc_non_json_integration_test
      PRIVATE
        bytetaper_extproc_grpc_server
        bytetaper_extproc_grpc
    )

    add_test(
      NAME extproc_non_json_integration_test
      COMMAND extproc_non_json_integration_test
    )

    add_executable(extproc_malformed_input_integration_test
      tests/integration/extproc/extproc_malformed_input_integration_test.cpp
    )
    target_include_directories(extproc_malformed_input_integration_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${BYTETAPER_GENERATED_PROTO_DIR}
    )
    target_link_libraries(extproc_malformed_input_integration_test
      PRIVATE
        bytetaper_extproc_grpc_server
        bytetaper_extproc_grpc
    )

    add_test(
      NAME extproc_malformed_input_integration_test
      COMMAND extproc_malformed_input_integration_test
    )

    add_executable(extproc_invalid_policy_integration_test
      tests/integration/extproc/extproc_invalid_policy_integration_test.cpp
    )
    target_include_directories(extproc_invalid_policy_integration_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${BYTETAPER_GENERATED_PROTO_DIR}
    )
    target_link_libraries(extproc_invalid_policy_integration_test
      PRIVATE
        bytetaper_extproc_grpc_server
        bytetaper_extproc_grpc
    )

    add_test(
      NAME extproc_invalid_policy_integration_test
      COMMAND extproc_invalid_policy_integration_test
    )

    add_executable(extproc_observe_mode_integration_test
      tests/integration/extproc/extproc_observe_mode_integration_test.cpp
    )
    target_include_directories(extproc_observe_mode_integration_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${BYTETAPER_GENERATED_PROTO_DIR}
    )
    target_link_libraries(extproc_observe_mode_integration_test
      PRIVATE
        bytetaper_extproc_grpc_server
        bytetaper_extproc_grpc
    )

    add_test(
      NAME extproc_observe_mode_integration_test
      COMMAND extproc_observe_mode_integration_test
    )

    add_executable(extproc_policy_not_found_integration_test
      tests/integration/extproc/extproc_policy_not_found_integration_test.cpp
    )
    target_include_directories(extproc_policy_not_found_integration_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${BYTETAPER_GENERATED_PROTO_DIR}
    )
    target_link_libraries(extproc_policy_not_found_integration_test
      PRIVATE
        bytetaper_extproc_grpc_server
        bytetaper_extproc_grpc
    )

    add_test(
      NAME extproc_policy_not_found_integration_test
      COMMAND extproc_policy_not_found_integration_test
    )
  endif()
endif()
