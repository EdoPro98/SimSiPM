enable_testing()

macro(package_add_test_with_libraries TESTNAME FILES LIBRARIES TEST_WORKING_DIRECTORY)
    add_executable(${TESTNAME} ${FILES})
    set_target_properties(${TESTNAME} PROPERTIES COMPILE_FLAGS "-O2 -g")
    target_link_libraries(${TESTNAME} gtest gmock gtest_main ${LIBRARIES})
    gtest_discover_tests(${TESTNAME}
        WORKING_DIRECTORY ${TEST_WORKING_DIRECTORY}
        PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
    )
    set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
endmacro()

find_package(GTest)

if(NOT GTest_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
  )
  FetchContent_GetProperties(googletest)
  if(NOT googletest_POPULATED)
    FetchContent_Populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
  endif()
endif()

include(GoogleTest)
package_add_test_with_libraries(TestSiPMXorshift256plus xorshift.cpp sipm "${PROJECT_DIR}")
package_add_test_with_libraries(TestSiPMRandom rand.cpp sipm "${PROJECT_DIR}")
package_add_test_with_libraries(TestSiPMProperities properties.cpp sipm "${PROJECT_DIR}")
package_add_test_with_libraries(TestSiPMSensor sensor.cpp sipm "${PROJECT_DIR}")
