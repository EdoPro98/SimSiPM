# Interactive SiPM simulator GUI (Dear ImGui + ImPlot on GLFW/OpenGL3).
# Enabled from the top-level project with -DSIPM_BUILD_GUI=ON.

include(FetchContent)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# zlib compresses the waveform dumps
find_package(ZLIB QUIET)
if(NOT ZLIB_FOUND)
  message(STATUS "zlib not found on system — fetching from GitHub")
  set(ZLIB_BUILD_EXAMPLES OFF CACHE INTERNAL "")
  FetchContent_Declare(zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG        v1.3.1
    GIT_SHALLOW    TRUE)
  FetchContent_MakeAvailable(zlib)
  add_library(ZLIB::ZLIB ALIAS zlibstatic)
  target_include_directories(zlibstatic PUBLIC ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR})
endif()

find_package(glfw3 3.3 QUIET)
if(NOT glfw3_FOUND)
  message(STATUS "glfw3 not found on system — fetching from GitHub")
  set(GLFW_BUILD_DOCS     OFF CACHE INTERNAL "")
  set(GLFW_BUILD_TESTS    OFF CACHE INTERNAL "")
  set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
  set(GLFW_INSTALL        OFF CACHE INTERNAL "")
  set(GLFW_BUILD_WAYLAND  OFF CACHE INTERNAL "")
  FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE)
  FetchContent_MakeAvailable(glfw)
endif()

FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.92.6
  GIT_SHALLOW    TRUE)
FetchContent_Declare(implot
  GIT_REPOSITORY https://github.com/epezent/implot.git
  GIT_TAG        v1.0
  GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(imgui implot)

add_library(sipm_gui_deps STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  ${implot_SOURCE_DIR}/implot.cpp
  ${implot_SOURCE_DIR}/implot_items.cpp
)
target_include_directories(sipm_gui_deps PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
  ${implot_SOURCE_DIR}
)
target_link_libraries(sipm_gui_deps PUBLIC glfw OpenGL::GL)

# Precomputed scintillator spectra (source x material x size), generated at
# build time by a Monte Carlo using NIST XCOM cross sections. Regenerated only
# when the tool or its input data change.
set(SIPM_GUI_SPECTRA_DIR ${CMAKE_CURRENT_BINARY_DIR}/spectra)
file(GLOB SIPM_GUI_SPECTRA_INPUTS
  ${CMAKE_CURRENT_SOURCE_DIR}/data/scintillators.json
  ${CMAKE_CURRENT_SOURCE_DIR}/data/nuclides/*.csv
  ${CMAKE_CURRENT_SOURCE_DIR}/data/xcom/*.csv)
add_executable(sipm_gui_spectra
  tools/make_spectra.cpp
  src/Scintillator.cpp
  src/FileIO.cpp
)
target_include_directories(sipm_gui_spectra PRIVATE src)
target_link_libraries(sipm_gui_spectra PRIVATE sipm nlohmann_json::nlohmann_json Threads::Threads)
add_custom_command(
  OUTPUT ${SIPM_GUI_SPECTRA_DIR}/stamp
  COMMAND sipm_gui_spectra ${CMAKE_CURRENT_SOURCE_DIR}/data ${SIPM_GUI_SPECTRA_DIR}
  DEPENDS sipm_gui_spectra ${SIPM_GUI_SPECTRA_INPUTS}
  COMMENT "Precomputing scintillator spectra (Monte Carlo, about half a minute)"
  VERBATIM)
add_custom_target(sipm_gui_spectra_data DEPENDS ${SIPM_GUI_SPECTRA_DIR}/stamp)

add_executable(sipm_gui
  src/main.cpp
  src/App.cpp
  src/Pipeline.cpp
  src/Source.cpp
  src/Scintillator.cpp
  src/SipmModels.cpp
  src/Png.cpp
  src/Recorder.cpp
  src/FileIO.cpp
)
add_dependencies(sipm_gui sipm_gui_spectra_data)
target_compile_definitions(sipm_gui PRIVATE
  SIPM_GUI_FONT="${imgui_SOURCE_DIR}/misc/fonts/Roboto-Medium.ttf"
  SIPM_GUI_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
  SIPM_GUI_SPECTRA_DIR="${SIPM_GUI_SPECTRA_DIR}"
)
target_link_libraries(sipm_gui PRIVATE sipm sipm_gui_deps nlohmann_json::nlohmann_json ZLIB::ZLIB Threads::Threads)
