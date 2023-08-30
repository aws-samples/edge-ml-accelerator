project(test_genicam_capture)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

add_executable(test_genicam_capture test.cc)

target_link_libraries(test_genicam_capture
    PUBLIC
    ${EDGE_ML_PROJECT_NAME}::pipeline
    )

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/TEST_CONFIG_GENICAM.json
    DESTINATION ${PROJECT_BUILD_DIR}/all_test_configs/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/TEST_CONFIG_GENICAM.json
    DESTINATION ${PROJECT_BUILD_DIR}/source/tests/all_test_configs/)

install(TARGETS test_genicam_capture
  COMPONENT bin
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})