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

namespace OpenCVHelpers
{
    void CreateOrUpdateTexture2D(
        _In_ const Graphics::DeviceResourcesPtr& deviceResources,
        _In_ const cv::Mat& image,
        _Inout_opt_ Rendering::Texture2DPtr& texture)
    {
        if (nullptr == texture)
        {
            texture =
                std::make_shared<Rendering::Texture2D>(
                    deviceResources,
                    image.cols,
                    image.rows);
        }

        {
            void* mappedTexture =
                texture->MapCPUTexture<void>(
                    D3D11_MAP_WRITE /* mapType */);

            cv::Mat mappedTextureAsImage(
                texture->GetHeight(),
                texture->GetWidth(),
                CV_8UC4,
                mappedTexture);

            image.copyTo(
                mappedTextureAsImage);

            texture->UnmapCPUTexture();
        }

        texture->CopyCPU2GPU();

        ENSURES(nullptr != texture);
    }

    void ReadBackTexture2D(
        _In_ const Rendering::Texture2DPtr& texture,
        _Out_ cv::Mat& image)
    {
        texture->CopyGPU2CPU();

        {
            void* mappedTexture =
                texture->MapCPUTexture<void>(
                    D3D11_MAP_READ /* mapType */);

            cv::Mat mappedTextureAsImage(
                texture->GetHeight(),
                texture->GetWidth(),
                CV_8UC4,
                mappedTexture);

            mappedTextureAsImage.copyTo(
                image);

            texture->UnmapCPUTexture();
        }
    }
}
