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
        _multiFrameBuffer = ref new HoloLensForCV::MultiFrameBuffer;
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

    void InitializeCameraMatrixAndDistortionCoefficients(
        _Out_ cv::Mat& camera_matrix_as_cv_mat,
        _Out_ cv::Mat& distortion_coefficients_as_cv_mat)
    {
        camera_matrix_as_cv_mat.create(
            3 /* _rows */,
            3 /* _cols */,
            CV_64FC1 /* _type */);

        cv::setIdentity(
            camera_matrix_as_cv_mat);

        camera_matrix_as_cv_mat.at<double>(0, 0) = 460.3187770118759 /* FX */;
        camera_matrix_as_cv_mat.at<double>(1, 1) = 492.7493611932309 /* FY */;
        camera_matrix_as_cv_mat.at<double>(0, 2) = 326.3250047807984 /* PX */;
        camera_matrix_as_cv_mat.at<double>(1, 2) = 282.7021871576021 /* PY */;

        distortion_coefficients_as_cv_mat.create(
            5 /* _rows */,
            1 /* _cols */,
            CV_64FC1 /* _type */);

        distortion_coefficients_as_cv_mat.at<double>(0, 0) = -0.003425237781823855 /* K0 */;
        distortion_coefficients_as_cv_mat.at<double>(1, 0) = 6.374549764809066e-05 /* K1 */;
        distortion_coefficients_as_cv_mat.at<double>(2, 0) = -3.163886258401793 /* CX */;
        distortion_coefficients_as_cv_mat.at<double>(3, 0) = -0.2860290570373986 /* CY */;
        distortion_coefficients_as_cv_mat.at<double>(4, 0) = -2.04069093750431e-06 /* K2 */;
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
            std::vector<cv::Vec3d> rvecs, tvecs;

            const float arucoMarkerLength =
                29.0f / 1000.0f;

            cv::Mat camera_matrix_as_cv_mat;
            cv::Mat distortion_coefficients_as_cv_mat;

            InitializeCameraMatrixAndDistortionCoefficients(
                camera_matrix_as_cv_mat,
                distortion_coefficients_as_cv_mat);

            cv::aruco::estimatePoseSingleMarkers(
                arucoMarkerCorners,
                arucoMarkerLength,
                camera_matrix_as_cv_mat,
                distortion_coefficients_as_cv_mat,
                rvecs,
                tvecs);

#if 0
            for (auto& rvec : rvecs)
            {
                dbg::trace(L"rvec: %f %f %f", rvec[0], rvec[1], rvec[2]);
            }

            for (auto& tvec : tvecs)
            {
                dbg::trace(L"tvec: %f %f %f", tvec[0], tvec[1], tvec[2]);
            }
#endif

            if (_markerRenderer && !tvecs.empty())
            {
                _markerRenderer->SetPosition(
                    Windows::Foundation::Numerics::float3((float)tvecs[0][0], (float)tvecs[0][1], (float)tvecs[0][2]));
            }

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

        if (!_markerRenderer)
        {
            _markerRenderer = std::make_shared<Rendering::MarkerRenderer>(
                _deviceResources);
        }

        _markerRenderer->Update(
            stepTimer);
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

        if (_markerRenderer)
        {
            _markerRenderer->Render();
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
                _multiFrameBuffer);

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
