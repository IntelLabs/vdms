cmake_minimum_required (VERSION 3.10)

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

link_directories(/usr/local/lib /usr/lib/x86_64-linux-gnu/)
include_directories(
    ../src
    ../include/
    ../utils/include/
    ../src/vcl
    /usr/include/jsoncpp
    ../src/pmgd/include
    ../src/pmgd/util
)

add_executable(unit_test
    main.cc
    server/json_queries.cc
    unit_tests/pmgd_queries.cc
    unit_tests/helpers.cc
    unit_tests/TDBImage_test.cc
    unit_tests/Image_test.cc
    unit_tests/Video_test.cc
    unit_tests/DescriptorSetAdd_test.cc
    unit_tests/DescriptorSetClassify_test.cc
    unit_tests/DescriptorSetTrain_test.cc
    unit_tests/DescriptorSetReadFS_test.cc
    unit_tests/DescriptorSetStore_test.cc
)

target_link_libraries(unit_test
    avcodec
    avformat
    dms
    faiss
    gtest
    jsoncpp
    pmgd
    pmgd-util
    protobuf
    pthread
    tiledb
    vcl
    vdms-utils
    ${CMAKE_THREAD_LIBS_INIT}
    ${OpenCV_LIBS}
)
