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

    HundredsOfNanoseconds TimeConverter::UnsignedQpcToRelativeTicks(
        _In_ const uint64_t qpc) const
    {
        static const std::uint64_t c_ticksPerSecond = 10'000'000;

        const std::uint64_t q = qpc / _qpf.QuadPart;
        const std::uint64_t r = qpc % _qpf.QuadPart;

        return HundredsOfNanoseconds(
            q * c_ticksPerSecond + (r * c_ticksPerSecond) / _qpf.QuadPart);
    }

    HundredsOfNanoseconds TimeConverter::QpcToRelativeTicks(
        _In_ const int64_t qpc) const
    {
        if (qpc < 0)
        {
            return -UnsignedQpcToRelativeTicks(
                static_cast<uint64_t>(-qpc));
        }
        else
        {
            return UnsignedQpcToRelativeTicks(
                static_cast<uint64_t>(qpc));
        }
    }

    HundredsOfNanoseconds TimeConverter::QpcToRelativeTicks(
        _In_ const LARGE_INTEGER qpc) const
    {
        return QpcToRelativeTicks(
            qpc.QuadPart);
    }

    HundredsOfNanoseconds TimeConverter::FileTimeToAbsoluteTicks(
        _In_ const FILETIME ft) const
    {
        ULARGE_INTEGER ft_uli;

        ft_uli.HighPart = ft.dwHighDateTime;
        ft_uli.LowPart = ft.dwLowDateTime;

        return HundredsOfNanoseconds(
            ft_uli.QuadPart);
    }

    HundredsOfNanoseconds TimeConverter::RelativeTicksToAbsoluteTicks(
        _In_ const HundredsOfNanoseconds ticks) const
    {
        return _qpc2ft + ticks;
    }

    HundredsOfNanoseconds TimeConverter::CalculateRelativeToAbsoluteTicksOffset() const
    {
        LARGE_INTEGER qpc_now;
        FILETIME ft_now;

        ASSERT(QueryPerformanceCounter(
            &qpc_now));

        GetSystemTimePreciseAsFileTime(
            &ft_now);

        const HundredsOfNanoseconds qpc_now_in_ticks =
            QpcToRelativeTicks(
                qpc_now);

        const HundredsOfNanoseconds ft_now_in_ticks =
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
    }
}
