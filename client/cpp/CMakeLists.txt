cmake_minimum_required (VERSION 3.10)
project(vdms_client)
find_package( OpenCV REQUIRED )
find_package(Protobuf CONFIG REQUIRED)
find_package( Threads REQUIRED )
include_directories(../../utils/include/ )


add_library(vdms-client SHARED VDMSClient.cc CSVParserUtil.cpp)
target_link_libraries(vdms-client protobuf vdms_protobuf vdms-utils pthread ${CMAKE_THREAD_LIBS_INIT} )

