#!/bin/bash

set -e

# Pin third party libraries
GRPC_VERSION_TAG="v1.46.2"
AWS_IOT_DEVICE_SDK_CPP_V2_TAG="v1.15.2"
AWS_SDK_CPP_TAG="1.9.253"
RC_GENICAM_TAG="v2.5.14"
OPENCV_TAG="4.6.0"
ONNXRUNTIME_TAG="v1.13.1"
TRITON_CLIENT_COMMIT="3d05400"
NLOHMANN_JSON_TAG="v3.11.2"


function getOperatingSystem()
{
    echo "Determining OS version"
    OS_ID=""
    OS_VERSION=""
    OS_NAME=""
    OS_ARCH=""

    # we only support linux version with /etc/os-release present
    if [ -r /etc/os-release ]; then
        # load the variables in os-release
        . /etc/os-release
        OS_ID=$ID
        OS_VERSION_ID=$VERSION_ID
        OS_NAME=$NAME
        OS_ARCH=`uname -m`
    fi

    if [ -z $OS_ID ]; then
        echo $OS_ID
        echo Cannot recognize operating system. Exiting
        exit
    fi

    echo === You are running installing on OS=$OS_NAME,ID=$OS_ID,VERSION=$OS_VERSION_ID,ARCH=$OS_ARCH ===
}

function update_submodules()
{
    echo Updating third party repositories
    echo Run the following command if submodules are not pulled
    echo git submodule update --init --recursive --depth 1

    git submodule deinit --all --force
    git submodule init
    git submodule update --recursive

    # Pin the versions of external dependencies
    # This section relies on the git submodule folder paths
    pushd third_party/grpc
    echo Switching grpc to ${GRPC_VERSION_TAG}
    git fetch --all --tags
    git checkout --detach tags/${GRPC_VERSION_TAG}
    git submodule update --init --recursive --depth 1
    git reset --hard
    popd

    pushd third_party/aws-iot-device-sdk-cpp-v2
    echo Switching aws-iot-device-sdk-cpp-v2 to ${AWS_IOT_DEVICE_SDK_CPP_V2_TAG}
    git fetch --all --tags
    git checkout --detach tags/${AWS_IOT_DEVICE_SDK_CPP_V2_TAG}
    git submodule update --init --recursive --depth 1
    git reset --hard
    popd

    pushd third_party/aws-sdk-cpp
    echo Switching aws-sdk-cpp to ${AWS_SDK_CPP_TAG}
    git fetch --all --tags
    git checkout --detach tags/${AWS_SDK_CPP_TAG}
    git submodule update --init --recursive --depth 1
    git reset --hard
    popd

    pushd third_party/rc_genicam_api
    echo Switching rc_genicam_api to ${RC_GENICAM_TAG}
    git fetch --all --tags
    git checkout --detach tags/${RC_GENICAM_TAG}
    git submodule update --init --recursive --depth 1
    git reset --hard
    popd

    pushd third_party/opencv
    echo Switching opencv to ${OPENCV_TAG}
    git fetch --all --tags
    git checkout --detach tags/${OPENCV_TAG}
    git submodule update --init --recursive --depth 1
    git reset --hard
    popd

    pushd third_party/onnxruntime
    echo Switching onnxruntime to ${ONNXRUNTIME_TAG}
    git fetch --all --tags
    git checkout --detach tags/${ONNXRUNTIME_TAG}
    git submodule update --init --recursive --depth 1
    git reset --hard
    popd

    pushd third_party/triton_client
    echo Switching triton to ${TRITON_CLIENT_COMMIT}
    git fetch --all --tags
    git checkout ${TRITON_CLIENT_COMMIT}
    git submodule update --init --recursive --depth 1
    git reset --hard
    popd

    pushd third_party/nlohmann_json
    echo Switching nlohmann/json to ${NLOHMANN_JSON_TAG}
    git fetch --all --tags
    git checkout tags/${NLOHMANN_JSON_TAG}
    git submodule update --init --recursive --depth 1
    git reset --hard
    popd
}


