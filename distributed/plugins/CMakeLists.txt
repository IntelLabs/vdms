cmake_minimum_required (VERSION 3.10)
project(distributed_plugins)
find_package(Java REQUIRED)
include(UseJava)
execute_process(COMMAND protoc -I=${CMAKE_CURRENT_SOURCE_DIR}/../../utils/src/protobuf/ --java_out=${CMAKE_CURRENT_BINARY_DIR} queryMessage.proto)

add_jar(TestPlugin TestPlugin.java PublisherServiceThread.java QueueServiceThread.java SubscriberServiceThread.java VdmsTransaction.java TcpVdmsConnection.java VdmsConnection.java ${CMAKE_CURRENT_BINARY_DIR}/VDMS/protobufs/QueryMessage.java INCLUDE_JARS /usr/share/java/protobuf.jar /usr/share/java/json-simple-1.1.1.jar  /usr/share/java/commons-codec-1.15.jar MANIFEST manifest-repeater.mf OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})

add_jar(TestFilter TestFilter.java PublisherServiceThread.java PassList.java QueueServiceThread.java SubscriberServiceThread.java QueryList.java VdmsTransaction.java TcpVdmsConnection.java VdmsConnection.java ${CMAKE_CURRENT_BINARY_DIR}/VDMS/protobufs/QueryMessage.java INCLUDE_JARS /usr/share/java/protobuf.jar /usr/share/java/json-simple-1.1.1.jar /usr/share/java/commons-codec-1.15.jar MANIFEST manifest-filter.mf OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})

add_jar(TestAutoPublisher AutoDeletePlugin.java AutoPublisherServiceThread.java QueueServiceThread.java VdmsTransaction.java TcpVdmsConnection.java ${CMAKE_CURRENT_BINARY_DIR}/VDMS/protobufs/QueryMessage.java INCLUDE_JARS /usr/share/java/protobuf.jar /usr/share/java/json-simple-1.1.1.jar /usr/share/java/commons-codec-1.15.jar MANIFEST manifest-auto.mf OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})


add_jar(QueryMessage ${CMAKE_CURRENT_BINARY_DIR}/VDMS/protobufs/QueryMessage.java INCLUDE_JARS /usr/share/java/protobuf.jar OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
