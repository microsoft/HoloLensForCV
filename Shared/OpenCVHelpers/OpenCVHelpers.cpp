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

namespace rmcv
{
    void WrapHoloLensSensorFrameWithCvMat(
        _In_ HoloLensForCV::SensorFrame^ holoLensSensorFrame,
        _Out_ cv::Mat& wrappedImage)
    {
        Windows::Graphics::Imaging::SoftwareBitmap^ bitmap =
            holoLensSensorFrame->SoftwareBitmap;

        Windows::Graphics::Imaging::BitmapBuffer^ bitmapBuffer =
            bitmap->LockBuffer(
                Windows::Graphics::Imaging::BitmapBufferAccessMode::Read);

        uint32_t pixelBufferDataLength = 0;

        uint8_t* pixelBufferData =
            Io::GetPointerToMemoryBuffer(
                bitmapBuffer->CreateReference(),
                pixelBufferDataLength);

        int32_t wrappedImageType;

        switch (bitmap->BitmapPixelFormat)
        {
        case Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8:
            wrappedImageType = CV_8UC4;
            break;

        case Windows::Graphics::Imaging::BitmapPixelFormat::Gray16:
            wrappedImageType = CV_16UC1;
            break;

        case Windows::Graphics::Imaging::BitmapPixelFormat::Gray8:
            wrappedImageType = CV_8UC1;
            break;

        default:
            dbg::trace(
                L"WrapHoloLensSensorFrameWithCvMat: unrecognized bitmap pixel format, falling back to CV_8UC1");

            wrappedImageType = CV_8UC1;
            break;
        }

        wrappedImage = cv::Mat(
            bitmap->PixelHeight,
            bitmap->PixelWidth,
            wrappedImageType,
            pixelBufferData);
    }
}
