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
    TimeConverter::TimeConverter()
        : _qpf()
        , _qpc2ft()
    {
        Initialize();
    }

    uint64_t TimeConverter::QpcToRelativeTicks(
        _In_ const uint64_t qpc)
    {
        static const std::uint64_t c_ticksPerSecond = 10'000'000;

        const std::uint64_t q = qpc / _qpf.QuadPart;
        const std::uint64_t r = qpc % _qpf.QuadPart;

        return q * c_ticksPerSecond + (r * c_ticksPerSecond) / _qpf.QuadPart;
    }

    uint64_t TimeConverter::QpcToRelativeTicks(
        _In_ const LARGE_INTEGER qpc)
    {
        REQUIRES(qpc.QuadPart >= 0);

        return QpcToRelativeTicks(
            static_cast<uint64_t>(
                qpc.QuadPart));
    }

    uint64_t TimeConverter::FileTimeToAbsoluteTicks(
        _In_ const FILETIME ft)
    {
        ULARGE_INTEGER ft_uli;

        ft_uli.HighPart = ft.dwHighDateTime;
        ft_uli.LowPart = ft.dwLowDateTime;

        return ft_uli.QuadPart;
    }

    uint64_t TimeConverter::RelativeTicksToAbsoluteTicks(
        _In_ const uint64_t ticks)
    {
        return _qpc2ft + ticks;
    }

    uint64_t TimeConverter::CalculateRelativeToAbsoluteTicksOffset()
    {
        LARGE_INTEGER qpc_now;
        FILETIME ft_now;

        ASSERT(QueryPerformanceCounter(
            &qpc_now));

        GetSystemTimePreciseAsFileTime(
            &ft_now);

        const uint64_t qpc_now_in_ticks =
            QpcToRelativeTicks(
                qpc_now);

        const uint64_t ft_now_in_ticks =
            FileTimeToAbsoluteTicks(
                ft_now);

        ASSERT(ft_now_in_ticks > qpc_now_in_ticks);

        return ft_now_in_ticks - qpc_now_in_ticks;
    }

    void TimeConverter::Initialize()
    {
        ASSERT(QueryPerformanceFrequency(
            &_qpf));

        _qpc2ft =
            CalculateRelativeToAbsoluteTicksOffset();

        dbg::trace(
            L"TimeConverter::Initialize: relative to absolute ticks offset is %llu",
            _qpc2ft);
    }
}
