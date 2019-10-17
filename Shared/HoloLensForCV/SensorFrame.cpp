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
	SensorFrame::SensorFrame(
		_In_ SensorType frameType,
		_In_ Windows::Foundation::DateTime timestamp,
		_In_ Windows::Graphics::Imaging::SoftwareBitmap^ softwareBitmap)
	{
		FrameType = frameType;
		Timestamp = timestamp;
		SoftwareBitmap = softwareBitmap;

	}

	/*SensorFrame::SensorFrame(
		_In_ SensorType frameType,
		_In_ Windows::Foundation::DateTime timestamp,
		_In_ Windows::Graphics::Imaging::SoftwareBitmap^ softwareBitmap,
		_In_ Windows::Media::Capture::Frames::MediaFrameReference^ frame)
	{
		FrameType = frameType;
		Timestamp = timestamp;
		SoftwareBitmap = softwareBitmap;

		CoreCameraIntrinsics = frame->VideoMediaFrame->CameraIntrinsics;
		if (CoreCameraIntrinsics == nullptr) {
			dbg::trace(L"SensorFrame:VideoMediaFrame CameraIntrinsics null.");
		}
		else {
			dbg::trace(L"SensorFrame:VideoMediaFrame CameraIntrinsics not null!");
		}

	}*/

	SensorFrame::SensorFrame(
		_In_ SensorType frameType,
		_In_ Windows::Foundation::DateTime timestamp,
		_In_ Windows::Graphics::Imaging::SoftwareBitmap^ softwareBitmap,
		_In_ Windows::Media::Devices::Core::CameraIntrinsics^ cameraIntrinsics)
		{

		FrameType = frameType;
		Timestamp = timestamp;
		SoftwareBitmap = softwareBitmap;
		CoreCameraIntrinsics = cameraIntrinsics;
		if (CoreCameraIntrinsics == nullptr) {
			dbg::trace(L"SensorFrame:VideoMediaFrame CameraIntrinsics null.");
		}
		else {
			dbg::trace(L"SensorFrame:VideoMediaFrame CameraIntrinsics not null!");
		}


}

	Platform::Array<uint8_t>^
	/*void*/ SensorFrame::sensorFrameToImageBuffer() {

		Windows::Graphics::Imaging::SoftwareBitmap^ bitmap;
		Windows::Graphics::Imaging::BitmapBuffer^ bitmapBuffer;
		Windows::Foundation::IMemoryBufferReference^ bitmapBufferReference;

		int32_t imageWidth = 0;
		int32_t imageHeight = 0;
		int32_t pixelStride = 1;
		int32_t rowStride = 0;

		Platform::Array<uint8_t>^ imageBufferAsPlatformArray;
		int32_t imageBufferSize = 0;

		{
#if DBG_ENABLE_INFORMATIONAL_LOGGING
			dbg::TimerGuard timerGuard(
				L"AppMain::sensorFrameToImageBuffer: buffer preparation",
				4.0 /* minimum_time_elapsed_in_milliseconds */);
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */
			imageWidth = SoftwareBitmap->PixelWidth;
			imageHeight = SoftwareBitmap->PixelHeight;

			bitmap =
				SoftwareBitmap;

			imageWidth = bitmap->PixelWidth;
			imageHeight = bitmap->PixelHeight;

			bitmapBuffer =
				SoftwareBitmap->LockBuffer(
					Windows::Graphics::Imaging::BitmapBufferAccessMode::Read);

			bitmapBufferReference =
				bitmapBuffer->CreateReference();

			uint32_t bitmapBufferDataSize = 0;

			uint8_t* bitmapBufferData =
				Io::GetTypedPointerToMemoryBuffer<uint8_t>(
					bitmapBufferReference,
					bitmapBufferDataSize);

			switch (SoftwareBitmap->BitmapPixelFormat)
			{
			case Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8:
				pixelStride = 4;
				break;

			case Windows::Graphics::Imaging::BitmapPixelFormat::Gray16:
				pixelStride = 2;
				break;

			case Windows::Graphics::Imaging::BitmapPixelFormat::Gray8:
				pixelStride = 1;
				break;

			default:
#if DBG_ENABLE_INFORMATIONAL_LOGGING
				dbg::trace(
					L"SensorFrame::sensorFrameToImageBuffer: unrecognized bitmap pixel format, assuming 1 byte per pixel");
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

				break;
			}

			rowStride =
				imageWidth * pixelStride;

			imageBufferSize =
				imageHeight * rowStride;

			ASSERT(
				imageBufferSize == (int32_t)bitmapBufferDataSize);

			imageBufferAsPlatformArray =
				ref new Platform::Array<uint8_t>(
					bitmapBufferData,
					imageBufferSize);

			return imageBufferAsPlatformArray;

		}

	}
	/*
	Windows::Foundation::Numerics::float4x4 SensorFrame::GetAbsoluteCameraPose() {

		Windows::Foundation::Numerics::float4x4^ output = 
			ref new Windows::Foundation::Numerics::float4x4(
				1, 0, 0, 0,
				0, -1, 0, 0,
				0, 0, -1, 0,
				0, 0, 0, 1);

		Windows::Foundation::Numerics::float4x4 InvFrameToOrigin;
		Windows::Foundation::Numerics::invert(FrameToOrigin, &InvFrameToOrigin);

		output = output * (CameraViewTransform*InvFrameToOrigin);

		return output;

	}

	Windows::Foundation::Numerics::float4x4 SensorFrame::GetCamToOrigin() {

		Windows::Foundation::Numerics::float4x4 camToRef;
		Windows::Foundation::Numerics::invert(CameraViewTransform, &camToRef);

		Windows::Foundation::Numerics::float4x4 camToOrigin = camToRef * FrameToOrigin;
		
		
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

		

		return camToOrigin;

	}
	*/
}
