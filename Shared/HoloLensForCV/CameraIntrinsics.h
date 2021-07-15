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

namespace HoloLensForCV
{
    /// <summary>
    /// Wraps the SensorStreaming::ICameraIntrinsics interface to exposes more detailed information
    /// about camera intrinsics.
    /// </summary>
    public ref class CameraIntrinsics sealed
    {
    internal:
        CameraIntrinsics(
            _In_ Microsoft::WRL::ComPtr<SensorStreaming::ICameraIntrinsics> sensorStreamingCameraIntrinsics,
            _In_ unsigned int imageWidth,
            _In_ unsigned int imageHeight);

    public:
        /// <summary>
        /// Maps an image pixel to the unit Z=1 plane.
        ///
        /// Convention applied is that integer coordinate of the pixel corresponds to
        /// the location of its top-left corner.
        /// </summary>
        bool MapImagePointToCameraUnitPlane(
            _In_ Windows::Foundation::Point UV,
            _Out_ Windows::Foundation::Point* XY);

        /// <summary>
        /// Projects a point from the unit Z=1 plane to the image plane. Returns the pixel
        /// coordinates of the projection.
        ///
        /// Convention applied is that integer coordinate of the pixel corresponds to
        /// the location of its top-left corner.
        /// </summary>
        bool MapCameraSpaceToImagePoint(
            _In_ Windows::Foundation::Point XY,
            _Out_ Windows::Foundation::Point* UV);

        property unsigned int ImageWidth;

        property unsigned int ImageHeight;

    private:
        Microsoft::WRL::ComPtr<SensorStreaming::ICameraIntrinsics> _sensorStreamingCameraIntrinsics;
    };
}