function run_build()
{
    echo
    echo Starting build
    echo


    rm -rf package
    mkdir -p package

    MY_INSTALL_DIR=`pwd`/package
    export PATH="$MY_INSTALL_DIR/bin:$PATH"
    echo Packages will be installed in $MY_INSTALL_DIR

    echo Building grpc
    mkdir -p grpc_build
    pushd grpc_build
    cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DgRPC_PROTOBUF_PROVIDER=module -DgRPC_ZLIB_PROVIDER=module -DgRPC_CARES_PROVIDER=module -DgRPC_ABSL_PROVIDER=module -DgRPC_RE2_PROVIDER=module -DgRPC_SSL_PROVIDER=module -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR ../third_party/grpc/
    make -j$(nproc)
    popd


    echo Building aws_sdk
    rm -rf aws_sdk_build
    mkdir -p aws_sdk_build
    pushd aws_sdk_build
    cmake -E env CXXFLAGS="-Wno-error=deprecated-declarations" cmake ../third_party/aws-sdk-cpp -DBUILD_SHARED_LIBS=ON -DENABLE_TESTING=OFF -DAUTORUN_UNIT_TESTS=OFF -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR -DCMAKE_BUILD_TYPE=Release -DBUILD_ONLY="s3;dynamodb"
    make -j$(nproc)
    popd


    echo Building aws iot device sdk
    mkdir -p aws_iot_device_sdk_cpp_v2_build
    pushd aws_iot_device_sdk_cpp_v2_build
    cmake ../third_party/aws-iot-device-sdk-cpp-v2 -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release
    make -j$(nproc)
    popd


    echo Building opencv
    mkdir -p opencv_build
    pushd opencv_build
    cmake ../third_party/opencv -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR -DCMAKE_BUILD_TYPE=Release -DCMAKE_BUILD_TESTS=OFF -DBUILD_opencv_python=OFF -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_ZLIB=OFF -DWITH_GSTREAMER=ON
    make -j$(nproc)
    popd


    echo Building onnxruntime
    mkdir -p onnxruntime_build
    pushd onnxruntime_build
    if [ "$OS_ARCH" == "aarch64" ] && [ -x "$(command -v nvcc)" ]; then
        if [ -x "$(command -v /usr/src/tensorrt/bin/trtexec)" ]; then
            echo "TensorRT already installed"
        else
            echo "Installing TensorRT"
            apt-get install tensorrt
        fi
        echo "Building onnxruntime for CUDA system"
        cmake ../third_party/onnxruntime/cmake -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR -DPYTHON_EXECUTABLE=$(which python3) -DCMAKE_BUILD_TYPE=Release -Donnxruntime_BUILD_SHARED_LIB=ON -DONNX_USE_PROTOBUF_SHARED_LIBS=ON -DBUILD_ONNX_PYTHON=OFF -Donnxruntime_USE_CUDA=ON -Donnxruntime_CUDA_HOME=/usr/local/cuda -Donnxruntime_CUDNN_HOME=/usr/lib/aarch64-linux-gnu -Donnxruntime_TENSORRT_HOME=/usr/lib/aarch64-linux-gnu
    else
        echo "Building onnxruntime for CPU system"
        cmake ../third_party/onnxruntime/cmake -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR -DPYTHON_EXECUTABLE=$(which python3) -DCMAKE_BUILD_TYPE=Release -Donnxruntime_BUILD_SHARED_LIB=ON -DONNX_USE_PROTOBUF_SHARED_LIBS=ON -DBUILD_ONNX_PYTHON=OFF
    fi
    make -j$(nproc)
    popd


    #   - mkdir pylon_package && cd pylon_package
    #   - aws s3 cp s3://edge-ml-accelerator-prebuilt-packages/pylon-packages/pylon_6.2.0.21487_aarch64.tar.gz .
    #   - tar -C . -xzf ./pylon_*.tar.gz && rm pylon_*.tar.gz
    #   - chmod 755 ${CODEBUILD_SRC_DIR}/pylon_package
    #   - export PATH=$PATH:${CODEBUILD_SRC_DIR}/pylon_package/bin && export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${CODEBUILD_SRC_DIR}/pylon_package/lib
    #   - cd ${CODEBUILD_SRC_DIR}
    #   - cp -r pylon_package package/.


    pushd grpc_build
    make install
    popd

    pushd aws_iot_device_sdk_cpp_v2_build
    make install
    popd

    pushd aws_sdk_build
    make install
    popd

    pushd opencv_build
    make install
    popd

    #triton client needs other packages to be on the install lib
    echo Building triton client
    mkdir -p triton_client_build
    pushd triton_client_build
    cmake ../third_party/triton_client/src/c++ -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR -DTRITON_ENABLE_CC_GRPC=ON -DTRITON_USE_THIRD_PARTY=OFF -DTRITON_KEEP_TYPEINFO=ON -DCMAKE_CXX_STANDARD=11
    make -j$(nproc) && make install
    popd

    pushd onnxruntime_build
    make install
    popd

    echo Building Nholmann/Json
    mkdir -p nlohmann_json_build
    pushd nlohmann_json_build
    cmake ../third_party/nlohmann_json/ -DJSON_BuildTests=OFF -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR
    make -j$(nproc) && make install
    popd


    echo Building Edge ML binary
    if [ -d build ]; then rm -rf build; fi
    mkdir -p build
    pushd build
    cmake -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR -DBUILD_DOC=OFF -DBUILD_TOOLS=OFF -DBUILD_SHARED_LIBS=ON -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc)
    make install
    popd

    echo Zipping Edge ML Accelerator
    zip -qor edgeml-accelerator-package.zip package/
}



function main()
{
    getOperatingSystem
    update_submodules
    run_build
}

main
