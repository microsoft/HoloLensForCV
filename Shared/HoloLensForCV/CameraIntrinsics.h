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
    //
    // Ref Class Wrapper for Native ICameraIntrinsics object
    // 
    public ref class CameraIntrinsics sealed
    {
    internal:
        CameraIntrinsics(
            _In_ Microsoft::WRL::ComPtr<SensorStreaming::ICameraIntrinsics> intrinsics,
            _In_ unsigned int imageWidth,
            _In_ unsigned int imageHeight);

    public:
        Windows::Foundation::Point MapImagePointToCameraUnitPlane(_In_ Windows::Foundation::Point pixel);

        property unsigned int ImageWidth;

        property unsigned int ImageHeight;

    private:
        Microsoft::WRL::ComPtr<SensorStreaming::ICameraIntrinsics> _intrinsics;
    };
}
