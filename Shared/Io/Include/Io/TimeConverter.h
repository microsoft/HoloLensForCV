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

        uint64_t QpcToRelativeTicks(
            _In_ const uint64_t qpc);

        uint64_t QpcToRelativeTicks(
            _In_ const LARGE_INTEGER qpc);

        uint64_t FileTimeToAbsoluteTicks(
            _In_ const FILETIME ft);

        uint64_t RelativeTicksToAbsoluteTicks(
            _In_ const uint64_t ticks);

        uint64_t CalculateRelativeToAbsoluteTicksOffset();

    private:
        void Initialize();

    private:
        LARGE_INTEGER _qpf;
        uint64_t _qpc2ft;
    };
}
