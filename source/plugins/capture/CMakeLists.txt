#
 # © 2021 Amazon Web Services, Inc. or its affiliates. All Rights Reserved.
 #
#

set(component capture)
set(components ${components} ${component} PARENT_SCOPE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

if (USE_GENICAM)
    find_package(rc_genicam_api_static)
endif()

if((USE_PYLON AND USE_GENICAM))
    add_library(${component} SHARED
        src/base_capture.cc
        src/genicam_capture.cc
        src/opencv_capture.cc
        src/pylon_capture.cc
    )
elseif(USE_PYLON)
    add_library(${component} SHARED
        src/base_capture.cc
        src/pylon_capture.cc
        src/opencv_capture.cc
    )
elseif(USE_GENICAM)
    add_library(${component} SHARED
        src/base_capture.cc
        src/genicam_capture.cc
        src/opencv_capture.cc
    )
else()
    add_library(${component} SHARED
        src/base_capture.cc
        src/opencv_capture.cc
    )
endif()

add_library(${EDGE_ML_PROJECT_NAME}::${component} ALIAS ${component})

target_include_directories(${component}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${NLOHMANN_JSON_INCLUDE_DIR}
)

if((USE_PYLON AND USE_GENICAM))
    target_link_libraries(${component}
        PUBLIC
        ${EDGE_ML_PROJECT_NAME}::utils
        ${OpenCV_LIBS}
        rc_genicam_api::rc_genicam_api_static
        ${PYLON_PATH}/lib/libpylonbase.so
        ${PYLON_PATH}/lib/libpylonutility.so
    )
elseif(USE_PYLON)
    target_link_libraries(${component}
        PUBLIC
        ${EDGE_ML_PROJECT_NAME}::utils
        ${OpenCV_LIBS}
        ${PYLON_PATH}/lib/libpylonbase.so
        ${PYLON_PATH}/lib/libpylonutility.so
    )
elseif(USE_GENICAM)
    target_link_libraries(${component}
        PUBLIC
        ${EDGE_ML_PROJECT_NAME}::utils
        ${OpenCV_LIBS}
        rc_genicam_api::rc_genicam_api_static
    )
else()
    target_link_libraries(${component}
        PUBLIC
        ${EDGE_ML_PROJECT_NAME}::utils
        ${OpenCV_LIBS}
    )
endif()

install(TARGETS ${component} EXPORT ${component}Targets
    COMPONENT ${component}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${EDGE_ML_PROJECT_NAME}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${EDGE_ML_PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION include
    )

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    TYPE INCLUDE
    COMPONENT ${component}
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.cuh"
        PATTERN "*.hpp"
    )

# optional
# install(EXPORT ${component}Targets
#     FILE "${PROJECT_NAME}-${component}Targets.cmake"
#     NAMESPACE ${PROJECT_NAME}::
#     DESTINATION lib/cmake/${PROJECT_NAME}
#     COMPONENT ${component}
# )
# configure_file("${component}Config.cmake.in"
#     "${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${component}Config.cmake"
#     @ONLY
# )
# install(
#     FILES
#     "${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${component}Config.cmake"
#     DESTINATION lib/cmake/${PROJECT_NAME}
#     COMPONENT ${component}
# )