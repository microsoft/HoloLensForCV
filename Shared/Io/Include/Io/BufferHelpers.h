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
    void* GetPointerToMemoryBuffer(
        _In_ Windows::Foundation::IMemoryBufferReference^ memoryBuffer,
        _Out_ uint32_t& memoryBufferLength);

    template <typename Ty>
    Ty* GetTypedPointerToMemoryBuffer(
        _In_ Windows::Foundation::IMemoryBufferReference^ memoryBuffer,
        _Out_ uint32_t& memoryBufferLength)
    {
        return reinterpret_cast<Ty*>(
            GetPointerToMemoryBuffer(
                memoryBuffer,
                memoryBufferLength));
    }

    void* GetPointerToIBuffer(
        _In_ Windows::Storage::Streams::IBuffer^ buffer);

    template <typename Ty>
    Ty* GetTypedPointerToIBuffer(
        Windows::Storage::Streams::IBuffer^ buffer)
    {
        return reinterpret_cast<Ty*>(
            GetPointerToIBuffer(
                buffer));
    }
}
