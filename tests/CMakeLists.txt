add_executable(myproject_tests
  test_units.cpp
  test_integration.cpp
  test_perf.cpp
)
target_link_libraries(myproject_tests PRIVATE gtest  gtest_main core ${OpenCV_LIBS})

foreach(opencv_dll ${OpenCV_LIBS})
    if(EXISTS "${opencv_dll}")
        add_custom_command(TARGET myproject_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${opencv_dll}"
                $<TARGET_FILE_DIR:myproject_tests>
        )
    endif()
endforeach()

add_test(NAME MyProjectTests COMMAND myproject_tests)
