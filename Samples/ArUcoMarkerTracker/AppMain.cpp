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

namespace ArUcoMarkerTracker
{
    struct DetectedMarker
    {
        int32_t markerId;

        Eigen::Vector3f point;
        Eigen::Vector3f dir;

        // Image position
        int x;
        int y;
    };

    std::map<int32_t, DetectedMarker> DetectArUcoMarkers(
        HoloLensForCV::SensorFrame^ frame)
    {
        std::map<int32_t, DetectedMarker> detectedMarkers;

        cv::Ptr<cv::aruco::DetectorParameters> arucoDetectorParameters =
            cv::aruco::DetectorParameters::create();

        cv::Ptr<cv::aruco::Dictionary> arucoDictionary =
            cv::aruco::getPredefinedDictionary(
                cv::aruco::DICT_6X6_1000);

        std::vector<std::vector<cv::Point2f>> arucoMarkers, arucoRejectedCandidates;
        std::vector<int32_t> arucoMarkerIds;

        cv::Mat wrappedImage;

        rmcv::WrapHoloLensVisibleLightCameraFrameWithCvMat(
            frame,
            wrappedImage);

        cv::aruco::detectMarkers(
            wrappedImage,
            arucoDictionary,
            arucoMarkers,
            arucoMarkerIds,
            arucoDetectorParameters,
            arucoRejectedCandidates);
 
        if (!arucoMarkerIds.empty())
        {
            Windows::Foundation::Numerics::float4x4 camToRef;

            if (!Windows::Foundation::Numerics::invert(frame->CameraViewTransform, &camToRef))
            {
                return detectedMarkers;
            }

            Windows::Foundation::Numerics::float4x4 camToOrigin = camToRef * frame->FrameToOrigin;
            Eigen::Vector3f camPinhole(camToOrigin.m41, camToOrigin.m42, camToOrigin.m43);

            Eigen::Matrix3f camToOriginR;

            camToOriginR(0, 0) = camToOrigin.m11;
            camToOriginR(0, 1) = camToOrigin.m12;
            camToOriginR(0, 2) = camToOrigin.m13;
            camToOriginR(1, 0) = camToOrigin.m21;
            camToOriginR(1, 1) = camToOrigin.m22;
            camToOriginR(1, 2) = camToOrigin.m23;
            camToOriginR(2, 0) = camToOrigin.m31;
            camToOriginR(2, 1) = camToOrigin.m32;
            camToOriginR(2, 2) = camToOrigin.m33;

            for (size_t i = 0; i < arucoMarkerIds.size(); ++i)
            {
                const auto& markerCorners = arucoMarkers[i];

                if (markerCorners.size() != 4)
                {
                    dbg::trace(
                        L"DetectArUcoMarkers: skipping marker id %i with %i corners",
                        arucoMarkerIds[i],
                        markerCorners.size());

                    continue;
                }

                for (size_t j = 0; j < markerCorners.size(); ++j)
                {
                    DetectedMarker detectedMarker;

                    detectedMarker.markerId =
                        arucoMarkerIds[i] * 4 + j;

                    detectedMarker.x = static_cast<int>(markerCorners[j].x);
                    detectedMarker.y = static_cast<int>(markerCorners[j].y);
                    detectedMarker.point = camPinhole;

                    Windows::Foundation::Point uv;

                    uv.X = static_cast<float>(markerCorners[j].x);
                    uv.Y = static_cast<float>(markerCorners[j].y);

                    Windows::Foundation::Point xy;

                    if (!frame->SensorStreamingCameraIntrinsics->MapImagePointToCameraUnitPlane(uv, &xy))
                    {
                        continue;
                    }

                    Eigen::Vector3f dirCam;

                    dirCam[0] = xy.X;
                    dirCam[1] = xy.Y;
                    dirCam[2] = 1.0f;

                    detectedMarker.dir =
                        camToOriginR.transpose() * dirCam;

                    detectedMarkers[detectedMarker.markerId] =
                        detectedMarker;
                }
            }
        }

        return detectedMarkers;
    }

    struct TrackedMarker
    {
        int32_t markerId;
        Eigen::Vector3f point;
    };

