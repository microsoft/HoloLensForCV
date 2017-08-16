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

namespace HoloLensForCV
{
    MediaFrameReaderContext::MediaFrameReaderContext(
        _In_ SensorType sensorType,
        _In_ SpatialPerception^ spatialPerception,
        _In_opt_ ISensorFrameSink^ sensorFrameSink)
        : _sensorType(sensorType)
        , _spatialPerception(spatialPerception)
        , _sensorFrameSink(sensorFrameSink)
    {
    }

    SensorFrame^ MediaFrameReaderContext::GetLatestSensorFrame()
    {
        SensorFrame^ latestSensorFrame;

        {
            std::lock_guard<std::mutex> latestSensorFrameMutexLockGuard(
                _latestSensorFrameMutex);

            latestSensorFrame =
                _latestSensorFrame;
        }

        return latestSensorFrame;
    }

    void MediaFrameReaderContext::FrameArrived(
        Windows::Media::Capture::Frames::MediaFrameReader^ sender,
        Windows::Media::Capture::Frames::MediaFrameArrivedEventArgs^ args)
    {
        //
        // TryAcquireLatestFrame will return the latest frame that has not yet been acquired.
        // This can return null if there is no such frame, or if the reader is not in the
        // "Started" state. The latter can occur if a FrameArrived event was in flight
        // when the reader was stopped.
        //
        Windows::Media::Capture::Frames::MediaFrameReference^ frame =
            sender->TryAcquireLatestFrame();

        if (nullptr == frame ||
            nullptr == frame->VideoMediaFrame ||
            nullptr == frame->VideoMediaFrame->SoftwareBitmap)
        {
            return;
        }

#if DBG_ENABLE_VERBOSE_LOGGING
        dbg::trace(
            L"MediaFrameReaderContext::FrameArrived: _sensorType=%s (%i), timestamp=%llu (relative) and %llu (absolute)",
            _sensorType.ToString()->Data(),
            (int32_t)_sensorType,
            frame->SystemRelativeTime->Value.Duration,
            _timeConverter.RelativeTicksToAbsoluteTicks(
                frame->SystemRelativeTime->Value.Duration));
#endif

        //
        // Convert the system boot relative timestamp of exposure we've received from the media
        // frame reader into the universal time format accepted by the spatial perception APIs.
        //
        Windows::Foundation::DateTime timestamp;

        timestamp.UniversalTime =
            _timeConverter.RelativeTicksToAbsoluteTicks(
                frame->SystemRelativeTime->Value.Duration);

        //
        // Attempt to obtain the rig pose at the time of exposure start.
        //
        Windows::Perception::PerceptionTimestamp^ perceptionTimestamp =
            _spatialPerception->CreatePerceptionTimestamp(
                timestamp);

        //
        // Create a copy of the software bitmap and wrap it up with a SensorFrame.
        //
        // Per MSDN, each MediaFrameReader maintains a circular buffer of MediaFrameReference
        // objects obtained from TryAcquireLatestFrame. After all of the MediaFrameReference
        // objects in the buffer have been used, subsequent calls to TryAcquireLatestFrame will
        // cause the system to call Close (or Dispose in C#) on the oldest buffer object in
        // order to reuse it.
        //
        Windows::Graphics::Imaging::SoftwareBitmap^ softwareBitmap =
            Windows::Graphics::Imaging::SoftwareBitmap::Copy(
                frame->VideoMediaFrame->SoftwareBitmap);

        //
        // Finally, wrap all of the above information in a SensorFrame object and pass it
        // down to the sensor frame sink. We'll also retain a reference to the latest sensor
        // frame on this object for immediate consumption by the app.
        //
        SensorFrame^ sensorFrame =
            ref new SensorFrame(
                _sensorType,
                timestamp,
                softwareBitmap);

        //
        // Extract the frame-to-origin transform, if the MFT exposed it:
        //
        bool frameToOriginObtained = false;

        if (nullptr != frame->CoordinateSystem)
        {
            Platform::IBox<Windows::Foundation::Numerics::float4x4>^ frameToOriginReference =
                frame->CoordinateSystem->TryGetTransformTo(
                    _spatialPerception->GetOriginFrameOfReference()->CoordinateSystem);

            if (nullptr != frameToOriginReference)
            {
#if DBG_ENABLE_VERBOSE_LOGGING
                Windows::Foundation::Numerics::float4x4 frameToOrigin =
                    frameToOriginReference->Value;

                dbg::trace(
                    L"frameToOrigin=[[%f, %f, %f, %f], [%f, %f, %f, %f], [%f, %f, %f, %f], [%f, %f, %f, %f]]",
                    frameToOrigin.m11, frameToOrigin.m12, frameToOrigin.m13, frameToOrigin.m14,
                    frameToOrigin.m21, frameToOrigin.m22, frameToOrigin.m23, frameToOrigin.m24,
                    frameToOrigin.m31, frameToOrigin.m32, frameToOrigin.m33, frameToOrigin.m34,
                    frameToOrigin.m41, frameToOrigin.m42, frameToOrigin.m43, frameToOrigin.m44);
#endif /* DBG_ENABLE_VERBOSE_LOGGING */

                sensorFrame->FrameToOrigin =
                    frameToOriginReference->Value;

                frameToOriginObtained = true;
            }
        }

        if (!frameToOriginObtained)
        {
            //
            // Set the FrameToOrigin to zero, making it obvious that we do not
            // have a valid pose for this frame.
            //
            Windows::Foundation::Numerics::float4x4 zero;

            memset(
                &zero,
                0 /* _Val */,
                sizeof(zero));

            sensorFrame->FrameToOrigin =
                zero;
        }

        //
        // Hold a reference to the camera intrinsics.
        //
        sensorFrame->CameraIntrinsics =
            frame->VideoMediaFrame->CameraIntrinsics;

        if (nullptr != _sensorFrameSink)
        {
            _sensorFrameSink->Send(
                sensorFrame);
        }

        {
            std::lock_guard<std::mutex> latestSensorFrameMutexLockGuard(
                _latestSensorFrameMutex);

            _latestSensorFrame =
                sensorFrame;
        }
    }
}
