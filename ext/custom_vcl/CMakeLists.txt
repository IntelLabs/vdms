cmake_minimum_required (VERSION 3.10)
project(custom_vcl_application)
find_package( OpenCV REQUIRED )

include_directories(.  ../../../vdms/src ../../../vdms/include/ ../../../vdms/src/vcl /usr/include/jsoncpp ${OpenCV_INCLUDE_DIRS})
add_executable(custom_vcl custom_vcl_process.cc )
target_link_libraries(custom_vcl vcl tiledb jsoncpp ${OpenCV_LIBS})
