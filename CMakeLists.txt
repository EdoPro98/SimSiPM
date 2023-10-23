cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_FLAGS_INIT -ffast-math)
project(SimSiPM VERSION 1 DESCRIPTION "Library for SiPM simulation" LANGUAGES CXX)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CMakeDependentOption)
include(FeatureSummary)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
    "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
    FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set(SIPM_BUILD_PYTHON OFF CACHE BOOL "Compile python bindings for SiPM simulation library")
set(SIPM_ENABLE_TEST OFF CACHE BOOL "Build tests for SiPM simulation library")
set(SIPM_ENABLE_BENCH OFF CACHE BOOL "Build benchmarks for SiPM simulation library")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	if(SIPM_ENABLE_TEST)
	    add_subdirectory(tests)
	endif(SIPM_ENABLE_TEST)
endif ()

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  if(SIPM_ENABLE_BENCH)
	    add_subdirectory(bench)
    endif(SIPM_ENABLE_BENCH)
endif ()



# Get files
file(GLOB_RECURSE src "src/*.cpp")
file(GLOB_RECURSE include "include/*.h")
file(GLOB_RECURSE pysrc "python/*.cpp")

# Library
add_library(sipm SHARED
  ${src}
  ${include}
)

# Include files
target_include_directories(sipm PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

set_target_properties(sipm PROPERTIES VERSION 1 OUTPUT_NAME sipm)
set_property(TARGET sipm PROPERTY PUBLIC_HEADER ${include})

feature_summary(WHAT ALL)

# Install
install(TARGETS sipm EXPORT sipm-config
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sipm
)

install(EXPORT sipm-config
	FILE sipm-config.cmake
	DESTINATION "${CMAKE_INSTALL_DATADIR}/sipm/cmake"
	NAMESPACE sipm::
)

if (SIPM_BUILD_PYTHON)
	find_package(Python COMPONENTS Interpreter Development REQUIRED)
	find_package(pybind11 CONFIG REQUIRED)
	pybind11_add_module(SiPM MODULE ${pysrc} ${src} ${include})
	target_include_directories(SiPM PRIVATE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	)
	set_property(TARGET SiPM PROPERTY CXX_STANDARD 17)
  target_compile_options(SiPM PRIVATE -fvisibility=hidden -ffast-math -O3)

	install(TARGETS SiPM
	LIBRARY DESTINATION lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages
	)
endif (SIPM_BUILD_PYTHON)
