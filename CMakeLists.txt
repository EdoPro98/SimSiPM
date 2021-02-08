cmake_minimum_required(VERSION 3.9)
project(SimSiPM VERSION 1 DESCRIPTION "Library for SiPM simulation" LANGUAGES CXX)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 11 CACHE STRING "Set C++ standard")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)

# Optimizations
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

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE CACHE BOOL "Turn on interprocedural optimizations")


# Get files
file(GLOB_RECURSE sources "src/*.cpp")
file(GLOB_RECURSE headers "src/*.h")
file(GLOB_RECURSE pythonsources "python/*.cpp")

file(GLOB_RECURSE public_headers "SimSiPM/*.h")
file(GLOB_RECURSE private_headers "src/*.h")


# Library
add_library(sipm SHARED
	${sources}
	${headers}
)


# Include files
target_include_directories(sipm PUBLIC SimSiPM/)
target_include_directories(sipm PRIVATE src/components/)

set_property(TARGET sipm PROPERTY PUBLIC_HEADER SimSiPM/SiPM.h)
set_property(TARGET sipm PROPERTY PUBLIC_HEADER ${public_headers})
set_property(TARGET sipm PROPERTY PRIVATE_HEADER ${private_headers})
set_property(TARGET sipm PROPERTY VERSION 1)

set(INSTALL_PYTHON_BINDINGS OFF CACHE BOOL "Install python bindings")

if(INSTALL_PYTHON_BINDINGS)
	set(CMAKE_CXX_STANDARD 14)
	find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
	find_package(pybind11 CONFIG REQUIRED)
	include_directories("SimSiPM/" "src/components")
	pybind11_add_module(SiPM SHARED ${pythonsources} ${sources})
endif(INSTALL_PYTHON_BINDINGS)

# Install
install(TARGETS sipm
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    PRIVATE_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
