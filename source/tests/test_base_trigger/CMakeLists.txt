project(test_base_trigger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

add_executable(test_base_trigger test.cc)

target_link_libraries(test_base_trigger
    PUBLIC
    ${EDGE_ML_PROJECT_NAME}::pipeline
    )

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/TEST_CONFIG_BASE_TRIGGER.json
    DESTINATION ${PROJECT_BUILD_DIR}/all_test_configs/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/TEST_CONFIG_BASE_TRIGGER.json
    DESTINATION ${PROJECT_BUILD_DIR}/source/tests/all_test_configs/)

install(TARGETS test_base_trigger
  COMPONENT bin
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})