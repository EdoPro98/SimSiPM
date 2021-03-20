cmake_minimum_required(VERSION 3.9)
project(SimSiPM VERSION 1 DESCRIPTION "Library for SiPM simulation" LANGUAGES CXX)
include(GNUInstallDirs)

set(COMPILE_PYTHON_BINDINGS OFF CACHE BOOL "Compile python bindings")

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
target_include_directories(sipm PUBLIC "include")
set_property(TARGET sipm PROPERTY VERSION 1)
set_property(TARGET sipm PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
set_property(TARGET sipm PROPERTY CXX_STANDARD 17)
set_target_properties(sipm PROPERTIES PUBLIC_HEADER "${include}")

# Install
install(TARGETS sipm
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if (COMPILE_PYTHON_BINDINGS)
	find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
	find_package(pybind11 CONFIG REQUIRED)
	pybind11_add_module(SiPM SHARED ${pysrc} ${src})
	target_include_directories(SiPM PUBLIC "include/")
	set_property(TARGET SiPM PROPERTY CXX_STANDARD 17)


	execute_process(
  COMMAND "${PYTHON_EXECUTABLE}" -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())"
  OUTPUT_VARIABLE PYTHON_INSTALL_SITE
  OUTPUT_STRIP_TRAILING_WHITESPACE)

	install(TARGETS SiPM
	LIBRARY DESTINATION ${Python3_SITEARCH}
	)
endif (COMPILE_PYTHON_BINDINGS)
