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

namespace ArucoMarkerTracker
{
    AppMain::AppMain(
        const std::shared_ptr<Graphics::DeviceResources>& deviceResources)
        : Holographic::AppMainBase(deviceResources)
        , _selectedHoloLensMediaFrameSourceGroupType(
            HoloLensForCV::MediaFrameSourceGroupType::HoloLensResearchModeSensors)
        , _holoLensMediaFrameSourceGroupStarted(false)
        , _undistortMapsInitialized(false)
        , _isActiveRenderer(false)
    {
    }

    void AppMain::OnHolographicSpaceChanged(
        Windows::Graphics::Holographic::HolographicSpace^ holographicSpace)
    {
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

        if (!_isActiveRenderer)
        {
            _currentSlateRenderer =
                std::make_shared<Rendering::SlateRenderer>(
                    _deviceResources);
            _slateRendererList.push_back(_currentSlateRenderer);

            // When a Pressed gesture is detected, the sample hologram will be repositioned
            // two meters in front of the user.
            _currentSlateRenderer->PositionHologram(
                pointerState->TryGetPointerPose(currentCoordinateSystem));

            _isActiveRenderer = true;
        }
        else
        {
            // Freeze frame
            _visualizationTextureList.push_back(_currentVisualizationTexture);
            _currentVisualizationTexture = nullptr;
            _isActiveRenderer = false;
        }
    }

    void AppMain::OnUpdate(
        _In_ Windows::Graphics::Holographic::HolographicFrame^ holographicFrame,
        _In_ const Graphics::StepTimer& stepTimer)
    {
        UNREFERENCED_PARAMETER(holographicFrame);

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

        for (auto& r : _slateRendererList)
        {
            r->Update(
                stepTimer);
        }

        //
        // Process sensor data received through the HoloLensForCV component.
        //
        if (!_holoLensMediaFrameSourceGroupStarted)
        {
            return;
        }

        HoloLensForCV::SensorFrame^ latestFrame;

        latestFrame =
            _holoLensMediaFrameSourceGroup->GetLatestSensorFrame(
                HoloLensForCV::SensorType::VisibleLightRightFront);

        if (nullptr == latestFrame)
        {
            return;
        }

        if (_latestSelectedCameraTimestamp.UniversalTime == latestFrame->Timestamp.UniversalTime)
        {
            return;
        }

        _latestSelectedCameraTimestamp = latestFrame->Timestamp;

        cv::Mat wrappedImage;

        rmcv::WrapHoloLensVisibleLightCameraFrameWithCvMat(
            latestFrame,
            wrappedImage);

        cv::Mat bgrImage;

        cv::cvtColor(
            wrappedImage,
            bgrImage,
            cv::COLOR_GRAY2BGR);

        cv::Ptr<cv::aruco::DetectorParameters> arucoDetectorParameters =
            cv::aruco::DetectorParameters::create();

        cv::Ptr<cv::aruco::Dictionary> arucoDictionary =
            cv::aruco::getPredefinedDictionary(
                cv::aruco::DICT_6X6_1000);

        std::vector<std::vector<cv::Point2f>> arucoMarkerCorners, arucoRejectedCandidates;
        std::vector<int32_t> arucoMarkerIds;

        cv::aruco::detectMarkers(
            wrappedImage,
            arucoDictionary,
            arucoMarkerCorners,
            arucoMarkerIds,
            arucoDetectorParameters,
            arucoRejectedCandidates);

        if (!arucoMarkerIds.empty())
        {
            cv::aruco::drawDetectedMarkers(
                bgrImage,
                arucoMarkerCorners,
                arucoMarkerIds);
        }

        cv::Mat bgraImage;

        cv::cvtColor(
            bgrImage,
            bgraImage,
            cv::COLOR_BGR2BGRA);

        OpenCVHelpers::CreateOrUpdateTexture2D(
            _deviceResources,
            bgraImage,
            _currentVisualizationTexture);
    }

    void AppMain::OnPreRender()
    {
    }

    // Renders the current frame to each holographic camera, according to the
    // current application and spatial positioning state.
    void AppMain::OnRender()
    {
        // Draw the sample hologram.
        for (size_t i = 0; i < _visualizationTextureList.size(); ++i)
        {
            _slateRendererList[i]->Render(
                _visualizationTextureList[i]);
        }

        if (_isActiveRenderer)
        {
            _currentSlateRenderer->Render(_currentVisualizationTexture);
        }
    }

    // Notifies classes that use Direct3D device resources that the device resources
    // need to be released before this method returns.
    void AppMain::OnDeviceLost()
    {

        for (auto& r : _slateRendererList)
        {
            r->ReleaseDeviceDependentResources();
        }

        _holoLensMediaFrameSourceGroup = nullptr;
        _holoLensMediaFrameSourceGroupStarted = false;

        for (auto& v : _visualizationTextureList)
        {
            v.reset();
        }
        _currentVisualizationTexture.reset();
    }

    // Notifies classes that use Direct3D device resources that the device resources
    // may now be recreated.
    void AppMain::OnDeviceRestored()
    {
        for (auto& r : _slateRendererList)
        {
            r->CreateDeviceDependentResources();
        }

        StartHoloLensMediaFrameSourceGroup();
    }

    void AppMain::StartHoloLensMediaFrameSourceGroup()
    {
        std::vector<HoloLensForCV::SensorType> enabledSensorTypes;

        enabledSensorTypes.emplace_back(
            HoloLensForCV::SensorType::VisibleLightLeftFront);

        enabledSensorTypes.emplace_back(
            HoloLensForCV::SensorType::VisibleLightRightFront);

        _holoLensMediaFrameSourceGroup =
            ref new HoloLensForCV::MediaFrameSourceGroup(
                _selectedHoloLensMediaFrameSourceGroupType,
                _spatialPerception,
                nullptr /* optionalSensorFrameSinkGroup */);

        for (const auto enabledSensorType : enabledSensorTypes)
        {
            _holoLensMediaFrameSourceGroup->Enable(
                enabledSensorType);
        }

        concurrency::create_task(_holoLensMediaFrameSourceGroup->StartAsync()).then(
            [&]()
        {
            _holoLensMediaFrameSourceGroupStarted = true;
        });
    }
}
