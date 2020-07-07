cmake_minimum_required (VERSION 3.10)
project(tests)
find_package( OpenCV REQUIRED )
find_package( Threads REQUIRED )

link_directories(/usr/local/lib /usr/lib/x86_64-linux-gnu/)
include_directories(/usr/include/jsoncpp ../utils/include/ ../pmgd/include ../pmgd/util ../include/ ../src/vcl ../src)
add_library(unit_tests SHARED unit_tests/pmgd_queries.cc unit_tests/helpers.cc unit_tests/TDBImage_test.cc unit_tests/Image_test.cc unit_tests/Video_test.cc unit_tests/DescriptorSetAdd_test.cc unit_tests/DescriptorSetClassify_test.cc unit_tests/DescriptorSetTrain_test.cc unit_tests/DescriptorSetReadFS_test.cc unit_tests/DescriptorSetStore_test.cc)
target_link_libraries(unit_tests vcl pmgd pmgd-util pmgd-jni pthread protobuf vdms-utils)

add_executable(query_tests main.cc)
target_link_libraries(query_tests unit_tests dms vcl tiledb faiss gtest jsoncpp ${CMAKE_THREAD_LIBS_INIT} ${OpenCV_LIBS})



#BoundingBoxCommand.cc
#CommunicationManager.cc
#DescriptorsCommand.cc
#DescriptorsManager.cc
#ExceptionsCommand.cc
#ImageCommand.cc
#PMGDIterators.cc
#PMGDQuery.cc
#PMGDQueryHandler.cc
#QueryHandler.cc
#QueryMessage.cc
#RSCommand.cc
#SearchExpression.cc
#Server.cc
#VDMSConfig.cc
#VideoCommand.cc

#vdms.cc
