find_package(benchmark REQUIRED)

include_directories(../include)

# Add source to this project's executable.
add_executable (rng "rng.cpp")
add_executable (random "random.cpp")
add_executable (sensor "sensor.cpp")

# Link Google Benchmark to the project
target_link_libraries(rng PRIVATE benchmark::benchmark sipm)
target_link_libraries(random PRIVATE benchmark::benchmark sipm)
target_link_libraries(sensor PRIVATE benchmark::benchmark sipm)
