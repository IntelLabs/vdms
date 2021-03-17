cmake_minimum_required (VERSION 3.10)
project(vcl_library)
find_package( OpenCV REQUIRED )

include_directories(../../include .)

add_library(vcl SHARED DescriptorSet.cc DescriptorSetData.cc Exception.cc FaissDescriptorSet.cc Image.cc KeyFrame.cc TDBDenseDescriptorSet.cc TDBDescriptorSet.cc TDBImage.cc TDBObject.cc TDBSparseDescriptorSet.cc utils.cc Video.cc RemoteConnection.cc)
target_link_libraries(vcl lapack faiss avformat avcodec swscale ${OpenCV_LIBS})
