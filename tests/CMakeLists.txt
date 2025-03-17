#find_package(GTest)

if(NOT GTest_FOUND)
	include(FetchContent)
	FetchContent_Declare(
  		googletest
  		GIT_REPOSITORY https://github.com/google/googletest.git
  		GIT_TAG origin/main
	)
	FetchContent_MakeAvailable(googletest)
endif()

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

enable_testing()

add_executable(TestSiPMRng xorshift.cpp)
add_executable(TestSiPMRandom rand.cpp)
add_executable(TestSiPMProperties properties.cpp)
add_executable(TestSiPMSensor sensor.cpp)

target_link_libraries(TestSiPMRng GTest::gtest_main sipm)
target_link_libraries(TestSiPMRandom GTest::gtest_main sipm)
target_link_libraries(TestSiPMProperties GTest::gtest_main sipm)
target_link_libraries(TestSiPMSensor GTest::gtest_main sipm)

include(GoogleTest)
include_directories(../include)
gtest_discover_tests(TestSiPMRng)
gtest_discover_tests(TestSiPMRandom)
gtest_discover_tests(TestSiPMProperties)
gtest_discover_tests(TestSiPMSensor)
