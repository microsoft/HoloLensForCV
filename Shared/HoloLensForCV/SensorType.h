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

#define ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS 1

namespace HoloLensForCV
{
    public enum class SensorType : int32_t
    {
        Undefined = -1,
        PhotoVideo = 0,

#if ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS
        ShortThrowToFDepth,
        ShortThrowToFReflectivity,
        LongThrowToFDepth,
        LongThrowToFReflectivity,
        VisibleLightLeftLeft,
        VisibleLightLeftFront,
        VisibleLightRightFront,
        VisibleLightRightRight,
#endif /* ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS */

        NumberOfSensorTypes
    };

    struct SensorTypeHash
    {
        size_t operator()(const SensorType& sensorType) const
        {
            return std::hash<int32_t>()((int32_t)sensorType);
        }
    };
}
