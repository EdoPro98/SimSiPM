find_package(benchmark REQUIRED)

include_directories(../include)

add_executable(random "random.cpp")
add_executable(sensor "sensor.cpp")

foreach(target random sensor)
  target_link_libraries(${target} PRIVATE benchmark::benchmark sipm)

  # Frame pointers are required for perf/VTune call-graph sampling.
  # -fno-omit-frame-pointer adds ~1% overhead but enables accurate profiling.
  target_compile_options(${target} PRIVATE -fno-omit-frame-pointer)

  # Optional: link libpfm4 when available so --benchmark_perf_counters works.
  find_library(PFM_LIB pfm)
  if(PFM_LIB)
    target_link_libraries(${target} PRIVATE ${PFM_LIB})
    target_compile_definitions(${target} PRIVATE HAVE_LIBPFM)
  endif()
endforeach()
