cmake_minimum_required (VERSION 3.17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_STANDARD 17)

option(CHRONO_TIMING "Use chrono timing." OFF)
option(CLIENT "Built client library." OFF)
option(CODE_COVERAGE "Collect coverage" OFF)

message("CHRONO_TIMING: " ${CHRONO_TIMING})
message("CLIENT: " ${CLIENT})
message("CODE_COVERAGE: " ${CODE_COVERAGE})

IF(CODE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -Wall -coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -coverage -fprofile-arcs -ftest-coverage")
    enable_testing()
ENDIF()

project(vdms_application)
add_compile_options(-g -fPIC -std=c++17)

find_package( OpenCV REQUIRED )
find_package(Protobuf CONFIG REQUIRED)
find_package( CURL REQUIRED )
find_package(AWSSDK REQUIRED COMPONENTS core s3)

include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

execute_process(COMMAND python3
  ${CMAKE_CURRENT_SOURCE_DIR}/utils/src/api_schema/createApiString.py
  ${CMAKE_CURRENT_SOURCE_DIR}/utils/src/api_schema/api_schema.json
  ${CMAKE_CURRENT_BINARY_DIR}/APISchema.h
)
add_library(vdms_protobuf OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/utils/src/protobuf/partitionerMessages.proto
  ${CMAKE_CURRENT_SOURCE_DIR}/utils/src/protobuf/pmgdMessages.proto
  ${CMAKE_CURRENT_SOURCE_DIR}/utils/src/protobuf/queryMessage.proto
)
target_link_libraries(vdms_protobuf PUBLIC protobuf::libprotobuf)
set(PROTO_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(vdms_protobuf PUBLIC "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>")
protobuf_generate(
  LANGUAGE cpp
  TARGET vdms_protobuf
  IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/utils/src/protobuf"
  PROTOC_OUT_DIR "${PROTO_BINARY_DIR}"
)

if (CLIENT)
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
  add_subdirectory(distributed)

  link_directories(/usr/local/lib /usr/lib/x86_64-linux-gnu/)
  include_directories(/usr/include/jsoncpp utils/include/ src/pmgd/include src/pmgd/util include/ src/vcl /usr/include ${CMAKE_CURRENT_BINARY_DIR}/utils/src/protobuf)
  add_library(dms SHARED
    src/BackendNeo4j.cc
    src/BoundingBoxCommand.cc
    src/BlobCommand.cc
    src/CommunicationManager.cc
    src/DescriptorsCommand.cc
    src/DescriptorsManager.cc
    src/ExceptionsCommand.cc
    src/ImageCommand.cc
    src/OpsIOCoordinator.cc
    src/PMGDIterators.cc
    src/PMGDQuery.cc
    src/PMGDQueryHandler.cc
    src/QueryHandlerExample.cc
    src/QueryHandlerBase.cc
    src/QueryHandlerPMGD.cc
    src/QueryMessage.cc
    src/RSCommand.cc
    src/SearchExpression.cc
    src/Server.cc
    src/VDMSConfig.cc
    src/VideoCommand.cc
    src/AutoDeleteNode.cc
    src/ImageLoop.cc
    src/VideoLoop.cc
  )
  target_link_libraries(dms vcl pmgd pmgd-util protobuf tbb tiledb vdms-utils pthread -lcurl -lzmq ${AWSSDK_LINK_LIBRARIES} neo4j-client)
  add_executable(vdms src/vdms.cc)
  target_link_libraries(vdms dms vdms_protobuf vcl tiledb faiss flinng jsoncpp ${OpenCV_LIBS} ${AWSSDK_LINK_LIBRARIES})
endif ()
