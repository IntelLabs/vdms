cmake_minimum_required (VERSION 3.10)
project(vdms_application)
find_package( OpenCV REQUIRED )

add_subdirectory(utils)
add_subdirectory(tests)
add_subdirectory(src/vcl)
add_subdirectory(client/cpp)
link_directories(/usr/local/lib /usr/lib/x86_64-linux-gnu/)
include_directories(/usr/include/jsoncpp utils/include/ pmgd/include pmgd/util include/ src/vcl )
add_library(dms SHARED src/BoundingBoxCommand.cc src/CommunicationManager.cc src/DescriptorsCommand.cc src/DescriptorsManager.cc src/ExceptionsCommand.cc src/ImageCommand.cc src/PMGDIterators.cc src/PMGDQuery.cc src/PMGDQueryHandler.cc src/QueryHandler.cc src/QueryMessage.cc src/RSCommand.cc src/SearchExpression.cc src/Server.cc src/VDMSConfig.cc src/VideoCommand.cc)
target_link_libraries(dms vcl pmgd pmgd-util pmgd-jni protobuf vdms-utils)

add_executable(vdms src/vdms.cc)
target_link_libraries(vdms dms vcl tiledb faiss jsoncpp ${OpenCV_LIBS})



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
