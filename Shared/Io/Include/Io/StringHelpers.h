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
    void TokenizeString(
        _In_ const std::string& string,
        _In_ const std::string& delimiter,
        _Inout_ std::vector<std::string>& tokens,
        _Inout_ std::vector<char>& tokenizerBuffer);
}

std::wstring Utf8ToUtf16(
    _In_z_ const char* text);

std::wstring Utf8ToUtf16(
    _In_ const std::string& text);

std::string Utf16ToUtf8(
    _In_z_ const wchar_t* text);

std::string Utf16ToUtf8(
    _In_ const std::wstring& text);
