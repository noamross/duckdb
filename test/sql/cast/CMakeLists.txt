
add_library(test_sql_cast OBJECT test_boolean_cast.cpp)
set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:test_sql_cast> PARENT_SCOPE)
