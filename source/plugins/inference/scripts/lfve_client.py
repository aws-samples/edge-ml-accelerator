"""
This script runs inference for Lookout For Vision on Edge. This is with the assumption that the Inference Server is running.
The script takes 2 inputs: (1) dataset path, and (1) type
The dataset path directory structure is expected as following:

    dataset:
            |- training
            |           |- anomaly
            |           |- normal
            |- validation
            |           |- anomaly
            |           |- normal

The script runs through all the images and generates the Confusion Matrix, Precision, Recall, Accuracy and the F1-Score.
"""

import grpc, numpy, sys, os, cv2, time, argparse, glob
from tqdm import tqdm

from lyra_edge_inference_server_model.inference_server_pb2_grpc import (InferenceServerStub)
import lyra_edge_inference_server_model.inference_server_pb2 as pb2

# function to get the PRECISION using the confusion matrix params
def precision(TP, FP, TN, FN):
    delta = 0.00001
    return (TP + delta)/(TP + FP + delta)

# function to get the RECALL using the confusion matrix params
def recall(TP, FP, TN, FN):
    delta = 0.00001
    return (TP + delta)/(TP + FN + delta)

# function to get the ACCURACY using the confusion matrix params
def accuracy(TP, FP, TN, FN):
    delta = 0.00001
    return (TP + TN + delta)/(TP + TN + FP + FN + delta)

# function to get the F1-SCORE using the confusion matrix params
def f1_score(TP, FP, TN, FN):
    delta = 0.00001
    p = precision(TP, FP, TN, FN)
    r = recall(TP, FP, TN, FN)
    return 2*(r*p)/(r + p)

parser = argparse.ArgumentParser()
parser.add_argument("--type", type=str, help="Select from type (abc/xyz)", required=True, default="abc") # if using different types of dataset to test multiple models
parser.add_argument("--dataset", type=str, help="Location of the dataset", required=True, default="")
args = parser.parse_args()

# selecting the server based on the type of dataset selected so selecting the right model for it
if args.type=='abc':
    channel = grpc.insecure_channel("<unix-abstract : server socket #1>")
    model_id = "<model arn #1>"
elif args.type=='xyz':
    channel = grpc.insecure_channel("<unix-abstract : server socket #2>")
    model_id = "<model arn #1>"
    
stub = InferenceServerStub(channel)

# running for both validation and training dataset
sub_dataset = ['validation', 'training']
# running for both anomaly and normal dataset
categories = ['anomaly', 'normal']

for sub_ds in sub_dataset:
    start_time = time.time()
    total_iters = 0

    TP, FP, TN, FN = 0, 0, 0, 0
    
    for ctg in categories:
        image_fnames = sorted(glob.glob(args.dataset + '/' + sub_ds + '/' + ctg + '/*'))

        for fname in tqdm(image_fnames):
            image = cv2.imread(fname)
            image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

            result = stub.DetectAnomalies(pb2.DetectAnomaliesRequest(model_id=model_id, bitmap=pb2.Bitmap(width=image.shape[1], height=image.shape[0], bitmap_bytes=bytes(image.tobytes()))))

            if result.detect_anomaly_result.is_anomalous:
                if ctg=='normal':
                    FP += 1
                elif ctg=='anomaly':
                    TP += 1
            elif not result.detect_anomaly_result.is_anomalous:
                if ctg=='normal':
                    TN += 1
                elif ctg=='anomaly':
                    FN += 1
            
            total_iters += 1
    end_time = time.time()

    precision_value = precision(TP, FP, TN, FN)
    recall_value = recall(TP, FP, TN, FN)
    accuracy_value = accuracy(TP, FP, TN, FN)
    f1_score_value = f1_score(TP, FP, TN, FN)

    print(" ")
    print("Results for " + args.type + "[" + sub_ds + "]:")

    print("|---------------------------|")
    print("|     P R E D I C T E D     |")
    print("|---------------------------|")
    print("|       |   NEG   |   POS   |")
    print("|-------|---------|---------|")
    print("|   | N |         |         |")
    print("| A | E |  {:4}   |  {:4}   |".format(TN,FP))
    print("| C | G |         |         |")
    print("| T |___|_________|_________|")
    print("| U | P |         |         |")
    print("| A | O |  {:4}   |  {:4}   |".format(FN,TP))
    print("| L | S |         |         |")
    print("|   |   |         |         |")
    print("|---------------------------|")

    print("Precision:", precision_value)
    print("Recall:", recall_value)
    print("F1 Score:", f1_score_value)
    print("Accuracy:", accuracy_value)
    print("Average Latency per image (seconds):", (end_time - start_time)/total_iters)
    print(" ")