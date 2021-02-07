cmake_minimum_required(VERSION 3.9)
project(SimSiPM VERSION 1 DESCRIPTION "Library for SiPM simulation" LANGUAGES CXX)
include(GNUInstallDirs)

set(CMAKE_INSTALL_PREFIX /usr CACHE STRING
	"Install path prefix, prepended onto install directories."
	FORCE)


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

file(GLOB_RECURSE sources "src/*.cpp")
file(GLOB_RECURSE headers "src/*.h")

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

target_compile_features(sipm PUBLIC cxx_std_11)

set_property(TARGET sipm PROPERTY PUBLIC_HEADER SimSiPM/SiPM.h)
set_property(TARGET sipm PROPERTY PUBLIC_HEADER ${public_headers})
set_property(TARGET sipm PROPERTY PRIVATE_HEADER ${private_headers})
set_property(TARGET sipm PROPERTY VERSION 1)



# Install
install(TARGETS sipm
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    PRIVATE_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
