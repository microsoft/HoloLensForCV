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

#define ASSERT(expr) \
    do { \
        if (!(expr)) { \
            dbg::trace(L"ERROR: %S:%i: ASSERT(%S) check failed", __FILE__, __LINE__, #expr); \
            throw std::logic_error("assertion failure"); \
        } \
    } while (0, 0)

#define ASSERT_SUCCEEDED(expr) \
    do { \
        const HRESULT _expr_hr = (expr); \
        if (FAILED(_expr_hr)) { \
            dbg::trace(L"ERROR: %S:%i: ASSERT_SUCCEEDED(%S) check failed with HRESULT 0x%08x", __FILE__, __LINE__, #expr, _expr_hr); \
            throw std::logic_error("assertion failure"); \
        } \
    } while (0, 0)

#define REQUIRES(expr) \
    do { \
        if (!(expr)) { \
            dbg::trace(L"ERROR: %S:%i: REQUIRES(%S) check failed", __FILE__, __LINE__, #expr); \
            throw std::logic_error("assertion failure"); \
        } \
    } while (0, 0)

#define ENSURES(expr) \
    do { \
        if (!(expr)) { \
            dbg::trace(L"ERROR: %S:%i: ENSURES(%S) check failed", __FILE__, __LINE__, #expr); \
            throw std::logic_error("assertion failure"); \
        } \
    } while (0, 0)
