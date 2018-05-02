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

namespace HoloLensForCV
{
    CameraIntrinsics::CameraIntrinsics(
        _In_ Microsoft::WRL::ComPtr<SensorStreaming::ICameraIntrinsics> sensorStreamingCameraIntrinsics,
        _In_ unsigned int imageWidth,
        _In_ unsigned int imageHeight)
        : _sensorStreamingCameraIntrinsics(sensorStreamingCameraIntrinsics)
    {
        ImageWidth = imageWidth;
        ImageHeight = imageHeight;
    }

    bool CameraIntrinsics::MapImagePointToCameraUnitPlane(
        _In_ Windows::Foundation::Point UV,
        _Out_ Windows::Foundation::Point* XY)
    {
        float uv[2] = { UV.X, UV.Y };
        float xy[2];

        if (FAILED(_sensorStreamingCameraIntrinsics->MapImagePointToCameraUnitPlane(uv, xy)))
        {
            XY->X = XY->Y = std::numeric_limits<float>::infinity();

            return false;
        }

        XY->X = xy[0];
        XY->Y = xy[1];

        return true;
    }

    bool CameraIntrinsics::MapCameraSpaceToImagePoint(
        _In_ Windows::Foundation::Point XY,
        _Out_ Windows::Foundation::Point* UV)
    {
        float xy[2] = { XY.X, XY.Y };
        float uv[2];

        if (FAILED(_sensorStreamingCameraIntrinsics->MapCameraSpaceToImagePoint(xy, uv)))
        {
            UV->X = UV->Y = std::numeric_limits<float>::infinity();

            return false;
        }

        UV->X = uv[0];
        UV->Y = uv[1];

        return true;
    }
}
