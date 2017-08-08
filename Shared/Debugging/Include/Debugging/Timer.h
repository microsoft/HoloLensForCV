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

namespace dbg
{
    //
    // QueryPerformanceCounter-based timer / stop-watch.
    //
    class Timer
    {
    public:
        Timer();

        void Reset();

        void MarkEvent();

        double GetMillisecondsFromStart() const;

        double GetMillisecondsFromLastEvent() const;

    private:
        double _ticksPerMilisecond;

        LARGE_INTEGER _startTime;
        LARGE_INTEGER _lastEventTime;
    };
}
