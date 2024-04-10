#!/bin/bash -e


#######################################################################################################################
# SETUP
#######################################################################################################################

BUILD_COVERAGE="off"
BUILD_THREADS="-j16"
DEBIAN_FRONTEND=noninteractive
WORKSPACE=/vdms
VDMS_DEP_DIR=/dependencies
BUILD_VDMS=false
OS_NAME=debian

LONG_LIST=(
    "help"
    "coverage"
    "dep_dir"
    "make"
    "os_name"
    "workspace"
)

OPTS=$(getopt \
    --options "ho:d:w:mc" \
    --long help,os_name:,coverage,dep_dir:,make,workspace: \
    --name "$(basename "$0")" \
    -- "$@"
)

eval set -- $OPTS

script_usage()
{
    cat <<EOF
    This script installs OS packages, installs dependencies, and builds VDMS.

    Usage: $0 [ -h ] [ -o OS_NAME ] [ -w WORKSPACE ] [ -d VDMS_DEP_DIR ] [ -m ] [ -c ]

    Options:
        -h or --help        Print this help message
        -o or --os_name     OS name for installation [debian, ubuntu]
        -w or --workspace   Path to vdms repository
        -d or --dep_dir     Path to directory to store dependencies
        -m or --make        Flag to build VDMS after dependency installation
        -c or --coverage    Flag to build VDMS with a test capabilities
EOF
}

while true; do
    case "$1" in
        -h | --help) script_usage; exit 0 ;;
        -o | --os_name) shift; OS_NAME=$1; shift ;;
        -w | --workspace) shift; WORKSPACE=$1; shift ;;
        -c | --coverage) BUILD_COVERAGE="on"; shift ;;
        -d | --dep_dir) shift; VDMS_DEP_DIR=$1; shift ;;
        -m | --make) BUILD_VDMS=true; shift ;;
        --) shift; break ;;
        *) script_usage; exit 0 ;;
    esac
done

# OS should be lowercase
OS_NAME="${OS_NAME,,}"

#######################################################################################################################
# INSTALL PACKAGES
#######################################################################################################################

apt-get update -y && apt-get upgrade -y
apt-get install -o 'Acquire::Retries=3' -y --no-install-suggests --no-install-recommends --fix-broken --fix-missing \
    apt-transport-https automake bison build-essential bzip2 ca-certificates \
    curl ed flex g++-9 gcc-9 git gnupg-agent javacc libarchive-tools libatlas-base-dev \
    libavcodec-dev libavformat-dev libboost-all-dev libbz2-dev libc-ares-dev libcurl4-openssl-dev \
    libncurses5-dev libdc1394-dev libgflags-dev libgoogle-glog-dev libgtk-3-dev libgtk2.0-dev \
    libhdf5-dev libjpeg-dev libjpeg8-dev libjsoncpp-dev libleveldb-dev liblmdb-dev \
    liblz4-dev libopenblas-dev libopenmpi-dev libpng-dev librdkafka-dev libsnappy-dev libssl-dev \
    libswscale-dev libtbb-dev libtbb2 libtiff-dev libtiff5-dev libtool libzmq3-dev linux-libc-dev mpich \
    openjdk-11-jdk-headless pkg-config procps python3-dev python3-pip software-properties-common \
    swig unzip uuid-dev
if [ ${OS_NAME} = "debian" ]; then
    apt-get install -y --no-install-suggests --no-install-recommends libdc1394-22-dev libjpeg62-turbo-dev
elif [ ${OS_NAME} = "ubuntu" ]; then
    apt-get install -y --no-install-suggests --no-install-recommends libdc1394-dev libjpeg8-dev
else
    echo "Invalid OS provide. Must be debian or ubuntu"
    exit 1;

