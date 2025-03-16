find_package(benchmark REQUIRED)

include_directories(../include)

# Add source to this project's executable.
add_executable (random "random.cpp")
add_executable (sensor "sensor.cpp")

# Link Google Benchmark to the project
target_link_libraries(random PRIVATE benchmark::benchmark)
target_link_libraries(random PRIVATE sipm)
target_link_libraries(sensor PRIVATE benchmark::benchmark)
target_link_libraries(sensor PRIVATE sipm)
