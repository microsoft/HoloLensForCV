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
    // Simple performance investigation primitive that calculates the amount of time
    // the object was alive. If that amount is greater than the specified threshold,
    // emits a debug trace formatted with the specified caption.
    //
    class TimerGuard
    {
    public:
        TimerGuard(
            _In_ const std::wstring& caption,
            _In_ const double maximumTimeElapsedInMillisecondsAllowed = 0.0);

        ~TimerGuard();

        Timer& GetTimer();

    private:
        const std::wstring _caption;
        const double _maximumTimeElapsedInMillisecondsAllowed;

        Timer _timer;
    };
}
