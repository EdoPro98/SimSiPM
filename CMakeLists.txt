cmake_minimum_required(VERSION 3.9)
project(SimSiPM VERSION 1 DESCRIPTION "Library for SiPM simulation" LANGUAGES CXX)
include(GNUInstallDirs)

set(SIPM_BUILD_PYTHON OFF CACHE BOOL "Compile python bindings for SiPM simulation library")
set(SIPM_ENABLE_OPENMP OFF CACHE BOOL "Use OpenMP in some parts of SiPM simulation library")
set(SIPM_ENABLE_TEST OFF CACHE BOOL "Build tests for SiPM simulation library")
set(SIPM_ENABLE_LTO OFF CACHE BOOL "Build SiPM with link time optimizations enabled")

if(SIPM_BUILD_PYTHON)
	find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
	find_package(pybind11 CONFIG REQUIRED)
endif(SIPM_BUILD_PYTHON)

if (SIPM_ENABLE_OPENMP)
	find_package(OpenMP REQUIRED)
endif(SIPM_ENABLE_OPENMP)

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
target_include_directories(sipm PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

set_property(TARGET sipm PROPERTY VERSION 1)
set_property(TARGET sipm PROPERTY CXX_STANDARD 17)
set_property(TARGET sipm PROPERTY PUBLIC_HEADER "${include}")
set_property(TARGET sipm PROPERTY OUTPUT_NAME sipm)

if (SIPM_ENABLE_LTO)
	set_property(TARGET sipm PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif (SIPM_ENABLE_LTO)
if (SIPM_ENABLE_OPENMP)
	target_link_libraries(sipm PRIVATE OpenMP::OpenMP_CXX)
endif(SIPM_ENABLE_OPENMP)

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
  find_package(Python)
	pybind11_add_module(SiPM SHARED ${pysrc} ${src})
	target_include_directories(SiPM PUBLIC "include/")
	if (SIPM_ENABLE_OPENMP)
		target_link_libraries(SiPM PRIVATE OpenMP::OpenMP_CXX)
	endif(SIPM_ENABLE_OPENMP)
	set_property(TARGET SiPM PROPERTY CXX_STANDARD 17)

	execute_process(
  COMMAND "${PYTHON_EXECUTABLE}" -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())"
  OUTPUT_VARIABLE PYTHON_INSTALL_SITE
  OUTPUT_STRIP_TRAILING_WHITESPACE)

	install(TARGETS SiPM
	LIBRARY DESTINATION lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages
	)
endif (SIPM_BUILD_PYTHON)
