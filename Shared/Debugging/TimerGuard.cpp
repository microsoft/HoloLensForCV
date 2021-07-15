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
    TimerGuard::TimerGuard(
        _In_ const std::wstring& caption,
        _In_ const double maximumTimeElapsedInMillisecondsAllowed)
        : _caption(caption)
        , _maximumTimeElapsedInMillisecondsAllowed(maximumTimeElapsedInMillisecondsAllowed)
        , _timer()
    {
    }

    TimerGuard::~TimerGuard()
    {
        const double millisecondsElapsed =
            _timer.GetMillisecondsFromStart();

        if (_maximumTimeElapsedInMillisecondsAllowed > 0.0)
        {
            if (millisecondsElapsed >= _maximumTimeElapsedInMillisecondsAllowed)
            {
                dbg::trace(
                    L"[TimerGuard] %s timer elapsed %.02fms (only reporting if over %.02fms)",
                    _caption.c_str(),
                    millisecondsElapsed,
                    _maximumTimeElapsedInMillisecondsAllowed);
            }
        }
        else
        {
            dbg::trace(
                L"[TimerGuard] %s timer elapsed %.02fms",
                _caption.c_str(),
                millisecondsElapsed);
        }
    }

    Timer& TimerGuard::GetTimer()
    {
        return _timer;
    }
}
