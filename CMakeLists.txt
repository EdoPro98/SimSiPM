cmake_minimum_required(VERSION 3.9)
project(SimSiPM VERSION 1.0.0 DESCRIPTION "Library for SiPM simulation" LANGUAGES CXX)
include(GNUInstallDirs)

# CXX standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11 CACHE STRING "C++ standard to be used")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
	message(STATUS "No default build type specified: setting CMAKE_BUILD_TYPE=Release")
	set(CMAKE_BUILD_TYPE Release CACHE STRING
		"Choose the build type: options are: Debug Release RelWithDebInfo MinSizeRel"
		FORCE)
else(NOT CMAKE_BUILD_TYPE)
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		message("====================================================================================")
		message(STATUS "Build type is set to Debug: Performance will be negatively impacted")
		message(STATUS "Add -DCMAKE_BUILD_TYPE=Release to the CMake command line to get an optimized build")
		message("====================================================================================")
	endif(CMAKE_BUILD_TYPE STREQUAL "Debug")
endif(NOT CMAKE_BUILD_TYPE)


# Library
add_library(sipm SHARED
  src/components/SiPMRandom.cpp
  src/lib/SiPMHit.cpp
  src/lib/SiPMProperties.cpp
  src/lib/SiPMAnalogSignal.cpp
  src/lib/SiPMDigitalSignal.cpp
  src/lib/SiPMSensor.cpp
  src/lib/SiPMAdc.cpp
)


set_target_properties(sipm PROPERTIES VERSION ${PROJECT_VERSION} PUBLIC_HEADER SimSiPM/SiPM.h)
set_target_properties(sipm PROPERTIES VERSION ${PROJECT_VERSION} PUBLIC_HEADER "SimSiPM/SiPM.h;SimSiPM/SiPMAdc.h;SimSiPM/SiPMAnalogSignal.h;SimSiPM/SiPMDigitalSignal.h;SimSiPM/SiPMDebugInfo.h;SimSiPM/SiPMHit.h;SimSiPM/SiPMProperties.h;SimSiPM/SiPMSensor.h")
set_target_properties(sipm PROPERTIES VERSION ${PROJECT_VERSION} PRIVATE_HEADER "src/components/SiPMHelpers.h;src/components/SiPMRandom.h")

# Include files
target_include_directories(sipm PUBLIC SimSiPM/)
target_include_directories(sipm PRIVATE src/components/)

# Install
install(TARGETS sipm
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    PRIVATE_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

add_subdirectory(tests)
enable_testing()


add_test(Uniform tests/Rand)
add_test(Exponential tests/Exp)
add_test(Integer tests/Integer)
add_test(Gaussian tests/Gaussian)
add_test(Poisson tests/Poisson)
add_test(Xorshift256 tests/Xorshift256)
