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
#include "MainPage.xaml.h"

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

namespace BatchProcessing
{
    MainPage::MainPage()
    {
        InitializeComponent();
    }

    void MainPage::Button_Select(
        Platform::Object^ sender,
        Windows::UI::Xaml::RoutedEventArgs^ e)
    {
        std::vector<std::wstring> files;

        auto folderPicker =
            ref new Windows::Storage::Pickers::FolderPicker();

        folderPicker->SuggestedStartLocation =
            Windows::Storage::Pickers::PickerLocationId::ComputerFolder;

        folderPicker->FileTypeFilter->Append(
            L"*");

        concurrency::create_task(
            folderPicker->PickSingleFolderAsync()).
            then([this](Windows::Storage::StorageFolder^ folder)
        {
            _cameraCalibrations =
                ReadCameraCalibrations(
                    folder);

            for (const auto& cameraCalibration : _cameraCalibrations)
            {
                dbg::trace(
                    L" *** found camera calibration for sensor '%s' [f=[%f, %f], p=[%f, %f], r=[%f, %f, %f], t=[%f, %f]",
                    cameraCalibration.SensorName.c_str(),
                    cameraCalibration.FocalLengthX,
                    cameraCalibration.FocalLengthY,
                    cameraCalibration.PrincipalPointX,
                    cameraCalibration.PrincipalPointY,
                    cameraCalibration.RadialDistortionX,
                    cameraCalibration.RadialDistortionY,
                    cameraCalibration.RadialDistortionZ,
                    cameraCalibration.TangentialDistortionX,
                    cameraCalibration.TangentialDistortionY);
            }

            _pvCameraFrames =
                DiscoverCameraFrames(
                    folder,
                    L"pv.csv");

            dbg::trace(
                L" *** found %i PV camera frames",
                _pvCameraFrames.size());

            for (const auto& cameraFrame : _pvCameraFrames)
            {
                dbg::trace(
                    L"     %llu: %s",
                    cameraFrame.Timestamp,
                    cameraFrame.FileName.c_str());
            }

            _currentPvCameraFrame = -1;

            MoveRecordingCursor(
                +1 /* howMuch */);
        });
    }

    void MainPage::Button_Previous(
        Platform::Object^ sender,
        Windows::UI::Xaml::RoutedEventArgs^ e)
    {
        MoveRecordingCursor(
            -1 /* howMuch */);
    }

    void MainPage::Button_Next(
        Platform::Object^ sender,
        Windows::UI::Xaml::RoutedEventArgs^ e)
    {
        MoveRecordingCursor(
            +1 /* howMuch */);
    }

    void MainPage::UpdatePreview()
    {
        if (_currentPvCameraFrame < 0)
        {
            return;
        }

        _pvCameraImage->Dispatcher->RunAsync(
            Windows::UI::Core::CoreDispatcherPriority::Normal,
            ref new Windows::UI::Core::DispatchedHandler(
                [this]()
        {
            const auto& pvCameraFrame =
                _pvCameraFrames[
                    _currentPvCameraFrame];

            {
                wchar_t caption[MAX_PATH] = {};

                swprintf_s(
                    caption,
                    L"PV Camera Image: frame %i / %i",
                    _currentPvCameraFrame + 1,
                    static_cast<uint32_t>(_pvCameraFrames.size()));

                _pvCameraImageCaption->Text =
                    ref new Platform::String(
                        caption);
            }

            auto image =
                ref new Windows::Graphics::Imaging::SoftwareBitmap(
                    Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8,
                    pvCameraFrame.Width,
                    pvCameraFrame.Height,
                    Windows::Graphics::Imaging::BitmapAlphaMode::Ignore);

            {
                auto imageBuffer =
                    image->LockBuffer(
                        Windows::Graphics::Imaging::BitmapBufferAccessMode::Write);

                auto imageBufferReference =
                    imageBuffer->CreateReference();

                uint32_t imageBufferDataLength = 0;

                uint8_t* imageBufferData =
                    Io::GetTypedPointerToMemoryBuffer<uint8_t>(
                        imageBufferReference,
                        imageBufferDataLength);

                ASSERT((int32_t)imageBufferDataLength == pvCameraFrame.Width * pvCameraFrame.Height * 4);

                ASSERT(0 == memcpy_s(
                    imageBufferData,
                    imageBufferDataLength,
                    pvCameraFrame.Image.data,
                    pvCameraFrame.Width * pvCameraFrame.Height * 4));
            }

            auto imageSource =
                ref new Windows::UI::Xaml::Media::Imaging::SoftwareBitmapSource();

            concurrency::create_task(
                imageSource->SetBitmapAsync(
                    image)).then(
                        [this, imageSource]()
            {
                _pvCameraImage->Source =
                    imageSource;
            });
        }));
    }

    void MainPage::Grid_KeyDown(
        Platform::Object^ sender,
        Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
    {
        if (e->Key == Windows::System::VirtualKey::PageUp)
        {
            MoveRecordingCursor(
                -10 /* howMuch */);
        }
        else if (e->Key == Windows::System::VirtualKey::Left)
        {
            MoveRecordingCursor(
                -1 /* howMuch */);
        }
        else if (e->Key == Windows::System::VirtualKey::Right)
        {
            MoveRecordingCursor(
                +1 /* howMuch */);
        }
        else if (e->Key == Windows::System::VirtualKey::PageDown)
        {
            MoveRecordingCursor(
                +10 /* howMuch */);
        }
    }

    void MainPage::MoveRecordingCursor(
        int32_t howMuch)
    {
        const int32_t numberOfFrames =
            (int32_t)_pvCameraFrames.size();

        if (numberOfFrames > 0)
        {
            if (howMuch < -numberOfFrames)
            {
                howMuch = -((-howMuch) % numberOfFrames);
            }

            _currentPvCameraFrame =
                (_currentPvCameraFrame + numberOfFrames + howMuch) % numberOfFrames;

            _pvCameraFrames[_currentPvCameraFrame].Load();

            UpdatePreview();
        }
    }
}

