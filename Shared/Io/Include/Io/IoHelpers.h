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
    Microsoft::WRL::ComPtr<IStorageFolderHandleAccess> GetStorageFolderHandleAccess(
        _In_ Windows::Storage::StorageFolder^ folder);

    // Function that reads from a binary file asynchronously.
    concurrency::task<std::vector<byte>> ReadDataAsync(
        const std::wstring& filename);

    // Function that reads from a binary file from the specified folder synchronously.
    std::vector<byte> ReadDataSync(
        _In_ Windows::Storage::StorageFolder^ folder,
        _In_ const std::wstring& fileName);
}
