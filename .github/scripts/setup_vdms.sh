#!/bin/bash -e
#######################################################################################################################
# SETUP
# Supported OS:
#            - Debian: 11(bullseye), 12(bookworm/stable), 13(trixie)
#            - Ubuntu: 20.04(focal), 22.04(jammy),        23.10(mantic), 24.04(noble)
#######################################################################################################################

BUILD_COVERAGE="off"
BUILD_THREADS="-j16"
DEBIAN_FRONTEND=noninteractive
CUR_DIR=$(dirname $(realpath  "$0"))
WORKSPACE=$(dirname $(dirname  ${CUR_DIR}))
VDMS_DEP_DIR=/dependencies
BUILD_VDMS=false
OS_NAME=$(awk -F= '$1=="ID" { print $2 ;}' /etc/os-release)
OS_VERSION=$(awk -F= '$1=="VERSION_ID" { print $2 ;}' /etc/os-release | sed -e 's|"||g')
MODIFY_PMGD=false
PYTHON_VERSION="3.12.3"
PYTHON_BASE=$(echo ${PYTHON_VERSION} | cut -d. -f-2)  #"3.12"
VIRTUAL_ENV=/opt/venv

LONG_LIST=(
    "help"
    "coverage"
    "dep_dir"
    "make"
    "python_version"
    "workspace"
)

OPTS=$(getopt \
    --options "hd:w:p:mc" \
    --long help,coverage,dep_dir:,make,python_version:,workspace: \
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
        -w or --workspace   Path to vdms repository
        -d or --dep_dir     Path to directory to store dependencies
        -m or --make        Flag to build VDMS after dependency installation
        -c or --coverage        Flag to build VDMS with a test capabilities
        -p or --python_version  Python version to install [default: 3.12.3]
EOF
}

while true; do
    case "$1" in
        -h | --help) script_usage; exit 0 ;;
        -w | --workspace) shift; WORKSPACE=$1; shift ;;
        -c | --coverage) BUILD_COVERAGE="on"; shift ;;
        -d | --dep_dir) shift; VDMS_DEP_DIR=$1; shift ;;
        -p | --python_version) shift;
                               PYTHON_VERSION=$1;
                               PYTHON_BASE=$(echo ${PYTHON_VERSION} | cut -d. -f-2);
                               shift
                               ;;
        -m | --make) BUILD_VDMS=true; shift ;;
        --) shift; break ;;
        *) script_usage; exit 0 ;;
    esac
done

# OS should be lowercase
OS_NAME="${OS_NAME,,}"

echo "Preparing system for VDMS..."
echo "Arguments used: "
echo -e "\tOS_NAME:\t${OS_NAME}"
echo -e "\tOS_VERSION:\t${OS_VERSION}"
echo -e "\tWORKSPACE:\t${WORKSPACE}"
echo -e "\tVDMS_DEP_DIR:\t${VDMS_DEP_DIR}"
echo -e "\tBUILD_COVERAGE:\t${BUILD_COVERAGE}"
echo -e "\tBUILD_VDMS:\t${BUILD_VDMS}"
echo -e "\tPYTHON_BASE:\t${PYTHON_BASE}"
echo -e "\tPYTHON_VERSION:\t${PYTHON_VERSION}"
echo -e "\tVIRTUAL_ENV:\t${VIRTUAL_ENV}"

mkdir -p $VDMS_DEP_DIR

#######################################################################################################################
# INSTALL PACKAGES
#######################################################################################################################

apt-get update -y && apt-get upgrade -y
apt-get install -o 'Acquire::Retries=3' -y --no-install-suggests \
        --no-install-recommends --fix-broken --fix-missing \
    apt-transport-https automake bison build-essential bzip2 ca-certificates \
    curl ed flex g++ gcc git gnupg-agent javacc libarchive-tools libatlas-base-dev \
    libavcodec-dev libavformat-dev libavutil-dev libboost-all-dev libbz2-dev libc-ares-dev \
    libcurl4-openssl-dev libdc1394-dev libgflags-dev libgoogle-glog-dev \
    libgtk-3-dev libgtk2.0-dev libhdf5-dev libjpeg-dev libjsoncpp-dev \
    libleveldb-dev liblmdb-dev liblz4-dev libncurses5-dev libopenblas-dev libopenmpi-dev \
    libpng-dev librdkafka-dev libsnappy-dev libssl-dev libswscale-dev libtbb-dev \
    libtiff-dev libtiff5-dev libtool libzip-dev linux-libc-dev mpich \
    pkg-config procps software-properties-common swig unzip uuid-dev

