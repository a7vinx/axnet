include_directories(${PROJECT_SOURCE_DIR}/src/)

add_executable(eventloop_test eventloop_test.cc)
target_link_libraries(eventloop_test axnet)

add_executable(buffer_test buffer_test.cc)
target_link_libraries(buffer_test axnet)

add_executable(tcpcs_test tcpcs_test.cc)
target_link_libraries(tcpcs_test axnet)

add_executable(pingpong_test pingpong_test.cc)
target_link_libraries(pingpong_test axnet)

add_executable(fake_http_test fake_http_test.cc)
target_link_libraries(fake_http_test axnet)
