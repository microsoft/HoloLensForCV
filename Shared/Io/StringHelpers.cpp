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
    void TokenizeString(
        _In_ const std::string& string,
        _In_ const std::string& delimiter,
        _Inout_ std::vector<std::string>& tokens,
        _Inout_ std::vector<char>& tokenizerBuffer)
    {
        tokens.clear();

        tokenizerBuffer.resize(
            string.length() + 1);

        std::copy(
            string.begin(),
            string.end(),
            tokenizerBuffer.begin());

        tokenizerBuffer[string.length()] = '\0';

        char* nextToken =
            nullptr;

        char* cursor =
            strtok_s(
            tokenizerBuffer.data(),
            delimiter.c_str(),
            &nextToken);

        while (nullptr != cursor)
        {
            tokens.emplace_back(
                cursor);

            cursor = strtok_s(
                nullptr,
                delimiter.c_str(),
                &nextToken);
        }
    }
}

std::wstring Utf8ToUtf16(
    _In_z_ const char* text)
{
    wchar_t buffer[1024];

    swprintf_s(
        buffer,
        L"%S",
        text);

    return std::wstring(
        buffer);
}

std::wstring Utf8ToUtf16(
    _In_ const std::string& text)
{
    return Utf8ToUtf16(
        text.c_str());
}

std::string Utf16ToUtf8(
    _In_z_ const wchar_t* text)
{
    char buffer[1024];

    sprintf_s(
        buffer,
        "%S",
        text);

    return std::string(
        buffer);
}

std::string Utf16ToUtf8(
    _In_ const std::wstring& text)
{
    return Utf16ToUtf8(
        text.c_str());
}
