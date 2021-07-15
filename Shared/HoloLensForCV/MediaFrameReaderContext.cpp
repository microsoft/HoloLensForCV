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
            latestSensorFrame = _latestSensorFrame;
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

        if (nullptr == frame)
        {
            dbg::trace(
                L"MediaFrameReaderContext::FrameArrived: _sensorType=%s (%i), frame is null",
                _sensorType.ToString()->Data(),
                (int32_t)_sensorType);

            return;
        }
        else if (nullptr == frame->VideoMediaFrame)
        {
            dbg::trace(
                L"MediaFrameReaderContext::FrameArrived: _sensorType=%s (%i), frame->VideoMediaFrame is null",
                _sensorType.ToString()->Data(),
                (int32_t)_sensorType);

            return;
        }
        else if (nullptr == frame->VideoMediaFrame->SoftwareBitmap)
        {
            dbg::trace(
                L"MediaFrameReaderContext::FrameArrived: _sensorType=%s (%i), frame->VideoMediaFrame->SoftwareBitmap is null",
                _sensorType.ToString()->Data(),
                (int32_t)_sensorType);

            return;
        }

#if DBG_ENABLE_VERBOSE_LOGGING
        dbg::trace(
            L"MediaFrameReaderContext::FrameArrived: _sensorType=%s (%i), timestamp=%llu (relative)",
            _sensorType.ToString()->Data(),
            (int32_t)_sensorType,
            frame->SystemRelativeTime->Value.Duration);
