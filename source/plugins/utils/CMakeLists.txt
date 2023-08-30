#
 # © 2021 Amazon Web Services, Inc. or its affiliates. All Rights Reserved.
 #
#

set(component utils)
set(components ${components} ${component} PARENT_SCOPE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_library(${component} SHARED
    src/logger.cc
    src/image_preprocess.cc
    src/result_postprocess.cc
    src/yaml_parser.cc
    src/json_parser.cc
    src/edge_ml_config.cc
)

if(USE_MIC730AI)
    target_sources(${component} PRIVATE
        src/mic730ai_dio.cc
        )
endif()

add_library(${EDGE_ML_PROJECT_NAME}::${component} ALIAS ${component})

target_include_directories(${component}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${NLOHMANN_JSON_INCLUDE_DIR}
)

target_link_libraries(${component}
    PUBLIC
    ${OpenCV_LIBS}
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