fi
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 1
update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 1
apt-get clean && rm -rf /var/lib/apt/lists/*
ln -s /usr/bin/python3 /usr/bin/python

#######################################################################################################################
# INSTALL DEPENDENCIES
#######################################################################################################################
AUTOCONF_VERSION="2.71"
AWS_SDK_VERSION="1.11.0"
CMAKE_VERSION="v3.27.2"
FAISS_VERSION="v1.7.4"
LIBEDIT_VERSION="20230828-3.1"
NUMPY_MIN_VERSION="1.26.0"
OPENCV_VERSION="4.5.5"
PEG_VERSION="0.1.19"
PROTOBUF_VERSION="24.2"
TILEDB_VERSION="2.14.1"
VALIJSON_VERSION="v0.6"

mkdir -p $VDMS_DEP_DIR
cd $VDMS_DEP_DIR

# INSTALL NUMPY
python3 -m pip install --no-cache-dir "numpy>=${NUMPY_MIN_VERSION}" "coverage>=7.3.1"


# INSTALL VALIJSON
git clone --branch ${VALIJSON_VERSION} https://github.com/tristanpenman/valijson.git $VDMS_DEP_DIR/valijson
cd $VDMS_DEP_DIR/valijson
cp -r include/* /usr/local/include/


# INSTALL CMAKE
git clone --branch ${CMAKE_VERSION} https://github.com/Kitware/CMake.git $VDMS_DEP_DIR/CMake
cd $VDMS_DEP_DIR/CMake
./bootstrap
make ${BUILD_THREADS}
make install


# INSTALL AUTOCONF
curl -L -o $VDMS_DEP_DIR/autoconf-${AUTOCONF_VERSION}.tar.xz https://ftp.gnu.org/gnu/autoconf/autoconf-${AUTOCONF_VERSION}.tar.xz
cd $VDMS_DEP_DIR
tar -xf autoconf-${AUTOCONF_VERSION}.tar.xz
cd autoconf-${AUTOCONF_VERSION}
./configure
make ${BUILD_THREADS}
make install


# INSTALL PROTOBUF & ITS DEPENDENCIES
git clone -b "v${PROTOBUF_VERSION}" --recurse-submodules https://github.com/protocolbuffers/protobuf.git $VDMS_DEP_DIR/protobuf
cd $VDMS_DEP_DIR/protobuf/third_party/googletest
mkdir build && cd build/
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBUILD_GMOCK=ON -DCMAKE_CXX_STANDARD=17 ..
make ${BUILD_THREADS}
make install

cd $VDMS_DEP_DIR/protobuf/third_party/abseil-cpp
mkdir build && cd build
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_INSTALL_PREFIX=/usr/local -DABSL_BUILD_TESTING=ON \
    -DABSL_USE_EXTERNAL_GOOGLETEST=ON \
    -DABSL_FIND_GOOGLETEST=ON -DCMAKE_CXX_STANDARD=17 ..
make ${BUILD_THREADS}
make install
ldconfig /usr/local/lib

cd $VDMS_DEP_DIR/protobuf
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_CXX_STANDARD=17 -Dprotobuf_BUILD_SHARED_LIBS=ON \
    -Dprotobuf_ABSL_PROVIDER=package \
    -Dprotobuf_BUILD_TESTS=ON \
    -Dabsl_DIR=/usr/local/lib/cmake/absl .
make ${BUILD_THREADS}
make install
python3 -m pip install --no-cache-dir "protobuf==4.${PROTOBUF_VERSION}"


# INSTALL DESCRIPTOR LIBRARIES (FAISS, FLINNG)
git clone --branch ${FAISS_VERSION} https://github.com/facebookresearch/faiss.git $VDMS_DEP_DIR/faiss
cd $VDMS_DEP_DIR/faiss
mkdir build && cd build
cmake -DFAISS_ENABLE_GPU=OFF -DPython_EXECUTABLE=/usr/bin/python3 \
    -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release ..
make ${BUILD_THREADS}
make install

git clone https://github.com/tonyzhang617/FLINNG.git $VDMS_DEP_DIR/FLINNG
cd $VDMS_DEP_DIR/FLINNG
mkdir build && cd build
cmake ..
make ${BUILD_THREADS}
make install


# INSTALL TILEDB
curl -L -o $VDMS_DEP_DIR/${TILEDB_VERSION}.tar.gz https://github.com/TileDB-Inc/TileDB/archive/refs/tags/${TILEDB_VERSION}.tar.gz
cd $VDMS_DEP_DIR
tar -xvf ${TILEDB_VERSION}.tar.gz
cd TileDB-${TILEDB_VERSION}
mkdir build && cd build
../bootstrap --prefix=/usr/local/
make ${BUILD_THREADS}
sudo make install-tiledb


# INSTALL AWS S3 SDK
git clone -b ${AWS_SDK_VERSION} --recurse-submodules https://github.com/aws/aws-sdk-cpp $VDMS_DEP_DIR/aws-sdk-cpp
mkdir -p $VDMS_DEP_DIR/aws-sdk-cpp/build
cd $VDMS_DEP_DIR/aws-sdk-cpp/build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/usr/local/ -DCMAKE_INSTALL_PREFIX=/usr/local/ \
    -DBUILD_ONLY="s3" -DCUSTOM_MEMORY_MANAGEMENT=OFF -DENABLE_TESTING=OFF
make ${BUILD_THREADS}
make install


# INSTALL OPENCV
git clone --branch ${OPENCV_VERSION} https://github.com/opencv/opencv.git $VDMS_DEP_DIR/opencv
cd $VDMS_DEP_DIR/opencv
mkdir build && cd build
cmake -D BUILD_PERF_TESTS=OFF -D BUILD_TESTS=OFF ..
make ${BUILD_THREADS}
sudo make install


# INSTALL NEO4J CLIENTS
curl -L -o $VDMS_DEP_DIR/peg-${PEG_VERSION}.tar.gz https://github.com/gpakosz/peg/releases/download/${PEG_VERSION}/peg-${PEG_VERSION}.tar.gz
cd $VDMS_DEP_DIR/
tar -xf peg-${PEG_VERSION}.tar.gz
cd peg-${PEG_VERSION}
make ${BUILD_THREADS}
make install

git clone https://github.com/cleishm/libcypher-parser.git $VDMS_DEP_DIR/libcypher
cd $VDMS_DEP_DIR/libcypher
./autogen.sh
./configure
make install

curl -L -o $VDMS_DEP_DIR/libedit-${LIBEDIT_VERSION}.tar.gz https://thrysoee.dk/editline/libedit-${LIBEDIT_VERSION}.tar.gz
cd $VDMS_DEP_DIR/
tar -xzf libedit-${LIBEDIT_VERSION}.tar.gz
cd libedit-${LIBEDIT_VERSION}
./configure
make ${BUILD_THREADS}
make install

git clone https://github.com/majensen/libneo4j-omni.git $VDMS_DEP_DIR/libomni
cd $VDMS_DEP_DIR/libomni
./autogen.sh
./configure --disable-werror --prefix=/usr
make install -w --debug


# CLEANUP
rm -rf $VDMS_DEP_DIR /usr/local/share/doc /usr/local/share/man

#######################################################################################################################
# BUILD VDMS
#######################################################################################################################

mkdir -p ${WORKSPACE}/build && cd ${WORKSPACE}/build

cmake -DCODE_COVERAGE="${BUILD_COVERAGE}" ..

if [ $BUILD_VDMS == true ]; then
    make ${BUILD_THREADS}
fi

cp ${WORKSPACE}/config-vdms.json ${WORKSPACE}/build/

export PYTHONPATH=${WORKSPACE}/client/python:${PYTHONPATH}

