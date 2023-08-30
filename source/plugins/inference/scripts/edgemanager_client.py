"""
This script runs inference for Amazon SageMaker EdgeManager. This is with the assumption that the Inference Server is running.
The script takes 2 inputs: (1) dataset path, (2) results saving path

The script runs through all the images and generates the output data.
"""

import grpc
from agent_pb2 import (CaptureDataRequest, LoadModelRequest, PredictRequest, Tensor, 
        TensorMetadata, UnLoadModelRequest, ListModelsRequest, DescribeModelRequest)
import agent_pb2_grpc
import os, argparse, numpy as np, glob, time
from PIL import Image

model_path = '/greengrass/v2/work/<edgemanager component name>' 
model_name = '<edgemanager model name>'
agent_socket = 'unix:///tmp/aws.greengrass.SageMakerEdgeManager.sock'

parser = argparse.ArgumentParser()
parser.add_argument("--dataset", type=str, help="Location of the dataset", required=True, default="")
parser.add_argument("--output", type=str, help="Location of saving the output", required=True, default="")
args = parser.parse_args()

def list_models():
    response = edge_manager_client.ListModels(ListModelsRequest())
    return response

def list_model_tensors(models):
    return {
        model.name: {
            'inputs': model.input_tensor_metadatas,
            'outputs': model.output_tensor_metadatas
        }
        for model in list_models().models
    }

def predict_image(model_name, image_path):
    image_tensor = Tensor()
    image_tensor.byte_data = Image.open(image_path).tobytes()
    image_tensor_metadata = list_model_tensors(list_models())[model_name]['inputs'][0]
    image_tensor.tensor_metadata.name = image_tensor_metadata.name
    image_tensor.tensor_metadata.data_type = image_tensor_metadata.data_type
    for shape in image_tensor_metadata.shape:
        image_tensor.tensor_metadata.shape.append(shape)
    predict_request = PredictRequest()
    predict_request.name = model_name
    predict_request.tensors.append(image_tensor)
    predict_response = edge_manager_client.Predict(predict_request)
    return predict_response

channel = grpc.insecure_channel(agent_socket)
edge_manager_client = agent_pb2_grpc.AgentStub(channel)

# loading the model
try:
    response = edge_manager_client.LoadModel(LoadModelRequest(url=model_path, name=model_name))
    print(response)
except Exception as e:
    print(e)
    print('Model already loaded!')

# getting the description of the model
response = edge_manager_client.DescribeModel(DescribeModelRequest(name=model_name))
print(response)

# listing the models
list_models()

# unloading the model
response = edge_manager_client.UnLoadModel(UnLoadModelRequest(name=model_name))
print(response)

# running inference
image_files = sorted(glob.glob(args.dataset+'/*'))
for image_file in image_files:
    start_time = time.time()
    total_iters = 0

    predict_response = predict_image(model_name, image_file)
    
    print("Predict Response: ", predict_response)