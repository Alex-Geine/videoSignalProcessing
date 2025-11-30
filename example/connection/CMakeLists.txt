cmake_minimum_required(VERSION 3.10.0)
project(photo VERSION 0.1.0 LANGUAGES C CXX)

find_package(OpenCV)
find_package(ZeroMQ)

include_directories( ${OpenCV_INCLUDE_DIRS} ) 
add_executable(srv_send srv_send.cpp)
add_executable(srv_recv srv_recv.cpp)
add_executable(clt_recv clt_recv.cpp)
add_executable(clt_send clt_send.cpp)

target_link_libraries( srv_send ${OpenCV_LIBS} zmq)
target_link_libraries( srv_recv ${OpenCV_LIBS} zmq)
target_link_libraries( clt_recv ${OpenCV_LIBS} zmq)
target_link_libraries( clt_send ${OpenCV_LIBS} zmq)


