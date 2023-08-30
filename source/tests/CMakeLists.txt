cmake_minimum_required (VERSION 3.7)

project(unit-tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")


add_subdirectory(test_base_trigger)
add_test(NAME test_base_trigger COMMAND test_base_trigger)

add_subdirectory(test_base_capture)
add_test(NAME test_base_capture COMMAND test_base_capture)

add_subdirectory(test_opencv_capture)
add_test(NAME test_opencv_capture COMMAND test_opencv_capture)

add_subdirectory(test_gstreamer_capture)
add_test(NAME test_gstreamer_capture COMMAND test_gstreamer_capture)

if(USE_GENICAM)
    add_subdirectory(test_genicam_capture)
    add_test(NAME test_genicam_capture COMMAND test_genicam_capture)
endif()

if(USE_PYLON)
    add_subdirectory(test_pylon_capture)
    add_test(NAME test_pylon_capture COMMAND test_pylon_capture)  
endif()

add_subdirectory(test_base_output)
add_test(NAME test_base_output COMMAND test_base_output)
