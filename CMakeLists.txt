cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_FLAGS_INIT "-ffast-math")
project(SimSiPM VERSION 1 DESCRIPTION "Library for SiPM simulation" LANGUAGES CXX)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CMakeDependentOption)
include(FeatureSummary)

set(SIPM_BUILD_PYTHON OFF CACHE BOOL "Compile python bindings for SiPM simulation library")
set(SIPM_ENABLE_TEST OFF CACHE BOOL "Build tests for SiPM simulation library")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	if(SIPM_ENABLE_TEST)
	    enable_testing()
	    include(GoogleTest)
	    add_subdirectory(tests)
	endif(SIPM_ENABLE_TEST)
endif ()

# Optimizations
if(NOT CMAKE_BUILD_TYPE)
	message(STATUS "No default build type specified: setting CMAKE_BUILD_TYPE=Release")
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the build type: options are: Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif(NOT CMAKE_BUILD_TYPE)

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
target_include_directories(sipm PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

set_property(TARGET sipm PROPERTY VERSION 1)
set_property(TARGET sipm PROPERTY PUBLIC_HEADER "${include}")
set_property(TARGET sipm PROPERTY OUTPUT_NAME sipm)

feature_summary(WHAT ALL)

# Install
install(TARGETS sipm EXPORT sipm-config
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT sipm-config
	FILE sipm-config.cmake
	DESTINATION "${CMAKE_INSTALL_DATADIR}/sipm/cmake"
	NAMESPACE sipm::
)

if (SIPM_BUILD_PYTHON)
	find_package(Python COMPONENTS Interpreter Development REQUIRED)
	find_package(nanobind CONFIG REQUIRED)
	nanobind_add_module(SiPM NB_STATIC ${pysrc} ${src} ${include})
  target_compile_options(SiPM PRIVATE -fvisibility=hidden)

	install(TARGETS SiPM
	LIBRARY DESTINATION lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages
	)
endif (SIPM_BUILD_PYTHON)
