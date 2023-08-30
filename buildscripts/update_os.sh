#!/bin/bash


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



function update_centos_packages()
{
    yum update -y
    dnf install https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm -y
    yum install epel-release -y
    yum install -y git zip unzip gcc gcc-c++ kernel-devel make wget curl libpng-devel autoconf libtool-ltdl-devel pkg-config libcurl-devel openssl-devel zlib-devel pulseaudio-libs-devel libuuid-devel gstreamer1 gstreamer1-devel gstreamer python3 python3-pip
    cmake --version
}


function update_amzn_packages()
{
    yum update -y
    amazon-linux-extras install epel -y
    yum install -y git zip unzip tar gcc gcc-c++ kernel-devel make wget curl libpng-devel autoconf libtool-ltdl-devel pkg-config libcurl-devel openssl-devel zlib-devel pulseaudio-libs-devel libuuid-devel gstreamer1 gstreamer1-devel gstreamer python3 python3-pip
    wget https://github.com/Kitware/CMake/releases/download/v3.22.2/cmake-3.22.2.tar.gz
    tar -zxvf cmake-3.22.2.tar.gz && rm cmake-3.22.2.tar.gz
    pushd cmake-3.22.2 && ./bootstrap && make -j$(nproc) && make install && popd && rm -rf cmake-3.22.2
    cmake --version
}


function update_ubuntu_packages()
{
    apt update -y    
    apt install -y git zip unzip build-essential wget curl libpng-dev autoconf libtool pkg-config libcurl4-openssl-dev libssl-dev uuid-dev zlib1g-dev libpulse-dev software-properties-common coreutils libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev python3 python3-pip
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
    apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"
    apt update && apt install -y kitware-archive-keyring && rm /etc/apt/trusted.gpg.d/kitware.gpg
    apt install -y cmake
    cmake --version
}


function update_packages() 
{
    case $OS_ID in
        
        "centos")
            echo -n "Updating CentOS packages"
            update_centos_packages
        ;;

        "amzn")
            echo -n "Updating AmazonLinux2 packages"
            update_amzn_packages
        ;;
        
        "ubuntu")
            echo -n "Updating Ubuntu packages"
            update_ubuntu_packages
        ;;
        
        *)
            echo Cannot recognize operating system. Exiting
            exit
            
    esac

    if [ -x "$(command -v aws)" ]; then
        echo "AWS Installed already"
    else
        echo "Installing AWS CLI"    
        curl "https://awscli.amazonaws.com/awscli-exe-linux-$(arch).zip" -o "awscliv2.zip" && unzip awscliv2.zip -d awscliv2 && ./awscliv2/aws/install --update && rm awscliv2.zip && rm -rf awscliv2
    fi

    if [ $? -eq 0 ]; then
        aws configure 2>&1 || echo "aws configure failed"
    fi
}



function main() 
{
    getOperatingSystem
    update_packages
}


main