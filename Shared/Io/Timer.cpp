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

namespace Io
{
    namespace Internal
    {
        // Gets the current number of ticks from QueryPerformanceCounter. Throws an
        // exception if the call to QueryPerformanceCounter fails.
        int64_t GetPerformanceCounter()
        {
            LARGE_INTEGER counter;

            ASSERT(QueryPerformanceCounter(
                &counter));

            return counter.QuadPart;
        }
    }

    Timer::Timer()
    {
        _qpcTotalStartTime =
            Internal::GetPerformanceCounter();

        _qpcElapsedStartTime =
            _qpcTotalStartTime;
    }

    HundredsOfNanoseconds Timer::GetElapsedTime() const
    {
        const int64_t qpcCurrentTime =
            Internal::GetPerformanceCounter();

        const int64_t qpcElapsedTime =
            qpcCurrentTime - _qpcElapsedStartTime;

        return _timeConverter.QpcToRelativeTicks(
            qpcCurrentTime - _qpcElapsedStartTime);
    }

    double Timer::GetElapsedSeconds() const
    {
        return std::chrono::duration_cast<std::chrono::duration<double>>(
            GetElapsedTime()).count();
    }

    HundredsOfNanoseconds Timer::GetTotalTime() const
    {
        const int64_t qpcCurrentTime =
            Internal::GetPerformanceCounter();

        return _timeConverter.QpcToRelativeTicks(
            qpcCurrentTime);
    }

    double Timer::GetTotalSeconds() const
    {
        return std::chrono::duration_cast<std::chrono::duration<double>>(
            GetTotalTime()).count();
    }

    void Timer::ResetElapsedTime()
    {
        _qpcElapsedStartTime =
            Internal::GetPerformanceCounter();
    }
}
