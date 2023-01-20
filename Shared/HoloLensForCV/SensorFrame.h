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

namespace HoloLensForCV
{
	//
	// Collects information about a sensor frame -- originated on device, or remotely.
	// 
	public ref class SensorFrame sealed
	{
	public:
		SensorFrame(
			_In_ SensorType frameType,
			_In_ Windows::Foundation::DateTime timestamp,
			_In_ Windows::Graphics::Imaging::SoftwareBitmap^ softwareBitmap);

		/*SensorFrame(
			_In_ SensorType frameType,
			_In_ Windows::Foundation::DateTime timestamp,
			_In_ Windows::Graphics::Imaging::SoftwareBitmap^ softwareBitmap,
			_In_ Windows::Media::Capture::Frames::MediaFrameReference^ frame);
			*/

		SensorFrame(
			_In_ SensorType frameType,
			_In_ Windows::Foundation::DateTime timestamp,
			_In_ Windows::Graphics::Imaging::SoftwareBitmap^ softwareBitmap,
			_In_ Windows::Media::Devices::Core::CameraIntrinsics^ cameraIntrinsics);

		property SensorType FrameType;
		property Windows::Foundation::DateTime Timestamp;
		property Windows::Graphics::Imaging::SoftwareBitmap^ SoftwareBitmap;

		property Windows::Media::Devices::Core::CameraIntrinsics^ CoreCameraIntrinsics;
		property CameraIntrinsics^ SensorStreamingCameraIntrinsics;

		property Windows::Foundation::Numerics::float4x4 FrameToOrigin;
		property Windows::Foundation::Numerics::float4x4 CameraViewTransform;
		property Windows::Foundation::Numerics::float4x4 CameraProjectionTransform;
		//Windows::Foundation::Numerics::float4x4 GetAbsoluteCameraPose();
		//Windows::Foundation::Numerics::float4x4 GetCamToOrigin();

		//property int32_t imageBufferSize;
		//uint8_t* SensorFrame::GetBitmapBufferData();
		//property Platform::Array<uint8_t>^ imageBufferAsPlatformArray;

		//void SensorFrame::sensorFrameToImageBuffer();
		Platform::Array<uint8_t>^ SensorFrame::sensorFrameToImageBuffer();


	private:

		//property int32_t imageWidth;
		//property int32_t imageHeight;
		//property int32_t pixelStride;
		//property int32_t rowStride;
		//property uint8_t* bitmapBufferData;


	};
}
