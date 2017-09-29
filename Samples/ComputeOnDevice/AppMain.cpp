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

namespace ComputeOnDevice
{
    AppMain::AppMain(
        const std::shared_ptr<Graphics::DeviceResources>& deviceResources)
        : Holographic::AppMainBase(deviceResources)
        , _selectedHoloLensMediaFrameSourceGroupType(
            HoloLensForCV::MediaFrameSourceGroupType::PhotoVideoCamera)
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
                HoloLensForCV::SensorType::PhotoVideo);

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

        rmcv::WrapHoloLensSensorFrameWithCvMat(
            latestFrame,
            wrappedImage);

        if (!_undistortMapsInitialized)
        {
            Windows::Media::Devices::Core::CameraIntrinsics^ cameraIntrinsics =
                latestFrame->CameraIntrinsics;

            cv::Mat cameraMatrix(3, 3, CV_64FC1);

            cv::setIdentity(cameraMatrix);

            cameraMatrix.at<double>(0, 0) = cameraIntrinsics->FocalLength.x;
            cameraMatrix.at<double>(1, 1) = cameraIntrinsics->FocalLength.y;
            cameraMatrix.at<double>(0, 2) = cameraIntrinsics->PrincipalPoint.x;
            cameraMatrix.at<double>(1, 2) = cameraIntrinsics->PrincipalPoint.y;

            cv::Mat distCoeffs(5, 1, CV_64FC1);

            distCoeffs.at<double>(0, 0) = cameraIntrinsics->RadialDistortion.x;
            distCoeffs.at<double>(1, 0) = cameraIntrinsics->RadialDistortion.y;
            distCoeffs.at<double>(2, 0) = cameraIntrinsics->TangentialDistortion.x;
            distCoeffs.at<double>(3, 0) = cameraIntrinsics->TangentialDistortion.y;
            distCoeffs.at<double>(4, 0) = cameraIntrinsics->RadialDistortion.z;

            cv::initUndistortRectifyMap(
                cameraMatrix,
                distCoeffs,
                cv::Mat_<double>::eye(3, 3) /* R */,
                cameraMatrix,
                cv::Size(wrappedImage.cols, wrappedImage.rows),
                CV_32FC1 /* type */,
                _undistortMap1,
                _undistortMap2);

            _undistortMapsInitialized = true;
        }

        cv::remap(
            wrappedImage,
            _undistortedPVCameraImage,
            _undistortMap1,
            _undistortMap2,
            cv::INTER_LINEAR);

        cv::resize(
            _undistortedPVCameraImage,
            _resizedPVCameraImage,
            cv::Size(),
            0.5 /* fx */,
            0.5 /* fy */,
            cv::INTER_AREA);

        cv::medianBlur(
            _resizedPVCameraImage,
            _blurredPVCameraImage,
            3 /* ksize */);

        cv::Canny(
            _blurredPVCameraImage,
            _cannyPVCameraImage,
            50.0,
            200.0);

        for (int32_t y = 0; y < _blurredPVCameraImage.rows; ++y)
        {
            for (int32_t x = 0; x < _blurredPVCameraImage.cols; ++x)
            {
                if (_cannyPVCameraImage.at<uint8_t>(y, x) > 64)
                {
                    _blurredPVCameraImage.at<uint32_t>(y, x) = 0xFFFF00FF;
                }
            }
        }

        OpenCVHelpers::CreateOrUpdateTexture2D(
            _deviceResources,
            _blurredPVCameraImage,
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
        _sensorFrameStreamer =
            ref new HoloLensForCV::SensorFrameStreamer();

        _sensorFrameStreamer->EnableAll();

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
