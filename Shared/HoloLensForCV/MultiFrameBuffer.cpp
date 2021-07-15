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
    static double TimeDeltaAMinusB(
        Windows::Foundation::DateTime a,
        Windows::Foundation::DateTime b)
    {
        auto timeDiff100ns = a.UniversalTime - b.UniversalTime;

        return timeDiff100ns * 1e-7;
    }

    ISensorFrameSink^ MultiFrameBuffer::GetSensorFrameSink(
        _In_ SensorType /* sensorType */)
    {
        return this;
    }

    void MultiFrameBuffer::Send(
        SensorFrame^ sensorFrame)
    {
        std::lock_guard<std::mutex> lock(_framesMutex);
        
        auto& buffer = _frames[sensorFrame->FrameType];
        
        buffer.push_back(sensorFrame);
        
        while (buffer.size() > 5)
        {
            buffer.pop_front();
        }
    }

    SensorFrame^ MultiFrameBuffer::GetLatestFrame(
        SensorType sensor)
    {
        std::lock_guard<std::mutex> lock(_framesMutex);

        auto& buffer = _frames[sensor];

        if (buffer.empty())
        {
            return nullptr;
        }

        return buffer.back();
    }

    SensorFrame^ MultiFrameBuffer::GetFrameForTime(
        SensorType sensor,
        Windows::Foundation::DateTime Timestamp,
        float toleranceInSeconds)
    {
        std::lock_guard<std::mutex> lock(_framesMutex);

        auto& buffer = _frames[sensor];

        for (auto& f : buffer)
        {
            const double secondsDifference =
                std::abs(
                    TimeDeltaAMinusB(
                        Timestamp,
                        f->Timestamp));

            if (secondsDifference < toleranceInSeconds)
            {
                return f;
            }
        }

        return nullptr;
    }

    Windows::Foundation::DateTime MultiFrameBuffer::GetTimestampForSensorPair(
        SensorType a, 
        SensorType b,
        float toleranceInSeconds)
    {
        std::vector<Windows::Foundation::DateTime> vta;
        std::vector<Windows::Foundation::DateTime> vtb;

        {
            std::lock_guard<std::mutex> lock(_framesMutex);

            for (auto& f : _frames[a])
            {
                vta.push_back(f->Timestamp);
            }

            for (auto& f : _frames[b])
            {
                vtb.push_back(f->Timestamp);
            }
        }

        Windows::Foundation::DateTime best;
        best.UniversalTime = 0;

        for (auto ta : vta)
        {
            for (auto tb : vtb)
            {
                if (std::abs(TimeDeltaAMinusB(ta, tb)) < toleranceInSeconds)
                {
                    if (TimeDeltaAMinusB(ta, best) > 0)
                    {
                        best = ta;
                    }
                }
            }
        }

        return best;
    }
}
