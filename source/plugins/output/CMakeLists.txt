#
 # © 2021 Amazon Web Services, Inc. or its affiliates. All Rights Reserved.
 #
#

set(component output)
set(components ${components} ${component} PARENT_SCOPE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_library(${component} SHARED
    src/base_output.cc
    src/local_disk.cc
    src/s3_upload.cc
    src/publish_ipc_topic.cc
    src/publish_mqtt_topic.cc
    )

add_library(${EDGE_ML_PROJECT_NAME}::${component} ALIAS ${component})

target_include_directories(${component}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${NLOHMANN_JSON_INCLUDE_DIR}
)

target_link_libraries(${component}
    PUBLIC
    ${EDGE_ML_PROJECT_NAME}::utils
    ${PNG_LIBRARY}
    ${AWSSDK_LINK_LIBRARIES}
    AWS::GreengrassIpc-cpp
    -lstdc++fs
    )

#install scripts
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