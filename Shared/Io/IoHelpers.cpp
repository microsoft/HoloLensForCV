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
    Microsoft::WRL::ComPtr<IStorageFolderHandleAccess> GetStorageFolderHandleAccess(
        _In_ Windows::Storage::StorageFolder^ folder)
    {
        Microsoft::WRL::ComPtr<IUnknown> folderAbiPointer(
            reinterpret_cast<IUnknown*>(
                folder));

        Microsoft::WRL::ComPtr<IStorageFolderHandleAccess> folderHandleAccess;

        ASSERT_SUCCEEDED(folderAbiPointer.As(
            &folderHandleAccess));

        return folderHandleAccess;
    }

    concurrency::task<std::vector<byte>> ReadDataAsync(
        const std::wstring& filename)
    {
        return concurrency::create_task(
            Windows::Storage::PathIO::ReadBufferAsync(
                Platform::StringReference(
                    filename.c_str()))).
            then([](Windows::Storage::Streams::IBuffer^ fileBuffer) -> std::vector<byte>
        {
            std::vector<byte> returnBuffer;

            returnBuffer.resize(
                fileBuffer->Length);

            Windows::Storage::Streams::DataReader::FromBuffer(
                fileBuffer)->ReadBytes(
                    Platform::ArrayReference<byte>(
                        returnBuffer.data(),
                        static_cast<uint32_t>(returnBuffer.size())));

            return std::move(returnBuffer);
        });
    }

    std::vector<byte> ReadDataSync(
        _In_ Windows::Storage::StorageFolder^ folder,
        _In_ const std::wstring& fileName)
    {
        Microsoft::WRL::ComPtr<IStorageFolderHandleAccess> folderHandleAccess =
            GetStorageFolderHandleAccess(
                folder);

        HANDLE input = nullptr;

        ASSERT_SUCCEEDED(folderHandleAccess->Create(
            fileName.c_str() /* fileName */,
            HCO_OPEN_EXISTING /* creationOptions */,
            HAO_READ /* accessOptions */,
            HSO_SHARE_READ /* sharingOptions */,
            HO_NONE /* options */,
            nullptr /* oplockBreakingHandler */,
            &input));

        LARGE_INTEGER fileSize = {};

        ASSERT(!!GetFileSizeEx(
            input,
            &fileSize));

        std::vector<byte> fileBuffer(
            static_cast<size_t>(fileSize.QuadPart));

        DWORD numberOfBytesRead = 0;

        ASSERT(!!ReadFile(
            input,
            &fileBuffer[0],
            static_cast<DWORD>(fileBuffer.size()),
            &numberOfBytesRead,
            nullptr /* lpOverlapped */));

        ASSERT(fileBuffer.size() == numberOfBytesRead);

        return std::move(
            fileBuffer);
    }
}
