project(pipeline_app)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(pipeline_app main.cc)

target_link_libraries(pipeline_app
    PUBLIC
    ${EDGE_ML_PROJECT_NAME}::pipeline
    )

install(TARGETS pipeline_app
    COMPONENT bin
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
