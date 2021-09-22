cmake_minimum_required (VERSION 3.17)

IF(CODE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -Wall -coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -coverage -fprofile-arcs -ftest-coverage")
    enable_testing()
ENDIF()

project(vdms_application)
add_compile_options(-g -fPIC)

find_package( OpenCV REQUIRED )
find_package(Protobuf REQUIRED)

include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

execute_process(COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/utils/src/api_schema/createApiString.py ${CMAKE_CURRENT_SOURCE_DIR}/utils/src/api_schema/api_schema.json ${CMAKE_CURRENT_BINARY_DIR}/APISchema.h)

message(STATUS "before " ${PROTO_SRCS} ${PROTO_HDRS})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS utils/src/protobuf/partitionerMessages.proto utils/src/protobuf/pmgdMessages.proto utils/src/protobuf/queryMessage.proto)
add_library(vdms_protobuf SHARED ${PROTO_SRCS} ${PROTO_HDRS})
message(STATUS "after " ${PROTO_SRCS} ${PROTO_HDRS})

add_subdirectory(src/pmgd)
add_subdirectory(utils)
add_subdirectory(tests)
add_subdirectory(src/vcl)
add_subdirectory(client/cpp)
link_directories(/usr/local/lib /usr/lib/x86_64-linux-gnu/)
include_directories(/usr/include/jsoncpp utils/include/ src/pmgd/include src/pmgd/util include/ src/vcl ${CMAKE_CURRENT_BINARY_DIR}/utils/src/protobuf)
add_library(dms SHARED src/BoundingBoxCommand.cc src/CommunicationManager.cc src/DescriptorsCommand.cc src/DescriptorsManager.cc src/ExceptionsCommand.cc src/ImageCommand.cc src/PMGDIterators.cc src/PMGDQuery.cc src/PMGDQueryHandler.cc src/QueryHandler.cc src/QueryMessage.cc src/RSCommand.cc src/SearchExpression.cc src/Server.cc src/VDMSConfig.cc src/VideoCommand.cc ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(dms vcl pmgd pmgd-util protobuf vdms-utils)

add_executable(vdms src/vdms.cc)
target_link_libraries(vdms dms vdms_protobuf vcl tiledb faiss jsoncpp ${OpenCV_LIBS})

#https://cmake.org/cmake/help/v3.9/module/FindProtobuf.html

#target_link_libraries(bar ${Protobuf_LIBRARIES})
