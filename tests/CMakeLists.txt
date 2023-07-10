cmake_minimum_required (VERSION 3.17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_STANDARD 17)

option(CODE_COVERAGE "Collect coverage" OFF)
IF(CODE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -Wall -coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -coverage -fprofile-arcs -ftest-coverage")
    enable_testing()
ENDIF()
message("Coverage:" ${CODE_COVERAGE})

project(tests
    LANGUAGES "CXX"
)
find_package( OpenCV REQUIRED )
find_package( Threads REQUIRED )
find_package(AWSSDK REQUIRED COMPONENTS core s3)

link_directories(/usr/local/lib/ /usr/lib/x86_64-linux-gnu/)
include_directories(
    ../src
    ../include/
    ../include/vcl
    ../utils/include/
    ../src/vcl
    /usr/include/jsoncpp
    ../src/pmgd/include
    ../src/pmgd/util
    ../client/cpp
    ../utils/
)

add_executable(unit_tests
    main.cc
    server/json_queries.cc
    unit_tests/pmgd_queries.cc
    unit_tests/helpers.cc
    unit_tests/TDBImage_test.cc
    unit_tests/Image_test.cc
    unit_tests/RemoteConnection_test.cc
    unit_tests/Video_test.cc
    unit_tests/DescriptorSetAdd_test.cc
    unit_tests/DescriptorSetClassify_test.cc
    unit_tests/DescriptorSetTrain_test.cc
    unit_tests/DescriptorSetReadFS_test.cc
    unit_tests/DescriptorSetStore_test.cc
    unit_tests/client_add_entity.cc
    unit_tests/client_csv.cc
    unit_tests/meta_data.cc
    unit_tests/client_find_entities.cc
    unit_tests/client_image.cc
    unit_tests/client_bounding_box.cc
    unit_tests/client_descriptors.cc
    unit_tests/client_videos.cc
    unit_tests/client_blob.cc
)

target_link_libraries(unit_tests
    avcodec
    avformat
    dms
    faiss
    flinng
    gtest
    jsoncpp
    pmgd
    pmgd-util
    protobuf
    pthread
    tiledb
    vcl
    vdms-client
    vdms-utils
    ${CMAKE_THREAD_LIBS_INIT}
    ${OpenCV_LIBS}
    ${AWSSDK_LINK_LIBRARIES}
)
