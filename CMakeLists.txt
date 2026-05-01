cmake_minimum_required(VERSION 3.22)

project(ByteTaper LANGUAGES CXX)

include(FetchContent)
include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(BYTETAPER_ENABLE_GTEST_TESTS "Enable GoogleTest-based unit tests" OFF)
option(BYTETAPER_ENABLE_INTEGRATION_TESTS "Enable integration-style extproc adapter tests" OFF)

add_compile_options(
  -fno-exceptions
  -fno-rtti
)

add_executable(bytetaper-extproc
  src/main.cpp
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

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(YAML_CPP REQUIRED yaml-cpp)

    add_library(policy_yaml_loader STATIC
      src/policy/yaml_loader.cpp
    )
    set_source_files_properties(src/policy/yaml_loader.cpp
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
  endif()
endif()
