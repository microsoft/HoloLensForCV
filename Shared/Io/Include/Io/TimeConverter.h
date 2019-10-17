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
    //
    // Handles conversion between time values reported by the QueryPerformanceCounter API,
    // relative ticks (counting hundreds of nanoseconds elapsed since device boot) and
    // the absolute ticks (counting hundreds of nanoseconds since midnight of January 1st, 1601,
    // see the definition of FILETIME for more details on this time encoding).
    //
    class TimeConverter
    {
    public:
        TimeConverter();

        HundredsOfNanoseconds QpcToRelativeTicks(
            _In_ const int64_t qpc) const;

        HundredsOfNanoseconds QpcToRelativeTicks(
            _In_ const LARGE_INTEGER qpc) const;

        HundredsOfNanoseconds FileTimeToAbsoluteTicks(
            _In_ const FILETIME ft) const;

        HundredsOfNanoseconds RelativeTicksToAbsoluteTicks(
            _In_ const HundredsOfNanoseconds ticks) const;

        HundredsOfNanoseconds CalculateRelativeToAbsoluteTicksOffset() const;

    private:
        void Initialize();

        HundredsOfNanoseconds UnsignedQpcToRelativeTicks(
            _In_ const uint64_t qpc) const;

    private:
        LARGE_INTEGER _qpf;
        HundredsOfNanoseconds _qpc2ft;
    };
}
