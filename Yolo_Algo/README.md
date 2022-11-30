# YOLO Object Detection for Hololens Video Stream

Receives a video stream from Microsoft Hololens AR Headset on a local server and then processes the video. YOLO algorithm is applied to processed video.

Create a file with name bin and download the weights from https://drive.google.com/drive/folders/0B1tW_VtY7onidEwyQ2FtQVplWEU. Change the model name accordingly in the sensor_receiver.py file and then run the python file.

A stream of video signal is collected, it is processed for faster fps generation and then YOLO model is applied. Check and verify the active server-ports before running the python file.
