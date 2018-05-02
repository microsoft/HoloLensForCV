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

namespace Io
{
    class Timer
    {
    public:
        Timer();

        // Get elapsed time since the previous ResetElapsedTime call.
        HundredsOfNanoseconds GetElapsedTime() const;

        double GetElapsedSeconds() const;

        // Get total time since the start of the program.
        HundredsOfNanoseconds GetTotalTime() const;

        double GetTotalSeconds() const;

        // Reset the elapsed time counter
        void ResetElapsedTime();

    private:
        TimeConverter _timeConverter;

        int64_t _qpcTotalStartTime;
        int64_t _qpcElapsedStartTime;
    };

    typedef std::shared_ptr<Timer> TimerPtr;
}
