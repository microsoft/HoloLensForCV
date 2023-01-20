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

namespace HoloLensForCV
{
    class CsvWriter
    {
    public:
        CsvWriter(
            _In_ const std::wstring& outputFileName);

        ~CsvWriter();

        void WriteHeader(
            _In_ const std::vector<std::wstring>& columns);

        void WriteText(
            _In_ const std::wstring& text,
            _Inout_ bool* writeComma);

        void WriteInt32(
            _In_ const int32_t value,
            _Inout_ bool* writeComma);

        void WriteUInt64(
            _In_ const uint64_t value,
            _Inout_ bool* writeComma);

        void WriteFloat(
            _In_ const float value,
            _Inout_ bool* writeComma);

        void WriteDouble(
            _In_ const double value,
            _Inout_ bool* writeComma);

        void WriteFloat4x4(
            _In_ const Windows::Foundation::Numerics::float4x4& value,
            _Inout_ bool* writeComma);

        void WriteZeroFloat4x4(
            _Inout_ bool* writeComma);

        void WriteQuaternionWXYZ(
            _In_ const Windows::Foundation::Numerics::quaternion& value,
            _Inout_ bool* writeComma);

        void WriteFloat3XYZ(
            _In_ const Windows::Foundation::Numerics::float3& value,
            _Inout_ bool* writeComma);

        void EndLine();

    protected:
        void WriteComma(
            _Inout_ bool* shouldWrite);

    protected:
        std::wofstream _file;
    };
}
