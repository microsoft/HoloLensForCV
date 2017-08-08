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

#include "MainPage.g.h"

namespace BatchProcessing
{
    public ref class MainPage sealed
    {
    public:
        MainPage();

    private:
        void Button_Select(
            Platform::Object^ sender,
            Windows::UI::Xaml::RoutedEventArgs^ e);

        void Button_Previous(
            Platform::Object^ sender,
            Windows::UI::Xaml::RoutedEventArgs^ e);

        void Button_Next(
            Platform::Object^ sender,
            Windows::UI::Xaml::RoutedEventArgs^ e);

        void Grid_KeyDown(
            Platform::Object^ sender,
            Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);

        void MoveRecordingCursor(
            int32_t howMuch);

        void UpdatePreview();

    private:
        std::vector<HoloLensCameraCalibration> _cameraCalibrations;
        std::vector<HoloLensCameraFrame> _pvCameraFrames;
        int32_t _currentPvCameraFrame = -1;
    };
}
