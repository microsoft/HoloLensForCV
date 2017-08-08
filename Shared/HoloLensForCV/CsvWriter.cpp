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

namespace HoloLensForCV
{
    _Use_decl_annotations_
    CsvWriter::CsvWriter(
        const std::wstring& outputFileName)
        : _file(outputFileName)
    {
        ASSERT(_file);
    }

    CsvWriter::~CsvWriter()
    {
        EndLine();
    }

    _Use_decl_annotations_
    void CsvWriter::WriteHeader(
        const std::vector<std::wstring>& columns)
    {
        bool writeComma = false;

        for (const auto& column : columns)
        {
            WriteComma(
                &writeComma);

            _file << column;
        }

        EndLine();
    }

    _Use_decl_annotations_
    void CsvWriter::WriteText(
        const std::wstring& text,
        bool* writeComma)
    {
        WriteComma(
            writeComma);

        _file << text;
    }

    _Use_decl_annotations_
    void CsvWriter::WriteInt32(
        const int32_t value,
        bool* writeComma)
    {
        WriteComma(
            writeComma);

        _file << value;
    }

    _Use_decl_annotations_
    void CsvWriter::WriteUInt64(
        _In_ const uint64_t value,
        _Inout_ bool* writeComma)
    {
        WriteComma(
            writeComma);

        _file << value;
    }

    _Use_decl_annotations_
    void CsvWriter::WriteFloat(
        const float value,
        bool* writeComma)
    {
        WriteComma(
            writeComma);

        _file << value;
    }

    _Use_decl_annotations_
    void CsvWriter::WriteDouble(
        const double value,
        bool* writeComma)
    {
        WriteComma(
            writeComma);

        _file << value;
    }

    _Use_decl_annotations_
    void CsvWriter::WriteFloat4x4(
        const Windows::Foundation::Numerics::float4x4& value,
        bool* writeComma)
    {
        WriteFloat(value.m11, writeComma);
        WriteFloat(value.m12, writeComma);
        WriteFloat(value.m13, writeComma);
        WriteFloat(value.m14, writeComma);

        WriteFloat(value.m21, writeComma);
        WriteFloat(value.m22, writeComma);
        WriteFloat(value.m23, writeComma);
        WriteFloat(value.m24, writeComma);

        WriteFloat(value.m31, writeComma);
        WriteFloat(value.m32, writeComma);
        WriteFloat(value.m33, writeComma);
        WriteFloat(value.m34, writeComma);

        WriteFloat(value.m41, writeComma);
        WriteFloat(value.m42, writeComma);
        WriteFloat(value.m43, writeComma);
        WriteFloat(value.m44, writeComma);
    }

    void CsvWriter::WriteZeroFloat4x4(
        _Inout_ bool* writeComma)
    {
        for (int32_t i = 0; i < 16; ++i)
        {
            WriteFloat(0.0f, writeComma);
        }
    }

    _Use_decl_annotations_
    void CsvWriter::WriteQuaternionWXYZ(
        const Windows::Foundation::Numerics::quaternion& value,
        bool* writeComma)
    {
        WriteFloat(value.w, writeComma);
        WriteFloat(value.x, writeComma);
        WriteFloat(value.y, writeComma);
        WriteFloat(value.z, writeComma);
    }

    _Use_decl_annotations_
    void CsvWriter::WriteFloat3XYZ(
        const Windows::Foundation::Numerics::float3& value,
        bool* writeComma)
    {
        WriteFloat(value.x, writeComma);
        WriteFloat(value.y, writeComma);
        WriteFloat(value.z, writeComma);
    }

    void CsvWriter::EndLine()
    {
        _file << std::endl;
    }

    _Use_decl_annotations_
    void CsvWriter::WriteComma(
        bool* writeComma)
    {
        if (*writeComma)
        {
            _file << ',';
        }
        else
        {
            *writeComma = true;
        }
    }
}
