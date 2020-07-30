cmake_minimum_required (VERSION 3.10)
project(vdms_client)
find_package( OpenCV REQUIRED )

include_directories(../../utils/include/)
add_library(vdms-client SHARED VDMSClient.cc)
target_link_libraries(vdms-client vcl pmgd pmgd-util protobuf vdms-utils)

