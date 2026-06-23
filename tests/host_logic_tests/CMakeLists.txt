cmake_minimum_required(VERSION 3.16)
project(edgeguard_host_logic_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(edgeguard_logic
  ../../firmware/EdgeGuard_ESP32/edgeguard_control.cpp
  ../../firmware/EdgeGuard_ESP32/edgeguard_event_log.cpp
  ../../firmware/EdgeGuard_ESP32/edgeguard_sensor_history.cpp
  ../../firmware/EdgeGuard_ESP32/edgeguard_diagnostics.cpp
)
target_include_directories(edgeguard_logic PUBLIC ../../firmware/EdgeGuard_ESP32)
target_compile_options(edgeguard_logic PRIVATE -Wall -Wextra -Wpedantic -Werror)

enable_testing()
foreach(test_name test_control_logic test_event_log test_sensor_history)
  add_executable(${test_name} ${test_name}.cpp)
  target_link_libraries(${test_name} PRIVATE edgeguard_logic)
  target_compile_options(${test_name} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