#endif

        //
        // Convert the system boot relative timestamp of exposure we've received from the media
        // frame reader into the universal time format accepted by the spatial perception APIs.
        //
        Windows::Foundation::DateTime timestamp;

        timestamp.UniversalTime =
            _timeConverter.RelativeTicksToAbsoluteTicks(
                Io::HundredsOfNanoseconds(
                    frame->SystemRelativeTime->Value.Duration)).count();

        //
        // Create a copy of the software bitmap and wrap it up with a SensorFrame.
        //
        // Per MSDN, each MediaFrameReader maintains a circular buffer of MediaFrameReference
        // objects obtained from TryAcquireLatestFrame. After all of the MediaFrameReference
        // objects in the buffer have been used, subsequent calls to TryAcquireLatestFrame will
        // cause the system to call Close (or Dispose in C#) on the oldest buffer object in
        // order to reuse it.
        //
        // Because creating a copy of the software bitmap just in case the app would want to hold
        // onto it is fairly expensive, we will let the application decide whether to make a copy
        // or risk getting the reference to the media frame closed in flight.
        //
        Windows::Graphics::Imaging::SoftwareBitmap^ softwareBitmap =
            frame->VideoMediaFrame->SoftwareBitmap;

		Windows::Media::Devices::Core::CameraIntrinsics^ cameraIntrinsics = frame->VideoMediaFrame->CameraIntrinsics;

        //
        // Finally, wrap all of the above information in a SensorFrame object and pass it
        // down to the sensor frame sink. We'll also retain a reference to the latest sensor
        // frame on this object for immediate consumption by the app.
        //
		SensorFrame^ sensorFrame =
			//   ref new SensorFrame(_sensorType, timestamp, softwareBitmap);
		//	ref new SensorFrame(_sensorType, timestamp, softwareBitmap, frame);
			ref new SensorFrame(
				_sensorType
				,timestamp
				,softwareBitmap
			//	,cameraIntrinsics
			);


        //
        // Extract the frame-to-origin transform, if the MFT exposed it:
        //
        bool frameToOriginObtained = false;
        static const Platform::Guid c_MFSampleExtension_Spatial_CameraCoordinateSystem(0x9d13c82f, 0x2199, 0x4e67, 0x91, 0xcd, 0xd1, 0xa4, 0x18, 0x1f, 0x25, 0x34);

        Windows::Perception::Spatial::SpatialCoordinateSystem^ frameCoordinateSystem = nullptr;

        if (frame->Properties->HasKey(c_MFSampleExtension_Spatial_CameraCoordinateSystem))
        {
            frameCoordinateSystem = safe_cast<Windows::Perception::Spatial::SpatialCoordinateSystem^>(
                frame->Properties->Lookup(
                    c_MFSampleExtension_Spatial_CameraCoordinateSystem));
        }

        if (nullptr != frameCoordinateSystem)
        {
            Platform::IBox<Windows::Foundation::Numerics::float4x4>^ frameToOriginReference =
                frameCoordinateSystem->TryGetTransformTo(
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


		//frame->Properties->Lookup()
        //
        // Extract camera view transform, if the MFT exposed it:
        //
        static const Platform::Guid c_MFSampleExtension_Spatial_CameraViewTransform(
            0x4e251fa4, 0x830f, 0x4770, 0x85, 0x9a, 0x4b, 0x8d, 0x99, 0xaa, 0x80, 0x9b);

        if (frame->Properties->HasKey(c_MFSampleExtension_Spatial_CameraViewTransform))
        {
            Platform::Object^ mfMtUserData =
                frame->Properties->Lookup(c_MFSampleExtension_Spatial_CameraViewTransform);
            Platform::Array<byte>^ cameraViewTransformAsPlatformArray =
                safe_cast<Platform::IBoxArray<byte>^>(mfMtUserData)->Value;
            sensorFrame->CameraViewTransform =
                *reinterpret_cast<Windows::Foundation::Numerics::float4x4*>(
                    cameraViewTransformAsPlatformArray->Data);

#if DBG_ENABLE_VERBOSE_LOGGING
            auto cameraViewTransform = sensorFrame->CameraViewTransform;
            dbg::trace(
                L"cameraViewTransform=[[%f, %f, %f, %f], [%f, %f, %f, %f], [%f, %f, %f, %f], [%f, %f, %f, %f]]",
                cameraViewTransform.m11, cameraViewTransform.m12, cameraViewTransform.m13, cameraViewTransform.m14,
                cameraViewTransform.m21, cameraViewTransform.m22, cameraViewTransform.m23, cameraViewTransform.m24,
                cameraViewTransform.m31, cameraViewTransform.m32, cameraViewTransform.m33, cameraViewTransform.m34,
                cameraViewTransform.m41, cameraViewTransform.m42, cameraViewTransform.m43, cameraViewTransform.m44);
#endif /* DBG_ENABLE_VERBOSE_LOGGING */
        }
        else
        {
            //
            // Set the CameraViewTransform to zero, making it obvious that we do not
            // have a valid pose for this frame.
            //
            Windows::Foundation::Numerics::float4x4 zero;

            memset(
                &zero,
                0 /* _Val */,
                sizeof(zero));

            sensorFrame->CameraViewTransform = zero;
        }

        //
        // Extract camera projection transform, if the MFT exposed it:
        //
        static const Platform::Guid c_MFSampleExtension_Spatial_CameraProjectionTransform(
            0x47f9fcb5, 0x2a02, 0x4f26, 0xa4, 0x77, 0x79, 0x2f, 0xdf, 0x95, 0x88, 0x6a);

        if (frame->Properties->HasKey(c_MFSampleExtension_Spatial_CameraProjectionTransform))
        {
            Platform::Object^ mfMtUserData =
                frame->Properties->Lookup(c_MFSampleExtension_Spatial_CameraProjectionTransform);
            Platform::Array<byte>^ cameraViewTransformAsPlatformArray =
                safe_cast<Platform::IBoxArray<byte>^>(mfMtUserData)->Value;
            sensorFrame->CameraProjectionTransform =
                *reinterpret_cast<Windows::Foundation::Numerics::float4x4*>(
                    cameraViewTransformAsPlatformArray->Data);

#if DBG_ENABLE_VERBOSE_LOGGING
            auto cameraProjectionTransform = sensorFrame->CameraProjectionTransform;
            dbg::trace(
                L"cameraProjectionTransform=[[%f, %f, %f, %f], [%f, %f, %f, %f], [%f, %f, %f, %f], [%f, %f, %f, %f]]",
                cameraProjectionTransform.m11, cameraProjectionTransform.m12, cameraProjectionTransform.m13, cameraProjectionTransform.m14,
                cameraProjectionTransform.m21, cameraProjectionTransform.m22, cameraProjectionTransform.m23, cameraProjectionTransform.m24,
                cameraProjectionTransform.m31, cameraProjectionTransform.m32, cameraProjectionTransform.m33, cameraProjectionTransform.m34,
                cameraProjectionTransform.m41, cameraProjectionTransform.m42, cameraProjectionTransform.m43, cameraProjectionTransform.m44);
#endif /* DBG_ENABLE_VERBOSE_LOGGING */
        }
        else
        {
            //
            // Set the CameraProjectionTransform to zero, making it obvious that we do not
            // have a valid pose for this frame.
            //
            Windows::Foundation::Numerics::float4x4 zero;

            memset(
                &zero,
                0 /* _Val */,
                sizeof(zero));

            sensorFrame->CameraProjectionTransform = zero;
        }

        //
        // See if the frame comes with HoloLens Sensor Streaming specific intrinsics...
        //
        if (frame->Properties->HasKey(SensorStreaming::MFSampleExtension_SensorStreaming_CameraIntrinsics))
        {
            Microsoft::WRL::ComPtr<SensorStreaming::ICameraIntrinsics> sensorStreamingCameraIntrinsics =
                reinterpret_cast<SensorStreaming::ICameraIntrinsics*>(
                    frame->Properties->Lookup(
                        SensorStreaming::MFSampleExtension_SensorStreaming_CameraIntrinsics));

            //
            // The visible light camera images are grayscale, but packed as 32bpp ARGB images.
            //
            unsigned int imageWidth = softwareBitmap->PixelWidth;

            if ((_sensorType == SensorType::VisibleLightLeftFront) ||
                (_sensorType == SensorType::VisibleLightLeftLeft) ||
                (_sensorType == SensorType::VisibleLightRightFront) ||
                (_sensorType == SensorType::VisibleLightRightRight))
            {
                imageWidth = imageWidth * 4;
            }

            sensorFrame->SensorStreamingCameraIntrinsics =
                ref new CameraIntrinsics(
                    sensorStreamingCameraIntrinsics,
                    imageWidth,
                    softwareBitmap->PixelHeight);
        }
        else
        {
            if (_sensorType != SensorType::PhotoVideo)
            {
                dbg::trace(
                    L"MediaFrameReaderContext::FrameArrived: _sensorType=%s (%i), MFSampleExtension_SensorStreaming_CameraIntrinsics not found!",
                    _sensorType.ToString()->Data(),
                    (int32_t)_sensorType);
            }

            sensorFrame->CoreCameraIntrinsics =
                frame->VideoMediaFrame->CameraIntrinsics;
        }

        if (nullptr != _sensorFrameSink)
        {
            _sensorFrameSink->Send(
                sensorFrame);
        }

        {
            std::lock_guard<std::mutex> latestSensorFrameMutexLockGuard(
                _latestSensorFrameMutex);

            _latestSensorFrame = sensorFrame;
        }
    }
}
