cmake_minimum_required (VERSION 3.10)
project(distributed_manager)
find_package(Java REQUIRED)
include(UseJava)
execute_process(COMMAND protoc -I${CMAKE_CURRENT_SOURCE_DIR}/../../utils/src/protobuf --java_out=${CMAKE_CURRENT_BINARY_DIR} queryMessage.proto)


add_jar(TestServer TestServer.java ClientServiceThread.java QueueServiceThread.java VdmsTransaction.java ${CMAKE_CURRENT_BINARY_DIR}/VDMS/protobufs/QueryMessage.java INCLUDE_JARS /usr/share/java/protobuf.jar MANIFEST manifest.mf OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
