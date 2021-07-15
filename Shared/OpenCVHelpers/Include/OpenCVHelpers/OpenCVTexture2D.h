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

namespace OpenCVHelpers
{
    void CreateOrUpdateTexture2D(
        _In_ const Graphics::DeviceResourcesPtr& deviceResources,
        _In_ const cv::Mat& image,
        _Inout_opt_ Rendering::Texture2DPtr& texture);

    void ReadBackTexture2D(
        _In_ const Rendering::Texture2DPtr& texture,
        _Out_ cv::Mat& image);
}
