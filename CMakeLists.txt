cmake_minimum_required(VERSION 3.9)
project(SimSiPM VERSION 1.0.0 DESCRIPTION "Library for SiPM simulation" LANGUAGES CXX)
include(GNUInstallDirs)

# CXX standard
set(CMAKE_CXX_STANDARD 11 CACHE STRING "C++ standard to be used")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Library
add_library(sipm SHARED
  src/components/SiPMRandom.cpp
  src/components/SiPMRandomvect.cpp
  src/lib/SiPMHit.cpp
  src/lib/SiPMProperties.cpp
  src/lib/SiPMAnalogSignal.cpp
  src/lib/SiPMDigitalSignal.cpp
  src/lib/SiPMSensor.cpp
  src/lib/SiPMAdc.cpp
)

set_target_properties(sipm PROPERTIES VERSION ${PROJECT_VERSION} PUBLIC_HEADER SimSiPM/SiPM.h)
set_target_properties(sipm PROPERTIES VERSION ${PROJECT_VERSION} PUBLIC_HEADER "SimSiPM/SiPM.h;SimSiPM/SiPMAdc.h;SimSiPM/SiPMAnalogSignal.h;SimSiPM/SiPMDigitalSignal.h;SimSiPM/SiPMHit.h;SimSiPM/SiPMProperties.h;SimSiPM/SiPMSensor.h")
set_target_properties(sipm PROPERTIES VERSION ${PROJECT_VERSION} PRIVATE_HEADER "src/components/SiPMHelpers.h;src/components/SiPMRandomvect.h;src/components/SiPMRandom.h")

# Include files
target_include_directories(sipm PUBLIC SimSiPM/)
target_include_directories(sipm PRIVATE src/components/)

# Install
install(TARGETS sipm
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    PRIVATE_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
