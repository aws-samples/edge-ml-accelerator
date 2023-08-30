#!/bin/bash

set -e

function run_build() 
{
    echo
    echo Starting build
    echo
    
    MY_INSTALL_DIR=`pwd`/package
    export PATH="$MY_INSTALL_DIR/bin:$PATH"
    echo Packages will be installed in $MY_INSTALL_DIR

    echo Building Edge ML binary
    if [ -d build ]; then rm -rf build; fi
    mkdir -p build
    pushd build
    cmake -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR -DBUILD_DOC=OFF -DBUILD_TOOLS=OFF -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TESTS=OFF -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR -DCMAKE_BUILD_TYPE=Release ..
    make -j 20
    make install
    popd

    echo Zipping Edge ML Accelerator
    zip -qor edgeml-accelerator-package.zip package/
}


function main() 
{
    run_build
}

main