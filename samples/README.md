## Samples
The Edge ML Accelerator can be used in multiple ways. It can work as a standalone binary app as well as an AWS IoT Greengrass Component.

1. Using as a standalone binary App
Once the binaries are built, reference the binaries to the app and create an exectuble:
    ```
    $ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:path/to/edge-ml-accelerator/package/lib
    $ export PATH=$PATH:path/to/edge-ml-accelerator/package/bin
    $ export EDGE_ML_CONFIG='pwd'/examples/example_config_GENERIC.json
    $ cd app
    $ mkdir build && cd build
    $ cmake ..
    $ make -j4
    $ export NUMITER=25
    $ ./pipeline_app $NUMITER
    ```

2. Creating AWS IoT Greengrass Component, Publishing and Deploying it:
- This EdgeML Accelerator can be converted into an AWS IoT Greengrass Component to be deployed on Edge Devices
- Alternatively, use this method to create the GG Component:
    ```
    $ export AWS_ACCOUNT_NUM="ADD_ACCOUNT_NUMBER"
    $ export AWS_REGION="ADD_REGION"
    $ export DEV_IOT_THING="NAME_OF_OF_THING"
    $ export DEV_IOT_THING_GROUP="NAME_OF_IOT_THING_GROUP"
    $ cd greengrass_component
    $ bash buildscripts/build_deploy_gdk.sh
    ```

## Contributors:
For any issues, queries, concerns, updates, requests, kindly contact:
- [Romil Shah (@rpshah)](https://phonetool.amazon.com/users/rpshah)
- [Fabian Benitez-Quiroz (@fabianbq)](https://phonetool.amazon.com/users/fabianbq)