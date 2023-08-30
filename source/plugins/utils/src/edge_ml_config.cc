#include <edge-ml-accelerator/utils/edge_ml_config.h>

const char *CaptureTypesE[] = {"CAMERAMODE", "IMAGEFILEMODE", "VIDEOFILEMODE", "GSTREAMERMODE"};
const char *ModelTypesE[] = {"LFVE", "EDGEMANAGER", "ONNX", "TRITON", "NONE"};
const char *LfveModelStatusE[] = {"STOPPED", "STARTING", "RUNNING", "FAILED", "STOPPING"};
const char *EdgeManagerModelStatusE[] = {"OK", "UNKNOWN", "INTERNAL", "NOT_FOUND"};
const char *EdgeManagerModelDataTypeE[] = {"UINT8", "INT16", "INT32", "INT64", "FLOAT16", "FLOAT32", "FLOAT64"};
