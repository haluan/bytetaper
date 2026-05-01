cmake_minimum_required(VERSION 3.22)

project(ByteTaper LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(BYTETAPER_ENABLE_GTEST_TESTS "Enable GoogleTest-based unit tests" OFF)

add_compile_options(
  -fno-exceptions
  -fno-rtti
)

add_executable(bytetaper-extproc
  src/main.cpp
)

include(CTest)
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
  endif()
endif()
