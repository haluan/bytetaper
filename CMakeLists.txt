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

  set(BYTETAPER_PROTO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/proto")
  set(BYTETAPER_GENERATED_PROTO_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/extproc")

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
  endif()

  if(BYTETAPER_ENABLE_INTEGRATION_TESTS)
    add_executable(extproc_protobuf_generation_integration_test
      tests/integration/extproc/protobuf_generation_integration_test.cpp
    )
    target_include_directories(extproc_protobuf_generation_integration_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(extproc_protobuf_generation_integration_test
      PRIVATE
        bytetaper_extproc_proto_boundary
    )

    add_test(
      NAME extproc_protobuf_generation_integration_test
      COMMAND extproc_protobuf_generation_integration_test
    )
  endif()
endif()
