SET(DUCKDB_TEST_HELPERS_UNITS expression_helper.cpp test_helpers.cpp)

add_library(test_helpers STATIC ${DUCKDB_TEST_HELPERS_UNITS})

IF (NOT WIN32)
  target_link_libraries(test_helpers duckdb)
ENDIF()