//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "pch.h"

namespace BatchProcessing
{
    //
    // Describes a single camera frame retrieved from a recording.
    //
    struct HoloLensCameraFrame
    {
        uint64_t Timestamp;
        Windows::Storage::StorageFolder^ RecordingFolder;
        std::wstring FileName;
        cv::Mat FrameToOrigin;
        cv::Mat CameraViewTransform;
        cv::Mat CameraProjectionTransform;
        int32_t Width;
        int32_t Height;
        int32_t PixelFormat;
        cv::Mat Image;

        void Load();
        void Unload();
    };

    //
    // Opens a camera frame manifest file from the specified folder
    // and returns a list of camera frames. Does not load the camera
    // images -- call HoloLensCameraFrame::Load for that to happen.
    //
    std::vector<HoloLensCameraFrame> DiscoverCameraFrames(
        _In_ Windows::Storage::StorageFolder^ recordingFolder,
        _In_ const std::wstring& manifestFileName);
}
