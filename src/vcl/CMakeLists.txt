cmake_minimum_required (VERSION 3.10)
project(vcl_library)
find_package( OpenCV REQUIRED )

include_directories(../../include . /usr/local/include/opencv4 /usr/include/jsoncpp)

add_library(vcl SHARED DescriptorSet.cc DescriptorSetData.cc Exception.cc FaissDescriptorSet.cc Image.cc KeyFrame.cc TDBDenseDescriptorSet.cc TDBDescriptorSet.cc TDBImage.cc TDBObject.cc TDBSparseDescriptorSet.cc utils.cc Video.cc)
add_library(custom_vcl SHARED CustomVCL.cc)
target_link_libraries(vcl lapack faiss avformat avcodec swscale custom_vcl ${OpenCV_LIBS})

add_custom_command(TARGET custom_vcl POST_BUILD COMMAND cp $<TARGET_FILE:custom_vcl> "lib_defaultcustom_vcl.so")
add_custom_command(TARGET custom_vcl POST_BUILD COMMAND rm $<TARGET_FILE:custom_vcl>)

add_custom_command(TARGET custom_vcl POST_BUILD COMMAND ln -s "lib_defaultcustom_vcl.so" $<TARGET_FILE:custom_vcl>)

