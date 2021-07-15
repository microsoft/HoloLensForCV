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

#pragma once

namespace rmcv
{
    void WrapHoloLensSensorFrameWithCvMat(
        _In_ HoloLensForCV::SensorFrame^ holoLensSensorFrame,
        _Out_ cv::Mat& openCVImage);

    /// <summary>
    /// Wraps a HoloLens Visible Light Camera frame with a cv::Mat. The VLC images
    /// are 8bpp grayscale, but we deliver them through Media APIs as 32bpp BGRA
    /// images, with each of the BGRA values representing 4 consecutive grayscale
    /// pixel intensities.
    /// </summary>
    void WrapHoloLensVisibleLightCameraFrameWithCvMat(
        _In_ HoloLensForCV::SensorFrame^ holoLensSensorFrame,
        _Out_ cv::Mat& wrappedImage);
}
