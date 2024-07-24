cmake_minimum_required(VERSION 3.0.0)
project(kaka_test VERSION 0.1.0 LANGUAGES "CXX")

add_compile_options(-g -fPIC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -coverage -fprofile-abs-path")

find_package(Protobuf CONFIG REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(../client/cpp ../utils/include  librdkafka/src /usr/include/jsoncpp/ .  ..)
link_directories(  /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu/ . )

add_executable(meta_data kafka_test.cpp)
target_link_libraries(meta_data jsoncpp protobuf vdms-utils vdms-client vdms_protobuf rdkafka rdkafka++ pthread glog)

add_executable(image_data mutli_modal.cpp)
target_link_libraries(image_data jsoncpp protobuf vdms-utils vdms-client vdms_protobuf rdkafka rdkafka++ pthread glog)

add_executable(multi-modal adaptive_platform.cpp)
target_link_libraries(multi-modal jsoncpp protobuf vdms-utils vdms-client vdms_protobuf rdkafka rdkafka++ pthread glog)


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