    std::map<int32_t, TrackedMarker> TrackArUcoMarkers(
        HoloLensForCV::SensorFrame^ leftFrame,
        HoloLensForCV::SensorFrame^ rightFrame)
    {
        std::map<int32_t, TrackedMarker> trackedMarkers;

        Windows::Foundation::Numerics::float4x4 leftCamToRef;

        if (!Windows::Foundation::Numerics::invert(leftFrame->CameraViewTransform, &leftCamToRef))
        {
            return trackedMarkers;
        }

        auto leftCamToOrigin = leftCamToRef * leftFrame->FrameToOrigin;

        auto leftDetections = DetectArUcoMarkers(leftFrame);
        auto rightDetections = DetectArUcoMarkers(rightFrame);

        for (auto leftDetectionIterator : leftDetections)
        {
            auto leftDetection = leftDetectionIterator.second;

            if (rightDetections.count(leftDetection.markerId))
            {
                auto rightDetection = rightDetections[leftDetection.markerId];

                auto a = leftDetection.point;
                auto b = leftDetection.dir;
                auto c = rightDetection.point;
                auto d = rightDetection.dir;

                Eigen::Matrix<float, 3, 2> A;

                A.col(0) = b;
                A.col(1) = -d;

                Eigen::Vector3f y = (c - a);

                Eigen::Vector2f x = (A.transpose() * A).inverse() * (A.transpose() * y);

                TrackedMarker triangulatedMarkerCorner;

                triangulatedMarkerCorner.markerId = leftDetection.markerId;
                triangulatedMarkerCorner.point = (a + b * x[0] + c + d * x[1]) * 0.5f;

                trackedMarkers[triangulatedMarkerCorner.markerId] = triangulatedMarkerCorner;
            }
        }

        return trackedMarkers;
    }

    AppMain::AppMain(
        const std::shared_ptr<Graphics::DeviceResources>& deviceResources)
        : Holographic::AppMainBase(deviceResources)
        , _selectedHoloLensMediaFrameSourceGroupType(
            HoloLensForCV::MediaFrameSourceGroupType::HoloLensResearchModeSensors)
        , _holoLensMediaFrameSourceGroupStarted(false)
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
        // Process sensor data received through the HoloLensForCV component.
        //
        if (!_holoLensMediaFrameSourceGroupStarted)
        {
            return;
        }

        OnUpdateForMarkerTracker();

