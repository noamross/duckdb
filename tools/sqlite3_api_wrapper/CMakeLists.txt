
include_directories(include)

add_library(sqlite3_api_wrapper SHARED sqlite3_api_wrapper.c)
link_directories(../../build/src/Debug)
target_link_libraries(sqlite3_api_wrapper duckdb_static)
