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
    HundredsOfNanoseconds UniversalToUnixTime(
        _In_ const FILETIME fileTime)
    {
        const std::chrono::seconds c_unix_epoch(
            11'644'473'600);

        return HundredsOfNanoseconds(
            fileTime.dwLowDateTime + (static_cast<uint64_t>(fileTime.dwHighDateTime) << 32)) - c_unix_epoch;
    }
}