        {
            std::lock_guard<std::mutex> guard(_markerRenderersMutex);

            for (auto& markerRendererIterator : _markerRenderers)
            {
                markerRendererIterator.second->Update(
                    stepTimer);
            }
        }
    }

    void AppMain::OnUpdateForMarkerTracker()
    {
        const float c_timestampTolerance = 0.001f;

        auto commonTime = _multiFrameBuffer->GetTimestampForSensorPair(
            HoloLensForCV::SensorType::VisibleLightLeftFront,
            HoloLensForCV::SensorType::VisibleLightRightFront,
            c_timestampTolerance);

        HoloLensForCV::SensorFrame^ leftFrame = _multiFrameBuffer->GetFrameForTime(
            HoloLensForCV::SensorType::VisibleLightLeftFront,
            commonTime,
            c_timestampTolerance);

        HoloLensForCV::SensorFrame^ rightFrame = _multiFrameBuffer->GetFrameForTime(
            HoloLensForCV::SensorType::VisibleLightRightFront,
            commonTime,
            c_timestampTolerance);

        if (!leftFrame || !rightFrame)
        {
#if 0
            dbg::trace(L"AppMain::OnUpdateFor3DTracking: ref of depth frame missing");
#endif

            return;
        }

        auto timeDiff100ns = leftFrame->Timestamp.UniversalTime - rightFrame->Timestamp.UniversalTime;

        if (std::abs(timeDiff100ns * 1e-7f) > 2e-3f)
        {
#if 0
            dbg::trace(L"AppMain::OnUpdateFor3DTracking: times are differeny by %f seconds", std::abs(timeDiff100ns * 1e-7f));
#endif

            return;
        }

        static long long s_previousTimestamp = 0;

        if (leftFrame->Timestamp.UniversalTime == s_previousTimestamp)
        {
#if 0
            dbg::trace(L"AppMain::OnUpdateFor3DTracking: timestamp did not change");
#endif

            return;
        }

        if (InterlockedIncrement(&_markerUpdatesInProgress) > 1)
        {
            InterlockedDecrement(&_markerUpdatesInProgress);

            return;
        }

        s_previousTimestamp = leftFrame->Timestamp.UniversalTime;

        concurrency::create_task([this, leftFrame, rightFrame]()
        {
            auto trackedMarkers = TrackArUcoMarkers(
                leftFrame,
                rightFrame);

            {
                std::lock_guard<std::mutex> guard(_markerRenderersMutex);

                for (auto& triangulatedMarkerCornerIterator : trackedMarkers)
                {
                    auto& triangulatedMarkerCorner =
                        triangulatedMarkerCornerIterator.second;

                    Windows::Foundation::Numerics::float3 p(
                        triangulatedMarkerCorner.point.x(),
                        triangulatedMarkerCorner.point.y(),
                        triangulatedMarkerCorner.point.z());

                    if (_markerRenderers.find(triangulatedMarkerCorner.markerId) == _markerRenderers.end())
                    {
#if 0
                        dbg::trace(L"AppMain::OnUpdateFor3DTracking: adding marker renderer for marker id %i", triangulatedMarkerCorner.markerId);
#endif

                        _markerRenderers[triangulatedMarkerCorner.markerId] =
                            std::make_shared<Rendering::MarkerRenderer>(
                                _deviceResources,
                                0.0035f /* markerSize */);
                    }

#if 0
                    dbg::trace(L"AppMain::OnUpdateFor3DTracking: moving marker id %i to [%f, %f, %f]", triangulatedMarkerCorner.markerId, p.x, p.y, p.z);
#endif

                    _markerRenderers[triangulatedMarkerCorner.markerId]->SetIsEnabled(true);
                    _markerRenderers[triangulatedMarkerCorner.markerId]->SetPosition(p);

                    _lastObservedMarkerTimestamp[triangulatedMarkerCorner.markerId] =
                        leftFrame->Timestamp.UniversalTime;
                }

                for (auto& markerRendererIterator : _markerRenderers)
                {
                    const long long timeSinceLastObservation =
                        leftFrame->Timestamp.UniversalTime -
                        _lastObservedMarkerTimestamp[markerRendererIterator.first];

                    const float timeSinceLastObservationInSeconds =
                        static_cast<float>(timeSinceLastObservation) * 1e-7f;

                    if (timeSinceLastObservationInSeconds > 3.0f)
                    {
                        markerRendererIterator.second->SetIsEnabled(false);
                    }
                }
            }

            InterlockedDecrement(&_markerUpdatesInProgress);
        });
    }

    void AppMain::OnPreRender()
    {
    }

    // Renders the current frame to each holographic camera, according to the
    // current application and spatial positioning state.
    void AppMain::OnRender()
    {
        std::lock_guard<std::mutex> guard(_markerRenderersMutex);

        for (auto& markerRendererIterator : _markerRenderers)
        {
            markerRendererIterator.second->Render();
        }
    }

    // Notifies classes that use Direct3D device resources that the device resources
    // need to be released before this method returns.
    void AppMain::OnDeviceLost()
    {
        _holoLensMediaFrameSourceGroup = nullptr;
        _holoLensMediaFrameSourceGroupStarted = false;

        {
            std::lock_guard<std::mutex> guard(_markerRenderersMutex);

            for (auto& markerRendererIterator : _markerRenderers)
            {
                markerRendererIterator.second.reset();
            }
        }
    }

    // Notifies classes that use Direct3D device resources that the device resources
    // may now be recreated.
    void AppMain::OnDeviceRestored()
    {
        {
            std::lock_guard<std::mutex> guard(_markerRenderersMutex);

            for (auto& markerRendererIterator : _markerRenderers)
            {
                markerRendererIterator.second->CreateDeviceDependentResources();
            }
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

        _multiFrameBuffer =
            ref new HoloLensForCV::MultiFrameBuffer();

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