if [ ${OS_NAME} = "debian" ]; then
    apt-get install -y --no-install-suggests --no-install-recommends libjpeg62-turbo-dev

    if [ ${OS_VERSION} = "11" ]; then
        apt-get install -y --no-install-suggests --no-install-recommends libtbb2 openjdk-11-jdk-headless libzmq3-dev
        OLD_AV_LIBS=true
    else
        apt-get install -y --no-install-suggests --no-install-recommends libtbbmalloc2 openjdk-17-jdk-headless cppzmq-dev
        MODIFY_PMGD=true
    fi

elif [ ${OS_NAME} = "ubuntu" ]; then
    apt-get install -y --no-install-suggests --no-install-recommends libjpeg8-dev libzmq3-dev

    if [ ${OS_VERSION} = "20.04" ]; then
        apt-get install -y --no-install-suggests --no-install-recommends libtbb2 openjdk-11-jdk-headless
        OLD_AV_LIBS=true
    else
        apt-get install -y --no-install-suggests --no-install-recommends libtbbmalloc2 openjdk-17-jdk-headless
        MODIFY_PMGD=true
    fi

else
    echo "Invalid OS provide. Must be debian or ubuntu"
    exit 1;

fi


# SETUP PYTHON VERSION
version_exists=$(echo "$(python${PYTHON_BASE} --version | cut -d ' ' -f 2)" || echo false)
if [ "${version_exists}" != "${PYTHON_VERSION}" ]
then
    echo "Installing python ${PYTHON_VERSION}..."
    apt update -y
    apt install -y libffi-dev libgdbm-dev libnss3-dev libreadline-dev libsqlite3-dev zlib1g-dev
    curl -L -o ${VDMS_DEP_DIR}/Python-${PYTHON_VERSION}.tgz https://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tgz
    cd ${VDMS_DEP_DIR}
    tar -xzf Python-${PYTHON_VERSION}.tgz
    cd Python-${PYTHON_VERSION}
    ./configure --enable-optimizations && make -j && make altinstall
else
    echo "python ${PYTHON_VERSION} already installed"
fi
alias python=$(which python${PYTHON_BASE})
alias python3=$(which python${PYTHON_BASE})

python${PYTHON_BASE} -m venv ${VIRTUAL_ENV}
export PATH="$VIRTUAL_ENV/bin:$PATH"


if [ "${BUILD_COVERAGE}" = "on" ]; then
    apt-get install -y --no-install-suggests --no-install-recommends gdb
    python -m pip install --no-cache-dir "gcovr>=7.0"
    curl -L -o ${WORKSPACE}/minio https://dl.min.io/server/minio/release/linux-amd64/minio
    chmod +x ${WORKSPACE}/minio
    mkdir -p ${WORKSPACE}/minio_files/minio-bucket
    mkdir -p ${WORKSPACE}/tests/coverage_report

    # Install the MinIO Client mc command line tool used by scripts for creating buckets
    curl -o /usr/local/bin/mc https://dl.min.io/client/mc/release/linux-amd64/mc
    chmod +x /usr/local/bin/mc
fi

#######################################################################################################################
# INSTALL DEPENDENCIES
#######################################################################################################################
AUTOCONF_VERSION="2.71"
AWS_SDK_VERSION="1.11.336"
CMAKE_VERSION="v3.28.5"
FAISS_VERSION="v1.7.4"
LIBEDIT_VERSION="20230828-3.1"
NUMPY_MIN_VERSION="1.26.0"
OPENCV_VERSION="4.9.0"
PEG_VERSION="0.1.19"
PROTOBUF_VERSION="24.2"
TILEDB_VERSION="2.14.1"
VALIJSON_VERSION="v0.6"

