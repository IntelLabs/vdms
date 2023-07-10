cmake_minimum_required (VERSION 3.17)
project(vcl_library)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_STANDARD 17)

find_package( OpenCV REQUIRED )

include_directories(../../include . /usr/local/include/opencv4 /usr/include/jsoncpp)

add_library(vcl SHARED
    DescriptorSet.cc
    DescriptorSetData.cc
    Exception.cc
    FaissDescriptorSet.cc
    FlinngDescriptorSet.cc
    Image.cc
    KeyFrame.cc
    TDBDenseDescriptorSet.cc
    TDBDescriptorSet.cc
    TDBImage.cc
    TDBObject.cc
    TDBSparseDescriptorSet.cc
    utils.cc
    Video.cc
    CustomVCL.cc
    RemoteConnection.cc
)
link_directories( /usr/local/lib )
target_link_libraries(vcl lapack faiss tiledb flinng avformat avcodec swscale ${OpenCV_LIBS})
target_compile_options(vcl PRIVATE -Wno-deprecated-declarations)

