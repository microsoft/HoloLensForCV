from __future__ import print_function
import tensorflow as tf
gpu_options = tf.GPUOptions(per_process_gpu_memory_fraction=0.82)
sess = tf.Session(config=tf.ConfigProto(gpu_options=gpu_options))

import darkflow

from darkflow.net.build import TFNet

import argparse
import socket
import sys
import binascii
import struct
from collections import namedtuple
import cv2
import numpy as np
import matplotlib.pyplot as plt

options = {"model": "cfg/yolo.cfg", 
           "load": "bin/yolo.weights", 
           "threshold": 0.1, 
           "gpu": 1.0}

tfnet = TFNet(options)
def boxing(original_img, predictions):
    newImage = np.copy(original_img)

    for result in predictions:
        top_x = result['topleft']['x']
        top_y = result['topleft']['y']

        btm_x = result['bottomright']['x']
        btm_y = result['bottomright']['y']

        confidence = result['confidence']
        label = result['label'] + " " + str(round(confidence, 3))

        if confidence > 0.3:
            newImage = cv2.rectangle(newImage, (top_x, top_y), (btm_x, btm_y), (255,0,0), 3)
            newImage = cv2.putText(newImage, label, (top_x, top_y-5), cv2.FONT_HERSHEY_COMPLEX_SMALL , 0.8, (0, 230, 0), 1, cv2.LINE_AA)
            
    return newImage

PROCESS = True

# Definitions

# Protocol Header Format
# Cookie VersionMajor VersionMinor FrameType Timestamp ImageWidth
# ImageHeight PixelStride RowStride
SENSOR_STREAM_HEADER_FORMAT = "@IBBHqIIII"

SENSOR_FRAME_STREAM_HEADER = namedtuple(
    'SensorFrameStreamHeader',
    'Cookie VersionMajor VersionMinor FrameType Timestamp ImageWidth ImageHeight PixelStride RowStride'
)

# Each port corresponds to a single stream type
# Port for obtaining Photo Video Camera stream
PV_STREAM_PORT = 23940


def main(argv):
    """Receiver main"""
    parser = argparse.ArgumentParser()
    required_named_group = parser.add_argument_group('named arguments')

    required_named_group.add_argument("-a", "--host",
                                      help="Host address to connect", required=True)
    args = parser.parse_args(argv)

    # Create a TCP Stream socket
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    except (socket.error, msg):
        print("ERROR: Failed to create socket. Code: " + str(msg[0]) + ', Message: ' + msg[1])
        sys.exit()

    print('INFO: socket created')

    # Try connecting to the address
    s.connect((args.host, PV_STREAM_PORT))

    print('INFO: Socket Connected to ' + args.host + ' on port ' + str(PV_STREAM_PORT))

    # Try receive data
    try:
        quit = False
        while not quit:
            reply = s.recv(struct.calcsize(SENSOR_STREAM_HEADER_FORMAT))
            if not reply:
                print('ERROR: Failed to receive data')
                sys.exit()

            data = struct.unpack(SENSOR_STREAM_HEADER_FORMAT, reply)

            # Parse the header
            header = SENSOR_FRAME_STREAM_HEADER(*data)

            # read the image in chunks
            image_size_bytes = header.ImageHeight * header.RowStride
            image_data = bytes()
            
            while len(image_data) < image_size_bytes:
                remaining_bytes = image_size_bytes - len(image_data)
                image_data_chunk = s.recv(remaining_bytes)
                if not image_data_chunk:
                    print('ERROR: Failed to receive image data')
                    sys.exit()
                image_data += image_data_chunk

            image_array = np.frombuffer(image_data, dtype=np.uint8).reshape((header.ImageHeight,
                                        header.ImageWidth, header.PixelStride))
            if PROCESS:

                image_array=image_array[:,:,:3]

               
                results = tfnet.return_predict(image_array)
                new_frame = boxing(image_array, results)
               
            
            cv2.imshow('Photo Video Camera Stream',new_frame)

            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
    except KeyboardInterrupt:
        pass

    s.close()
    cv2.destroyAllWindows()
    cap.release()
    out.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main(sys.argv[1:])
    #main(192.168.100)