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
    typedef std::chrono::duration<int64_t, std::ratio<1, 10'000'000>> HundredsOfNanoseconds;

    //
    // Conversion from universal time (counting the number of hundreds of nanoseconds relative
    // to 00:00:00 UTC January 1, 1601) to Unix time (counting the number of seconds since the
    // start of the epoch, 00:00:00 UTC January 1, 1970).
    //
    HundredsOfNanoseconds UniversalToUnixTime(
        _In_ const FILETIME fileTime);
}
