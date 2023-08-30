cmake_minimum_required (VERSION 3.7)

project(edge-ml-accelerator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

set(EDGE_ML_PROJECT_NAME ${PROJECT_NAME})

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -lstdc++fs")

set(edge-ml-accelerator_MAIN_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND CMAKE_MODULE_PATH "${edge-ml-accelerator_MAIN_SOURCE_DIR}/cmake")

set(PROJECT_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})

message (STATUS "EDGE_ML_PROJECT_NAME ...................... " ${EDGE_ML_PROJECT_NAME})
message (STATUS "edge-ml-accelerator_MAIN_SOURCE_DIR ...................... " ${edge-ml-accelerator_MAIN_SOURCE_DIR})
message (STATUS "CMAKE_SOURCE_DIR ...................... " ${CMAKE_SOURCE_DIR})
message (STATUS "PROJECT_BUILD_DIR ...................... " ${PROJECT_BUILD_DIR})
message (STATUS "DCMAKE_PREFIX_PATH ...................... " ${DCMAKE_PREFIX_PATH})
message (STATUS "CMAKE_BUILD_TYPE ...................... " ${CMAKE_BUILD_TYPE})

set(PACKAGE_CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
set(CMAKE_INSTALL_BINDIR ${CMAKE_INSTALL_PREFIX}/bin)
message (STATUS "CMAKE_INSTALL_LIBDIR ...................... " ${CMAKE_INSTALL_LIBDIR})
message (STATUS "CMAKE_INSTALL_BINDIR ...................... " ${CMAKE_INSTALL_BINDIR})

find_package(OpenCV REQUIRED)
find_package(PNG)
find_package(AWSSDK REQUIRED COMPONENTS s3 dynamodb)
find_package(GreengrassIpc-cpp REQUIRED)
find_package(AWSSDK REQUIRED COMPONENTS greengrass)
set(NLOHMANN_JSON_INCLUDE_DIR ${PACKAGE_CMAKE_INSTALL_PREFIX}/include)


## Logging
set(LOGS_TYPE 1)
add_definitions(-DLOGS_TYPE=${LOGS_TYPE})
if(DEFINED LOGS_LEVEL)
    set(LOGS_LEVEL ${LOGS_LEVEL})
else()
    set(LOGS_LEVEL 1)
endif()
add_definitions(-DLOGS_LEVEL=${LOGS_LEVEL})
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_definitions(-DLOGS_LEVEL=5)
endif()
message (STATUS "LOGS_LEVEL ...................... " ${LOGS_LEVEL})
message (STATUS "LOGS_LEVEL_TYPE ...................... " ${LOGS_TYPE})


## Sources::Plugins::Utils
option(USE_MIC730AI "Using Advantech MIC730AI" ON)
if(USE_MIC730AI)
    add_definitions(-DWITH_MIC730AI)
endif()
message (STATUS "USE_MIC730AI ...................... " ${USE_MIC730AI})
add_subdirectory(source/plugins/utils)


## Sources::Plugins::Output
add_subdirectory(source/plugins/output)


## Sources::Plugins::Trigger
add_subdirectory(source/plugins/trigger)


## Sources::Plugins::Capture
option(USE_GENICAM "Using Pylon Capture" OFF)
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    include_directories(third_party/rc_genicam_api)
    add_subdirectory(third_party/rc_genicam_api)
    option(USE_GENICAM "Using Pylon Capture" ON)
endif()
add_subdirectory(source/plugins/capture)
option(USE_PYLON "Using Pylon Capture" OFF)
if(USE_PYLON)
    add_definitions(-DWITH_PYLON)
    set(PYLON_PATH ${CMAKE_SOURCE_DIR}/pylon_package)
    include_directories(${PYLON_PATH}/include)    
endif()
message (STATUS "USE_GENICAM ...................... " ${USE_GENICAM})
message (STATUS "USE_PYLON ...................... " ${USE_PYLON})


## Sources::Plugins::Inference
include(source/plugins/inference/cmake/common.cmake)
add_subdirectory(source/plugins/inference)

## Sources::Plugins::Pipeline
add_subdirectory(source/plugins/pipeline)


## Sources::Tests
option(CMAKE_BUILD_TESTS "Build tests" ON)
if(CMAKE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(source/tests)
endif()
message (STATUS "CMAKE_BUILD_TESTS ...................... " ${CMAKE_BUILD_TESTS})


## Sources::App to build the pipeline app
add_subdirectory(source/app)
