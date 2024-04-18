include(CTest)
include(Catch)

add_executable(serialization_tests serialization_tests.cpp)
target_link_libraries(serialization_tests  PRIVATE Catch2::Catch2WithMain)

catch_discover_tests(serialization_tests)
