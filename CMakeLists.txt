cmake_minimum_required(VERSION 3.22)

project(ByteTaper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
endif()
