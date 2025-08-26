add_library(MB85RC256V-pico INTERFACE)

target_sources(MB85RC256V-pico INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/FRAM.cpp
)

target_include_directories(MB85RC256V-pico INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

# Pull in pico libraries that we need
target_link_libraries(MB85RC256V-pico INTERFACE pico_stdlib hardware_i2c)