cd $VDMS_DEP_DIR


# INSTALL CMAKE
git clone --branch ${CMAKE_VERSION} https://github.com/Kitware/CMake.git $VDMS_DEP_DIR/CMake
cd $VDMS_DEP_DIR/CMake
./bootstrap
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


# INSTALL OPENCV
git clone https://github.com/opencv/opencv.git $VDMS_DEP_DIR/opencv
cd $VDMS_DEP_DIR/opencv
git checkout tags/${OPENCV_VERSION}
mkdir build && cd build
cmake -DBUILD_PERF_TESTS=OFF -DBUILD_TESTS=OFF ..
make ${BUILD_THREADS}
make install


# INSTALL PYTHON PACKAGES
python -m pip install --no-cache-dir "numpy>=${NUMPY_MIN_VERSION},<2.0.0" "coverage>=7.3.1" \
    "protobuf==4.${PROTOBUF_VERSION}" "cryptography>=42.0.7"


# INSTALL VALIJSON
git clone --branch ${VALIJSON_VERSION} https://github.com/tristanpenman/valijson.git $VDMS_DEP_DIR/valijson
cd $VDMS_DEP_DIR/valijson
cp -r include/* /usr/local/include/


# INSTALL DESCRIPTOR LIBRARIES (FAISS, FLINNG)
git clone --branch ${FAISS_VERSION} https://github.com/facebookresearch/faiss.git $VDMS_DEP_DIR/faiss
cd $VDMS_DEP_DIR/faiss
mkdir build && cd build
cmake -DFAISS_ENABLE_GPU=OFF -DPython_EXECUTABLE=$(which python) \
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
make install-tiledb


# INSTALL AWS S3 SDK
git clone -b ${AWS_SDK_VERSION} --recurse-submodules https://github.com/aws/aws-sdk-cpp $VDMS_DEP_DIR/aws-sdk-cpp
mkdir -p $VDMS_DEP_DIR/aws-sdk-cpp/build
cd $VDMS_DEP_DIR/aws-sdk-cpp/build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/usr/local/ -DCMAKE_INSTALL_PREFIX=/usr/local/ \
    -DBUILD_ONLY="s3" -DCUSTOM_MEMORY_MANAGEMENT=OFF -DENABLE_TESTING=OFF
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
make clean check
make install -w --debug


# CLEANUP
rm -rf $VDMS_DEP_DIR

#######################################################################################################################
# BUILD VDMS
#######################################################################################################################

cd ${WORKSPACE} && git submodule update --init --recursive

if [ ${MODIFY_PMGD} == true ]; then
    sed -i "s|java-11-openjdk|java-17-openjdk|g" ${WORKSPACE}/src/pmgd/java/CMakeLists.txt
    sed -i "s|#include <stdio.h>|#include <stdio.h>\n#include <stdexcept>|" ${WORKSPACE}/src/pmgd/test/neighbortest.cc
    sed -i "s|#include <stdio.h>|#include <stdio.h>\n#include <stdexcept>|" ${WORKSPACE}/src/pmgd/tools/mkgraph.cc
fi

if [ ${OLD_AV_LIBS} == true ]; then
    sed -i "s|#include <libavcodec/avcodec.h>||" ${WORKSPACE}/include/vcl/KeyFrame.h
    sed -i "s|#include <libavcodec/bsf.h>||" ${WORKSPACE}/include/vcl/KeyFrame.h
fi

mkdir -p ${WORKSPACE}/build && cd ${WORKSPACE}/build

cmake -DCODE_COVERAGE="${BUILD_COVERAGE}" ..

if [ $BUILD_VDMS == true ]; then
    make ${BUILD_THREADS}
fi

cp ${WORKSPACE}/config-vdms.json ${WORKSPACE}/build/

export PYTHONPATH=${WORKSPACE}/client/python:${PYTHONPATH}

