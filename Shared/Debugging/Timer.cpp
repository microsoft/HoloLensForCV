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

namespace dbg
{
    Timer::Timer()
    {
        LARGE_INTEGER ticks_per_second;

        QueryPerformanceFrequency(&ticks_per_second);

        _ticksPerMilisecond = static_cast<double>(ticks_per_second.QuadPart) / 1000.0;

        Reset();
    }

    void Timer::Reset()
    {
        MarkEvent();

        _startTime = _lastEventTime;
    }

    void Timer::MarkEvent()
    {
        QueryPerformanceCounter(&_lastEventTime);
    }

    double Timer::GetMillisecondsFromStart() const
    {
        LARGE_INTEGER current_time;

        QueryPerformanceCounter(&current_time);

        return static_cast<double>(current_time.QuadPart - _startTime.QuadPart) / _ticksPerMilisecond;
    }

    double Timer::GetMillisecondsFromLastEvent() const
    {
        LARGE_INTEGER current_time;

        QueryPerformanceCounter(&current_time);

        return static_cast<double>(current_time.QuadPart - _lastEventTime.QuadPart) / _ticksPerMilisecond;
    }
}
