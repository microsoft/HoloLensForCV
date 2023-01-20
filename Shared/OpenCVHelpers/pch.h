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

#include "targetver.h"

#include <map>
#include <array>
#include <memory>
#include <mutex>
#include <cstddef>
#include <stdexcept>
#include <shared_mutex>
#include <unordered_set>

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif /* !defined(WIN32_LEAN_AND_MEAN) */

#if !defined(NOMINMAX)
#define NOMINMAX
#endif /* !defined(NOMINMAX) */

#include <Windows.h>

#include <d2d1_2.h>
#include <d3d11_4.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <dwrite_2.h>
#include <wincodec.h>

#include <wrl.h>
#include <agile.h>
#include <collection.h>
#include <ppltasks.h>
#include <memorybuffer.h>

#include <windows.graphics.directx.direct3d11.interop.h>

#include <opencv2/imgproc/imgproc.hpp>

#include <Debugging/All.h>
#include <Io/All.h>
#include <Graphics/All.h>
#include <Rendering/All.h>
#include <OpenCVHelpers/OpenCVHelpers.h>
