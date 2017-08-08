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

#include <map>
#include <array>
#include <memory>
#include <mutex>
#include <fstream>
#include <sstream>
#include <cstddef>
#include <stdexcept>
#include <shared_mutex>
#include <unordered_set>

#include <agile.h>
#include <collection.h>
#include <ppltasks.h>
#include <memorybuffer.h>
#include <windowsnumerics.h>
#include <windows.foundation.h>
#include <windows.foundation.collections.h>

#include <d2d1_2.h>
#include <d3d11_4.h>
#include <dwrite_2.h>
#include <wincodec.h>
#include <DirectXColors.h>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#define DBG_ENABLE_VERBOSE_LOGGING 1

#include <Debugging/All.h>
#include <Graphics/All.h>
#include <Rendering/All.h>
#include <OpenCVHelpers/All.h>

#include "App.xaml.h"
