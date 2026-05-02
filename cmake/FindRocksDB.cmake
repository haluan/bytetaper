# FindRocksDB.cmake
#
# Finds the RocksDB library.
#
# This will define the following variables:
#  RocksDB_FOUND        - True if RocksDB was found.
#  RocksDB_INCLUDE_DIRS - Include directories for RocksDB.
#  RocksDB_LIBRARIES    - Libraries for RocksDB.
#  RocksDB_VERSION      - The version of RocksDB found.
#
# And the following imported targets:
#  RocksDB::RocksDB

find_path(RocksDB_INCLUDE_DIR
  NAMES rocksdb/db.h
  PATH_SUFFIXES include
)

find_library(RocksDB_LIBRARY
  NAMES rocksdb
)

if(RocksDB_INCLUDE_DIR AND EXISTS "${RocksDB_INCLUDE_DIR}/rocksdb/version.h")
  file(STRINGS "${RocksDB_INCLUDE_DIR}/rocksdb/version.h" ROCKSDB_MAJOR_VERSION_STR REGEX "^#define ROCKSDB_MAJOR [0-9]+$")
  file(STRINGS "${RocksDB_INCLUDE_DIR}/rocksdb/version.h" ROCKSDB_MINOR_VERSION_STR REGEX "^#define ROCKSDB_MINOR [0-9]+$")
  file(STRINGS "${RocksDB_INCLUDE_DIR}/rocksdb/version.h" ROCKSDB_PATCH_VERSION_STR REGEX "^#define ROCKSDB_PATCH [0-9]+$")

  string(REGEX REPLACE "^#define ROCKSDB_MAJOR ([0-9]+)$" "\\1" RocksDB_VERSION_MAJOR "${ROCKSDB_MAJOR_VERSION_STR}")
  string(REGEX REPLACE "^#define ROCKSDB_MINOR ([0-9]+)$" "\\1" RocksDB_VERSION_MINOR "${ROCKSDB_MINOR_VERSION_STR}")
  string(REGEX REPLACE "^#define ROCKSDB_PATCH ([0-9]+)$" "\\1" RocksDB_VERSION_PATCH "${ROCKSDB_PATCH_VERSION_STR}")

  set(RocksDB_VERSION "${RocksDB_VERSION_MAJOR}.${RocksDB_VERSION_MINOR}.${RocksDB_VERSION_PATCH}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RocksDB
  REQUIRED_VARS RocksDB_LIBRARY RocksDB_INCLUDE_DIR
  VERSION_VAR RocksDB_VERSION
)

if(RocksDB_FOUND)
  set(RocksDB_LIBRARIES ${RocksDB_LIBRARY})
  set(RocksDB_INCLUDE_DIRS ${RocksDB_INCLUDE_DIR})

  if(NOT TARGET RocksDB::RocksDB)
    add_library(RocksDB::RocksDB UNKNOWN IMPORTED)
    set_target_properties(RocksDB::RocksDB PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${RocksDB_INCLUDE_DIRS}"
      IMPORTED_LOCATION "${RocksDB_LIBRARIES}"
    )
    
    # RocksDB usually needs these dependencies
    find_package(Threads REQUIRED)
    set_target_properties(RocksDB::RocksDB PROPERTIES
      INTERFACE_LINK_LIBRARIES "Threads::Threads"
    )
  endif()
endif()

mark_as_advanced(RocksDB_INCLUDE_DIR RocksDB_LIBRARY)
