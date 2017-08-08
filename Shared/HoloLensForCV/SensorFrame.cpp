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
    SensorFrame::SensorFrame(
        _In_ SensorType frameType,
        _In_ Windows::Foundation::DateTime timestamp,
        _In_ Windows::Graphics::Imaging::SoftwareBitmap^ softwareBitmap)
    {
        FrameType = frameType;
        Timestamp = timestamp;
        SoftwareBitmap = softwareBitmap;
    }
}
