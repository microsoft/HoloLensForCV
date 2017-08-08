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
    uint8_t* GetPointerToMemoryBuffer(
        _In_ Windows::Foundation::IMemoryBufferReference^ memoryBuffer,
        _Out_ uint32_t& memoryBufferLength)
    {
        //
        // Query the IBufferByteAccess interface.
        //
        Microsoft::WRL::ComPtr<Windows::Foundation::IMemoryBufferByteAccess> bufferByteAccess;

        ASSERT_SUCCEEDED(reinterpret_cast<IInspectable*>(memoryBuffer)->QueryInterface(
            IID_PPV_ARGS(
                &bufferByteAccess)));

        //
        // Retrieve the buffer data.
        //
        uint8_t* memoryBufferData = nullptr;

        ASSERT_SUCCEEDED(bufferByteAccess->GetBuffer(
            &memoryBufferData,
            &memoryBufferLength));

        return memoryBufferData;
    }
}
