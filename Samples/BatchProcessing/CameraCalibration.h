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

namespace BatchProcessing
{
    struct HoloLensCameraCalibration
    {
        std::wstring SensorName;

        float FocalLengthX;
        float FocalLengthY;

        float PrincipalPointX;
        float PrincipalPointY;

        float RadialDistortionX;
        float RadialDistortionY;
        float RadialDistortionZ;

        float TangentialDistortionX;
        float TangentialDistortionY;
    };

    std::vector<HoloLensCameraCalibration> ReadCameraCalibrations(
        Windows::Storage::StorageFolder^ recordingFolder);
}
