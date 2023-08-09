cmake_minimum_required (VERSION 3.17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_STANDARD 17)

IF(CODE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -Wall -coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -coverage -fprofile-arcs -ftest-coverage")
    enable_testing()
ENDIF()

project(vdms_application)
add_compile_options(-g -fPIC -std=c++17)

find_package( OpenCV REQUIRED )
find_package(Protobuf REQUIRED)
find_package( CURL REQUIRED )
find_package(AWSSDK REQUIRED COMPONENTS core s3)

include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

execute_process(COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/utils/src/api_schema/createApiString.py ${CMAKE_CURRENT_SOURCE_DIR}/utils/src/api_schema/api_schema.json ${CMAKE_CURRENT_BINARY_DIR}/APISchema.h)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS utils/src/protobuf/partitionerMessages.proto utils/src/protobuf/pmgdMessages.proto utils/src/protobuf/queryMessage.proto)
add_library(vdms_protobuf SHARED ${PROTO_SRCS} ${PROTO_HDRS})

option(CLIENT "Built client library." OFF)
if (CLIENT)
  add_definitions("-D CLIENT")

  include_directories(client/cpp)
  include_directories(utils)
  add_subdirectory(utils)
  add_subdirectory(client/cpp)

else()
  add_subdirectory(src/pmgd)
  add_subdirectory(utils)
  add_subdirectory(tests)
  add_subdirectory(src/vcl)
  add_subdirectory(client/cpp)
  add_subdirectory(ext/custom_vcl)
  add_subdirectory(distributed)

  link_directories(/usr/local/lib /usr/lib/x86_64-linux-gnu/)
  include_directories(/usr/include/jsoncpp utils/include/ src/pmgd/include src/pmgd/util include/ src/vcl /usr/include ${CMAKE_CURRENT_BINARY_DIR}/utils/src/protobuf)
  add_library(dms SHARED
    src/BoundingBoxCommand.cc
    src/BlobCommand.cc
    src/CommunicationManager.cc
    src/DescriptorsCommand.cc
    src/DescriptorsManager.cc
    src/ExceptionsCommand.cc
    src/ImageCommand.cc
    src/PMGDIterators.cc
    src/PMGDQuery.cc
    src/PMGDQueryHandler.cc
    src/QueryHandler.cc
    src/QueryMessage.cc
    src/RSCommand.cc
    src/SearchExpression.cc
    src/Server.cc
    src/VDMSConfig.cc
    src/VideoCommand.cc
    src/AutoDeleteNode.cc
    src/ImageLoop.cc
    ${PROTO_SRCS} ${PROTO_HDRS}
)
  target_link_libraries(dms vcl pmgd pmgd-util protobuf tbb tiledb vdms-utils pthread -lcurl -lzmq ${AWSSDK_LINK_LIBRARIES})

  add_executable(vdms src/vdms.cc)
  target_link_libraries(vdms dms vdms_protobuf vcl tiledb faiss flinng jsoncpp ${OpenCV_LIBS} ${AWSSDK_LINK_LIBRARIES})
endif ()
