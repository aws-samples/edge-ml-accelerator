# Config Examples
The config JSON / YAML files can be configured in different ways based on the applicaton. More examples will be added here in order to demonstrate the different way Edge ML works.

## JSON/YAML based Configs:
### Generic Config: `example_config_GENERIC.{json,yml}`

### Acceptable variable values:
- capture
    - cameraType : `OPENCV` | `GENICAM` | `PYLON` | `GSTREAMER`
    - captureMode : (for OPENCV) `IMAGEFILEMODE` | `VIDEOFILEMODE` | `CAMERAMODE` | `GSTREAMERMODE` 
    - imageIn : (for OPENCV IMAGEFILEMODE) `/path/to/image.png`
    - videoIn : (for OPENCV VIDEOFILEMODE) `/path/to/video.mp4`
    - serialNumber : (for OPENCV GSTREAMERMODE)
        - `"filesrc location=/path/to/video.mp4 ! decodebin ! video/x-raw ! queue ! videoconvert ! appsink"`
        - `v4l2src device=/dev/video0 ! video/x-raw,format=YUY2,width=640,height=480,framerate=30/1 ! videoconvert ! video/x-raw, format=BGR ! appsink drop=1`
    - subpipelines : (default name) `pipeline1`