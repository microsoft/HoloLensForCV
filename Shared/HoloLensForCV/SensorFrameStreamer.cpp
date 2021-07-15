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
    SensorFrameStreamer::SensorFrameStreamer()
    {
    }

    void SensorFrameStreamer::EnableAll()
    {
        Enable(SensorType::PhotoVideo);

#if ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS
        Enable(SensorType::ShortThrowToFDepth);
        Enable(SensorType::ShortThrowToFReflectivity);
        Enable(SensorType::LongThrowToFDepth);
        Enable(SensorType::LongThrowToFReflectivity);
        Enable(SensorType::VisibleLightLeftLeft);
        Enable(SensorType::VisibleLightLeftFront);
        Enable(SensorType::VisibleLightRightFront);
        Enable(SensorType::VisibleLightRightRight);
#endif /* ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS */
    }

    void SensorFrameStreamer::Enable(
        _In_ SensorType sensorType)
    {
        switch (sensorType)
        {
        case SensorType::PhotoVideo:
            _sensorFrameStreamingServers[(int32_t)SensorType::PhotoVideo] =
                ref new SensorFrameStreamingServer(L"23940");
            break;

#if ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS
        case SensorType::ShortThrowToFDepth:
            _sensorFrameStreamingServers[(int32_t)SensorType::ShortThrowToFDepth] =
                ref new SensorFrameStreamingServer(L"23941");
            break;

        case SensorType::ShortThrowToFReflectivity:
            _sensorFrameStreamingServers[(int32_t)SensorType::ShortThrowToFReflectivity] =
                ref new SensorFrameStreamingServer(L"23942");
            break;

        case SensorType::LongThrowToFDepth:
            _sensorFrameStreamingServers[(int32_t)SensorType::LongThrowToFDepth] =
                ref new SensorFrameStreamingServer(L"23947");
            break;

        case SensorType::LongThrowToFReflectivity:
            _sensorFrameStreamingServers[(int32_t)SensorType::LongThrowToFReflectivity] =
                ref new SensorFrameStreamingServer(L"23948");
            break;

        case SensorType::VisibleLightLeftLeft:
            _sensorFrameStreamingServers[(int32_t)SensorType::VisibleLightLeftLeft] =
                ref new SensorFrameStreamingServer(L"23943");
            break;

        case SensorType::VisibleLightLeftFront:
            _sensorFrameStreamingServers[(int32_t)SensorType::VisibleLightLeftFront] =
                ref new SensorFrameStreamingServer(L"23944");
            break;

        case SensorType::VisibleLightRightFront:
            _sensorFrameStreamingServers[(int32_t)SensorType::VisibleLightRightFront] =
                ref new SensorFrameStreamingServer(L"23945");
            break;

        case SensorType::VisibleLightRightRight:
            _sensorFrameStreamingServers[(int32_t)SensorType::VisibleLightRightRight] =
                ref new SensorFrameStreamingServer(L"23946");
            break;
#endif /* ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS */
        }
    }

    ISensorFrameSink^ SensorFrameStreamer::GetSensorFrameSink(
        _In_ SensorType sensorType)
    {
        const int32_t sensorTypeAsIndex =
            (int32_t)sensorType;

        REQUIRES(
            0 <= sensorTypeAsIndex &&
            sensorTypeAsIndex < (int32_t)_sensorFrameStreamingServers.size());

        return _sensorFrameStreamingServers[
            sensorTypeAsIndex];
    }
}
