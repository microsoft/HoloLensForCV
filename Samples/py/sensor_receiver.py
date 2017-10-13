"""
 Copyright (c) Microsoft. All rights reserved.

 This code is licensed under the MIT License (MIT).
 THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
 ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
 IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
 PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
"""

""" Sample code to access HoloLens Research mode sensor stream """
# pylint: disable=C0103

import argparse
import socket
import sys
import binascii
import struct
from collections import namedtuple
import cv2
import numpy as np

PROCESS = True

# Definitions

# Protocol Header Format
# Cookie VersionMajor VersionMinor FrameType Timestamp ImageWidth
# ImageHeight PixelStride RowStride
SENSOR_STREAM_HEADER_FORMAT = "@IBBHqIIII"

SensorFrameStreamHeader = namedtuple('SensorFrameStreamHeader',
                                     'Cookie VersionMajor VersionMinor FrameType Timestamp ImageWidth ImageHeight PixelStride RowStride')

# Constants
# Each port corresponds to a single stream type
# Port for obtaining Photo Video Camera stream
PV_STREAM_PORT = 23940

def main(argv):
    """ Receiever main"""
    parser = argparse.ArgumentParser()
    required_named_group = parser.add_argument_group('named arguments')

    required_named_group.add_argument("-a", "--host",
                                      help="Host address to connect", required=True)
    args = parser.parse_args(argv)
    
    # Create a TCP Stream socket
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    except socket.error, msg:
        print "ERROR: Failed to create socket. Code: " + str(msg[0]) + ', Message: ' + msg[1]
        sys.exit()
    print 'INFO: socket created'

    # Try connecting to the address
    s.connect((args.host, PV_STREAM_PORT))

    print 'INFO: Socket Connected to ' + args.host + ' on port ' + str(PV_STREAM_PORT)

    # Try receive data
    try:
        quit = False
        while not quit:
            reply = s.recv(struct.calcsize(SENSOR_STREAM_HEADER_FORMAT))
            if not reply:
                print 'ERROR: Failed to receive data'
                sys.exit()

            data = struct.unpack(SENSOR_STREAM_HEADER_FORMAT, reply)

            # Parse the header
            header = SensorFrameStreamHeader(*data)

            # read the image in chunks
            image_size_bytes = header.ImageHeight * header.RowStride
            image_data = ''

            while len(image_data) < image_size_bytes:
                remaining_bytes = image_size_bytes - len(image_data)
                image_data_chunk = s.recv(remaining_bytes)
                if not image_data_chunk:
                    print 'ERROR: Failed to receive image data'
                    sys.exit()
                image_data += image_data_chunk

            image_array = np.frombuffer(image_data, dtype=np.uint8).reshape((header.ImageHeight,
                                        header.ImageWidth, header.PixelStride))
            if PROCESS:
                # process image
                gray = cv2.cvtColor(image_array,cv2.COLOR_BGR2GRAY)
                image_array = cv2.Canny(gray,50,150,apertureSize = 3)

            cv2.imshow('Photo Video Camera Stream', image_array)
            
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
    except KeyboardInterrupt:
        pass

    s.close()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main(sys.argv[1:])
