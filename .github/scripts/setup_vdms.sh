#!/bin/bash -e
#######################################################################################################################
# SETUP
# Supported OS:
#            - Debian: 11(bullseye), 12(bookworm/stable), 13(trixie)
#            - Ubuntu: 20.04(focal), 22.04(jammy),        23.10(mantic), 24.04(noble)
#######################################################################################################################

BUILD_COVERAGE="OFF"
USE_K8S="OFF"
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
    "k8"
    "make"
    "python_version"
    "workspace"
)

OPTS=$(getopt \
    --options "hd:w:p:mck" \
    --long help,coverage,k8,dep_dir:,make,python_version:,workspace: \
    --name "$(basename "$0")" \
    -- "$@"
)

eval set -- $OPTS

script_usage()
{
    cat <<EOF
    This script installs OS packages, installs dependencies, and builds VDMS.

    Usage: $0 [ -h ] [ OPTIONS ]

    Options:
        -h or --help            Print this help message
        -c or --coverage        Flag to build VDMS with a test capabilities
        -d or --dep_dir         Path to directory to store dependencies
        -k or --k8              Flag to build VDMS with kubernetes orchestration capabilities
        -m or --make            Flag to build VDMS after dependency installation
        -p or --python_version  Python version to install [default: 3.12.3]
        -w or --workspace       Path to vdms repository
EOF
}

while true; do
    case "$1" in
        -h | --help) script_usage; exit 0 ;;
        -c | --coverage) BUILD_COVERAGE="ON"; shift ;;
        -d | --dep_dir) shift; VDMS_DEP_DIR=$1; shift ;;
        -k | --k8) USE_K8S="ON"; shift ;;
        -m | --make) BUILD_VDMS=true; shift ;;
        -p | --python_version) shift;
                               PYTHON_VERSION=$1;
                               PYTHON_BASE=$(echo ${PYTHON_VERSION} | cut -d. -f-2);
                               shift
                               ;;
        -w | --workspace) shift; WORKSPACE=$1; shift ;;
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
echo -e "\tUSE_K8S:\t${USE_K8S}"
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
    apt-transport-https automake bazel-bootstrap bison build-essential bzip2 ca-certificates \
    curl ed flex g++ gcc git gnupg-agent javacc libarchive-tools libatlas-base-dev \
    libavcodec-dev libavformat-dev libavutil-dev libbison-dev libboost-all-dev libbz2-dev libc-ares-dev \
    libcurl4-openssl-dev libdc1394-dev libgflags-dev libgoogle-glog-dev \
    libgtk-3-dev libgtk2.0-dev libhdf5-dev libjpeg-dev libjsoncpp-dev \
    libleveldb-dev liblmdb-dev liblz4-dev libncurses5-dev libopenblas-dev libopenmpi-dev \
    libpng-dev librdkafka-dev libsnappy-dev libssl-dev libswscale-dev libtbb-dev \
    libtiff-dev libtiff5-dev libtool libwebsockets-dev libzip-dev linux-libc-dev mpich \
    pkg-config procps software-properties-common swig uncrustify unzip uuid-dev

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
# Check the version used by python3
version_exists=$(echo "$(python3 --version | cut -d ' ' -f 2)" || echo false)
version_used_base=""
is_older_version=$((dpkg --compare-versions "${version_exists}" "lt" "${PYTHON_VERSION}" && echo true) || echo false)
# if that version is lower than the required one
if [ $is_older_version = true ]
then
    # Check if the path to the required minimum version exists
    # if it doesn't exist then it displays the error and finish the script
    if [[ "$(which python${PYTHON_BASE})" == "" ]]; then
        echo "Error: please install the Python v${PYTHON_VERSION} or later..."
        # CLEANUP
        rm -rf $VDMS_DEP_DIR
        echo "Exiting..."
        exit 1
    fi

    # If the required version of Python is installed but it is not being used currently
    # Then, the script is going to use at least the version that it is required
    version_used_base=${PYTHON_BASE}
else
    # If the current version of Python is equal or later than the required one
    echo "$(python3 --version) is already installed"
    version_used_base=$(echo ${version_exists} | cut -d. -f-2 || echo false)
fi

# It sets the Python version found (3.12 or more recent) as default
alias python=$(which python${version_used_base})
alias python3=$(which python${version_used_base})

python${version_used_base} -m venv ${VIRTUAL_ENV}
export PATH="$VIRTUAL_ENV/bin:$PATH"


if [ "${BUILD_COVERAGE}" = "ON" ]; then
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
ABSEIL_VERSION="20250512.1"
AUTOCONF_VERSION="2.71"
AWS_SDK_VERSION="1.11.336"
CMAKE_VERSION="v3.28.5"
FAISS_VERSION="v1.9.0"
GRPC_VERSION="v1.75.1"
GTEST_VERSION="52eb8108c5bdec04579160ae17225d66034bd723"
LIBEDIT_VERSION="20230828-3.1"
NUMPY_MIN_VERSION="1.26.0"
OPENCV_VERSION="4.9.0"
PEG_VERSION="0.1.19"
PROTOBUF_VERSION="6.31.1"
PROTOBUF_VERSION_COMMIT="74211c0dfc2777318ab53c2cd2c317a2ef9012de"
TILEDB_VERSION="2.14.1"
VALIJSON_VERSION="v0.6"

