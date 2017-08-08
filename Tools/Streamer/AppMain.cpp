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
#include "AppMain.h"

using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;
using namespace Windows::Networking;
using namespace Windows::Networking::Connectivity;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;
using namespace Windows::Graphics::Holographic;
using namespace Windows::Perception::Spatial;
using namespace Windows::UI::Input::Spatial;
using namespace std::placeholders;

namespace Streamer
{
    // Loads and initializes application assets when the application is loaded.
    AppMain::AppMain(const std::shared_ptr<Graphics::DeviceResources>& deviceResources)
        : Holographic::AppMainBase(deviceResources)
        , _selectedHoloLensMediaFrameSourceGroupType(
            HoloLensForCV::MediaFrameSourceGroupType::PhotoVideoCamera)
        , _holoLensMediaFrameSourceGroupStarted(false)
    {
    }

    void AppMain::OnHolographicSpaceChanged(
        Windows::Graphics::Holographic::HolographicSpace^ holographicSpace)
    {
        //
        // Initialize the camera preview hologram.
        //
        _slateRenderer =
            std::make_unique<Rendering::SlateRenderer>(
                _deviceResources);

        //
        // Initialize the HoloLens media frame readers
        //
        StartHoloLensMediaFrameSourceGroup();
    }

    void AppMain::OnSpatialInput(
        _In_ Windows::UI::Input::Spatial::SpatialInteractionSourceState^ pointerState)
    {
        Windows::Perception::Spatial::SpatialCoordinateSystem^ currentCoordinateSystem =
            _spatialPerception->GetOriginFrameOfReference()->CoordinateSystem;

        // When a Pressed gesture is detected, the sample hologram will be repositioned
        // two meters in front of the user.
        _slateRenderer->PositionHologram(
            pointerState->TryGetPointerPose(currentCoordinateSystem));
    }

    // Updates the application state once per frame.
    void AppMain::OnUpdate(
        _In_ Windows::Graphics::Holographic::HolographicFrame^ holographicFrame,
        _In_ const Graphics::StepTimer& stepTimer)
    {
        dbg::TimerGuard timerGuard(
            L"AppMain::OnUpdate",
            30.0 /* minimum_time_elapsed_in_milliseconds */);

        //
        // Update scene objects.
        //
        // Put time-based updates here. By default this code will run once per frame,
        // but if you change the StepTimer to use a fixed time step this code will
        // run as many times as needed to get to the current step.
        //
        _slateRenderer->Update(
            stepTimer);

        if (!_holoLensMediaFrameSourceGroupStarted)
        {
            return;
        }

        HoloLensForCV::SensorFrame^ latestCameraPreviewFrame =
            _holoLensMediaFrameSourceGroup->GetLatestSensorFrame(
                HoloLensForCV::SensorType::PhotoVideo);

        if (nullptr == latestCameraPreviewFrame)
        {
            return;
        }

        if (_cameraPreviewTimestamp.UniversalTime == latestCameraPreviewFrame->Timestamp.UniversalTime)
        {
            return;
        }

        _cameraPreviewTimestamp = latestCameraPreviewFrame->Timestamp;

        if (nullptr == _cameraPreviewTexture)
        {
            _cameraPreviewTexture =
                std::make_shared<Rendering::Texture2D>(
                    _deviceResources,
                    latestCameraPreviewFrame->SoftwareBitmap->PixelWidth,
                    latestCameraPreviewFrame->SoftwareBitmap->PixelHeight,
                    DXGI_FORMAT_B8G8R8A8_UNORM);
        }

        {
            void* mappedTexture =
                _cameraPreviewTexture->MapCPUTexture<void>(
                    D3D11_MAP_WRITE /* mapType */);

            Windows::Graphics::Imaging::SoftwareBitmap^ bitmap =
                latestCameraPreviewFrame->SoftwareBitmap;

            REQUIRES(Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8 == bitmap->BitmapPixelFormat);

            Windows::Graphics::Imaging::BitmapBuffer^ bitmapBuffer =
                bitmap->LockBuffer(
                    Windows::Graphics::Imaging::BitmapBufferAccessMode::Read);

            uint32_t pixelBufferDataLength = 0;

            uint8_t* pixelBufferData =
                Io::GetPointerToMemoryBuffer(
                    bitmapBuffer->CreateReference(),
                    pixelBufferDataLength);

            const int32_t bytesToCopy =
                _cameraPreviewTexture->GetWidth() * _cameraPreviewTexture->GetHeight() * 4;

            ASSERT(static_cast<uint32_t>(bytesToCopy) == pixelBufferDataLength);

            memcpy_s(
                mappedTexture,
                bytesToCopy,
                pixelBufferData,
                pixelBufferDataLength);

            _cameraPreviewTexture->UnmapCPUTexture();
        }

        _cameraPreviewTexture->CopyCPU2GPU();
    }

    void AppMain::OnPreRender()
    {
    }

    // Renders the current frame to each holographic camera, according to the
    // current application and spatial positioning state.
    void AppMain::OnRender()
    {
        // Draw the sample hologram.
        _slateRenderer->Render(
            _cameraPreviewTexture);
    }

    // Notifies classes that use Direct3D device resources that the device resources
    // need to be released before this method returns.
    void AppMain::OnDeviceLost()
    {
        _slateRenderer->ReleaseDeviceDependentResources();

        _holoLensMediaFrameSourceGroup = nullptr;
        _holoLensMediaFrameSourceGroupStarted = false;

        _cameraPreviewTexture.reset();
    }

    // Notifies classes that use Direct3D device resources that the device resources
    // may now be recreated.
    void AppMain::OnDeviceRestored()
    {
        _slateRenderer->CreateDeviceDependentResources();

        StartHoloLensMediaFrameSourceGroup();
    }

    void AppMain::StartHoloLensMediaFrameSourceGroup()
    {
        _sensorFrameStreamer =
            ref new HoloLensForCV::SensorFrameStreamer();

        _sensorFrameStreamer->Enable(
            HoloLensForCV::SensorType::PhotoVideo);

        _holoLensMediaFrameSourceGroup =
            ref new HoloLensForCV::MediaFrameSourceGroup(
                _selectedHoloLensMediaFrameSourceGroupType,
                _spatialPerception,
                _sensorFrameStreamer);

        concurrency::create_task(_holoLensMediaFrameSourceGroup->StartAsync()).then(
            [&]()
        {
            _holoLensMediaFrameSourceGroupStarted = true;
        });
    }
}
