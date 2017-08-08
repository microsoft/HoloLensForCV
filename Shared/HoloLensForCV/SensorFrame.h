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
    // Collects information about a sensor frame -- originated on device, or remotely.
    // 
    public ref class SensorFrame sealed
    {
    public:
        SensorFrame(
            _In_ SensorType frameType,
            _In_ Windows::Foundation::DateTime timestamp,
            _In_ Windows::Graphics::Imaging::SoftwareBitmap^ softwareBitmap);

        property SensorType FrameType;
        property Windows::Foundation::DateTime Timestamp;
        property Windows::Graphics::Imaging::SoftwareBitmap^ SoftwareBitmap;

        property Windows::Media::Devices::Core::CameraIntrinsics^ CameraIntrinsics;
        property Windows::Foundation::Numerics::float4x4 FrameToOrigin;
    };
}
