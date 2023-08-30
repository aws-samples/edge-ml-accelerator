#!/bin/bash

SUDO_PREFIX=""

# This script assumes that scripts are located in the buildscripts folder

while getopts ':oseh' opt; do
    case "$opt" in
        o)
            SKIP_OS_UPDATES="TRUE"
        ;;
        
        s)
            echo "Run OS updates as sudo. Not needed for codebuild"
            SUDO_PREFIX="sudo"
        ;;

        e)
            echo "Build only EdgeML Accelerator"
            BUILD_EDGEML_PREFIX="TRUE"
        ;;
        
        # c)
        #   arg="$OPTARG"
        #   echo "Processing option 'c' with '${OPTARG}' argument"
        #   ;;
        
        ?|h)
            echo "Usage: $(basename $0) [-s]"
            echo "-o: Skip OS installs"
            echo "-s: Run OS updates as sudo"
            echo "-e: Build EdgeML Accelerator only"
            exit 1
        ;;
    esac
done
shift "$(($OPTIND -1))"

function main()
{
    if [ "$SKIP_OS_UPDATES" = "TRUE" ]; then
        echo "Skipping OS updates"
    else
        $SUDO_PREFIX bash buildscripts/update_os.sh
    fi
    
    if [ "$BUILD_EDGEML_PREFIX" = "TRUE" ]; then
        bash buildscripts/build_edgeml.sh
    else
        bash buildscripts/build_project.sh
    fi
}

main