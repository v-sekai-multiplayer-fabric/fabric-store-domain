# The store domain, and the game that is its tenant.
#
# `thirdparty/store-plane` is the engine: SQLite over a VFS whose pages live in
# FoundationDB. The game links it and adds no storage of its own, because the whole point is
# that the database is the game.
#
# Vendored as a subtree rather than fetched, for the reason the harness is: a repository that
# cannot build without cloning a second one is a note, not a repository.
cmake_minimum_required(VERSION 3.22)
project(fabric_store_domain C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)
find_path(FDB_INCLUDE_DIR foundationdb/fdb_c.h REQUIRED)
find_library(FDB_LIBRARY NAMES fdb_c REQUIRED)
find_path(SQLITE_INCLUDE_DIR sqlite3.h REQUIRED)
find_library(SQLITE_LIBRARY NAMES sqlite3 REQUIRED)

add_library(weft_fdb_vfs STATIC thirdparty/store-plane/fdb_vfs.c)
target_include_directories(weft_fdb_vfs PUBLIC
  ${FDB_INCLUDE_DIR}/foundationdb ${SQLITE_INCLUDE_DIR} thirdparty/store-plane)
target_compile_definitions(weft_fdb_vfs PUBLIC FDB_API_VERSION=730)
target_link_libraries(weft_fdb_vfs PUBLIC ${FDB_LIBRARY} ${SQLITE_LIBRARY} Threads::Threads)

# `thirdparty/taskweft` is the planner: `v-sekai-multiplayer-fabric/nif`'s `standalone/`
# headers, which are the HTN planner with no Elixir and no NIF in it. Header-only C++20, so
# `queen` links headers rather than a runtime, and `src/planner.cpp` is the only translation
# unit that sees them.
add_executable(queen src/queen.c src/planner.cpp)
target_include_directories(queen PRIVATE src thirdparty/taskweft)
target_link_libraries(queen PRIVATE weft_fdb_vfs)
target_compile_options(queen PRIVATE -Wall -Wextra)
set_target_properties(queen PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
