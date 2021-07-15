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
    public ref class MultiFrameBuffer sealed
        : public ISensorFrameSink
        , public ISensorFrameSinkGroup
    {
    public:
        virtual void Send(
            SensorFrame^ sensorFrame);

        virtual ISensorFrameSink^ GetSensorFrameSink(
            _In_ SensorType sensorType);

        SensorFrame^ GetLatestFrame(
            SensorType sensor);

        SensorFrame^ GetFrameForTime(
            SensorType sensor,
            Windows::Foundation::DateTime Timestamp,
            float toleranceInSeconds);

        Windows::Foundation::DateTime GetTimestampForSensorPair(
            SensorType a,
            SensorType b,
            float toleranceInSeconds);

    private:
        std::map<SensorType, std::deque<SensorFrame^>> _frames;
        std::mutex _framesMutex;
    };
}
