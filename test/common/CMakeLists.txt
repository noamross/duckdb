add_library(test_common OBJECT test_ops.cpp test_date.cpp test_file_system.cpp test_hyperloglog.cpp test_sort.cpp test_utf.cpp) # test_serializer.cpp
set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:test_common> PARENT_SCOPE)