cd $VDMS_DEP_DIR


# INSTALL CMAKE
git clone --branch ${CMAKE_VERSION} https://github.com/Kitware/CMake.git $VDMS_DEP_DIR/CMake
cd $VDMS_DEP_DIR/CMake
./bootstrap
make ${BUILD_THREADS}
make install


# INSTALL PROTOBUF & ITS DEPENDENCIES (GOOGLETEST, ABSEIL-CPP)
git clone https://github.com/google/googletest.git $VDMS_DEP_DIR/googletest
cd $VDMS_DEP_DIR/googletest && git checkout ${GTEST_VERSION}
mkdir build && cd build/
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBUILD_GMOCK=ON -DCMAKE_CXX_STANDARD=17 ..
make ${BUILD_THREADS}
make install

git clone -b ${ABSEIL_VERSION} https://github.com/abseil/abseil-cpp.git $VDMS_DEP_DIR/abseil-cpp
cd $VDMS_DEP_DIR/abseil-cpp
mkdir build && cd build
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_INSTALL_PREFIX=/usr/local -DABSL_BUILD_TESTING=ON \
    -DABSL_USE_EXTERNAL_GOOGLETEST=ON \
    -DABSL_FIND_GOOGLETEST=ON -DCMAKE_CXX_STANDARD=17 ..
make ${BUILD_THREADS}
make install
ldconfig /usr/local/lib

git clone --recurse-submodules https://github.com/protocolbuffers/protobuf.git $VDMS_DEP_DIR/protobuf
cd $VDMS_DEP_DIR/protobuf && git checkout ${PROTOBUF_VERSION_COMMIT}
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_CXX_STANDARD=17 -Dprotobuf_BUILD_SHARED_LIBS=ON \
    -Dprotobuf_ABSL_PROVIDER=package \
    -Dprotobuf_GTEST_PROVIDER=package \
    -Dprotobuf_BUILD_TESTS=ON \
    -Dabsl_DIR=/usr/local/lib/cmake/absl .
make ${BUILD_THREADS}
make install


# INSTALL AUTOCONF
curl -L -o $VDMS_DEP_DIR/autoconf-${AUTOCONF_VERSION}.tar.gz http://ftpmirror.gnu.org/autoconf/autoconf-${AUTOCONF_VERSION}.tar.gz
cd $VDMS_DEP_DIR
tar -xzf autoconf-${AUTOCONF_VERSION}.tar.gz
cd autoconf-${AUTOCONF_VERSION}
./configure
make ${BUILD_THREADS}
make install


# INSTALL gRPC
ldconfig
git clone -b ${GRPC_VERSION} --depth 1 --recursive https://github.com/grpc/grpc $VDMS_DEP_DIR/grpc
cd $VDMS_DEP_DIR/grpc
mkdir -p cmake/build && cd cmake/build
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_CXX_STANDARD=17 -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DgRPC_ABSL_PROVIDER=package \
    -DgRPC_PROTOBUF_PROVIDER=package \
    ../..
cmake --build . -- -j
cmake --install .


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
    "protobuf==${PROTOBUF_VERSION}" "cryptography>=44.0.1"


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


# FOR KUBERNETES ORCHESTRATION
if [ "${USE_K8S}" = "ON" ]; then
    git clone --depth 1 https://github.com/yaml/libyaml.git $VDMS_DEP_DIR/libyaml
    mkdir -p $VDMS_DEP_DIR/libyaml/build
    cd $VDMS_DEP_DIR/libyaml/build
    cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=ON ..
    make ${BUILD_THREADS}
    make install

    git clone https://github.com/kubernetes-client/c.git $VDMS_DEP_DIR/k8s
    CLIENT_REPO_ROOT=$VDMS_DEP_DIR/k8s
    mkdir -p ${CLIENT_REPO_ROOT}/kubernetes/build
    cd ${CLIENT_REPO_ROOT}/kubernetes/build
    cmake -DCMAKE_PREFIX_PATH=/usr/local -DCMAKE_INSTALL_PREFIX=/usr/local ..
    make ${BUILD_THREADS}
    make install
fi


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

cmake -DUSE_K8S="${USE_K8S}" -DCODE_COVERAGE="${BUILD_COVERAGE}" ..

if [ $BUILD_VDMS == true ]; then
    make ${BUILD_THREADS}
fi

cp ${WORKSPACE}/config-vdms.json ${WORKSPACE}/build/

export PYTHONPATH=${WORKSPACE}/client/python:${PYTHONPATH}

