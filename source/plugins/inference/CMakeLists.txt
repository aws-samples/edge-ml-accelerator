#
 # © 2021 Amazon Web Services, Inc. or its affiliates. All Rights Reserved.
 #
#

if(POLICY CMP0070)
    cmake_policy(SET CMP0070 NEW)
endif()

set(component inference)
set(components ${components} ${component} PARENT_SCOPE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

find_package(gRPC REQUIRED HINTS ${CMAKE_SOURCE_DIR}/third_party/grpc)

### Amazon Lookout for Vision Edge PROTO
# Proto file
get_filename_component(hw_proto "protos/edge-agent.proto" ABSOLUTE)
get_filename_component(hw_proto_path "${hw_proto}" PATH)
# Generated sources
set(PROTOS_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(hw_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/edge-agent.pb.cc")
set(hw_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/edge-agent.pb.h")
set(hw_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/edge-agent.grpc.pb.cc")
set(hw_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/edge-agent.grpc.pb.h")
# run command
add_custom_command(
      OUTPUT "${hw_proto_srcs}" "${hw_proto_hdrs}" "${hw_grpc_srcs}" "${hw_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hw_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_proto}"
      DEPENDS "${hw_proto}")
# hw_grpc_proto
add_library(hw_grpc_proto SHARED
  ${hw_grpc_srcs}
  ${hw_grpc_hdrs}
  ${hw_proto_srcs}
  ${hw_proto_hdrs}
)

target_link_libraries(hw_grpc_proto
  gRPC::grpc++
  )

install(TARGETS hw_grpc_proto
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${EDGE_ML_PROJECT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${EDGE_ML_PROJECT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    TYPE INCLUDE
    COMPONENT hw_grpc_proto
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.cuh"
        PATTERN "*.hpp"
    )

######################################

### Amazon SageMaker EdgeManager PROTO
# Proto file
get_filename_component(hw_em_proto "protos/edgemanager-agent.proto" ABSOLUTE)
get_filename_component(hw_em_proto_path "${hw_em_proto}" PATH)
# Generated sources
set(PROTOS_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(hw_em_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/edgemanager-agent.pb.cc")
set(hw_em_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/edgemanager-agent.pb.h")
set(hw_em_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/edgemanager-agent.grpc.pb.cc")
set(hw_em_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/edgemanager-agent.grpc.pb.h")
# run command
add_custom_command(
      OUTPUT "${hw_em_proto_srcs}" "${hw_em_proto_hdrs}" "${hw_em_grpc_srcs}" "${hw_em_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hw_em_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_em_proto}"
      DEPENDS "${hw_em_proto}")
# hw_em_grpc_proto
add_library(hw_em_grpc_proto SHARED
  ${hw_em_grpc_srcs}
  ${hw_em_grpc_hdrs}
  ${hw_em_proto_srcs}
  ${hw_em_proto_hdrs}
)

target_link_libraries(hw_em_grpc_proto
  gRPC::grpc++
  )

install(TARGETS hw_em_grpc_proto
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${EDGE_ML_PROJECT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${EDGE_ML_PROJECT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    TYPE INCLUDE
    COMPONENT hw_em_grpc_proto
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.cuh"
        PATTERN "*.hpp"
    )
######################################

# variable from main cmake
find_package(Threads REQUIRED)

add_library(${component} SHARED
    src/base_inference.cc
    src/lfve_client.cc
    src/edgemanager_client.cc
    src/triton_client.cc
    src/onnxruntime_client.cc
    )

add_library(${EDGE_ML_PROJECT_NAME}::${component} ALIAS ${component})

set(ONNX_INCLUDE_DIR ${PACKAGE_CMAKE_INSTALL_PREFIX}/include/onnxruntime)
file(GLOB ONNX_RUNTIME_LIBS_LIB ${PACKAGE_CMAKE_INSTALL_PREFIX}/lib/*onnxruntime*)
file(GLOB ONNX_RUNTIME_LIBS_LIB64 ${PACKAGE_CMAKE_INSTALL_PREFIX}/lib64/*onnxruntime*)
list(APPEND ONNX_RUNTIME_LIBS ${ONNX_RUNTIME_LIBS_LIB} ${ONNX_RUNTIME_LIBS_LIB64})

target_include_directories(${component}
    PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${PACKAGE_CMAKE_INSTALL_PREFIX}/include/
    ${ONNX_INCLUDE_DIR}
    ${NLOHMANN_JSON_INCLUDE_DIR}
)

file(GENERATE
    OUTPUT "${component}.txt"
    CONTENT "$<TARGET_PROPERTY:${component},INCLUDE_DIRECTORIES>\n"
)

target_link_directories(${component} PUBLIC ${CMAKE_INSTALL_LIBDIR})
target_link_directories(${component} PUBLIC ${CMAKE_INSTALL_LIBDIR}/../lib/)
target_link_directories(${component} PUBLIC ${CMAKE_INSTALL_LIBDIR}/../lib64/)

target_link_libraries(${component}
        PUBLIC
        ${EDGE_ML_PROJECT_NAME}::utils
        hw_grpc_proto
        hw_em_grpc_proto
        ${CMAKE_THREAD_LIBS_INIT}
        gRPC::grpc++
        grpcclient
        ${ONNX_RUNTIME_LIBS}
        )

#install scripts
install(TARGETS ${component} EXPORT ${component}Targets
    COMPONENT ${component}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${EDGE_ML_PROJECT_NAME}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${EDGE_ML_PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION include
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