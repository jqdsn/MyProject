add_executable(myproject_tests
  test_units.cpp
  test_integration.cpp
  test_perf.cpp
)
target_link_libraries(myproject_tests PRIVATE gtest  gtest_main core ${OpenCV_LIBS})
add_test(NAME MyProjectTests COMMAND myproject_tests)
