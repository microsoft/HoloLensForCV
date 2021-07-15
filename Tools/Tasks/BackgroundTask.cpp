#include "pch.h"
#include "BackgroundTask.h"

#include <locale>
#include <codecvt>
#include <string>

using namespace Tasks;
using namespace dbg;
using namespace concurrency;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Networking::Sockets;
using namespace Windows::Security::Cryptography::Certificates;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Web;
using namespace Windows::Data::Json;
using namespace Windows::Security::Cryptography;
using namespace Windows::Globalization::DateTimeFormatting;


namespace Tasks {
static int count = 0;
static long long s_previousTimestamp = 0;
static int id;
static int pubCount;
static bool connected = false;

static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

static const Platform::String^ base64_charsp =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";


static inline bool is_base64(unsigned char c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}


void BackgroundTask::Run(IBackgroundTaskInstance^ taskInstance) {


	dbg::trace(L"Background Task::Run called");
	
	id = 0;
	pubCount = 0;
	_selectedHoloLensMediaFrameSourceGroupType = HoloLensForCV::MediaFrameSourceGroupType::HoloLensResearchModeSensors;
	_holoLensMediaFrameSourceGroupStarted = false;
	keepRunning = true;
	auto cost = BackgroundWorkCost::CurrentBackgroundWorkCost;
	auto settings = ApplicationData::Current->LocalSettings;
	settings->Values->Insert("BackgroundWorkCost", cost.ToString());
	pairingInProgress = false;
	taskInstance->Canceled += ref new BackgroundTaskCanceledEventHandler(this, &BackgroundTask::OnCanceled);
	
	
	TaskDeferral = taskInstance->GetDeferral();
	TaskInstance = taskInstance;

	Connect();
	
	inProcess = false;
	
	while (keepRunning) {

		if (!_holoLensMediaFrameSourceGroupStarted) {
			if (!inProcess) { 	
				
				StartHoloLensMediaFrameSourceGroup(); 
			
			}
		}		
		
		else if (_holoLensMediaFrameSourceGroupStarted && !pairingInProgress) {
			
			concurrency::create_task(BackgroundTask::RunAsync()).then([&]() {
				
				pairingInProgress = false;
				count++;
	
			});
		}	
	}
	
	//TO-DO, send an output indicating that the bg task is closing

	dbg::trace(L"Background Task Completed.");

	TaskDeferral->Complete();

}


void BackgroundTask::StartHoloLensMediaFrameSourceGroup()
{
	std::vector<HoloLensForCV::SensorType> enabledSensorTypes;
	inProcess = true;
	//
	// Enabling all of the Research Mode sensors at the same time can be quite expensive
	// performance-wise. It's best to scope down the list of enabled sensors to just those
	// that are required for a given task. 

	//In this example, selected: the visible light front cameras.


	enabledSensorTypes.emplace_back(
		HoloLensForCV::SensorType::VisibleLightLeftFront);

	enabledSensorTypes.emplace_back(
		HoloLensForCV::SensorType::VisibleLightRightFront);

	_multiFrameBuffer =
		ref new HoloLensForCV::MultiFrameBuffer();

	_spatialPerception = ref new HoloLensForCV::SpatialPerception();

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
	dbg::trace(L"calling startasync for mediaframesourcegroup");
	concurrency::create_task(_holoLensMediaFrameSourceGroup->StartAsync()).then(
		[&]()
	{
		dbg::trace(L"media frame source group started.");
		inProcess = false;
		_holoLensMediaFrameSourceGroupStarted = true;


		leftCameraIntrinsics = _holoLensMediaFrameSourceGroup->GetCameraIntrinsics(HoloLensForCV::SensorType::VisibleLightLeftFront);
		rightCameraIntrinsics = _holoLensMediaFrameSourceGroup->GetCameraIntrinsics(HoloLensForCV::SensorType::VisibleLightRightFront);

		if (leftCameraIntrinsics == nullptr) {

			dbg::trace(L"Camera Intrinsics nullptr.");	//Not calibrated	
		}
	});
}

Windows::Foundation::IAsyncAction^ BackgroundTask::RunAsync() {

	return concurrency::create_async(
		[this]()
	{

		return StreamAsync();
	});

}


concurrency::task<void> BackgroundTask::StreamAsync() {

	//dbg::trace(L"Background Task::StreamAsync called");

	Concurrency::task<void> streamTask =
		Concurrency::task_from_result();

	pairingInProgress = true;
	const float c_timestampTolerance = 0.001f;

	auto commonTime = _multiFrameBuffer->GetTimestampForSensorPair(
		HoloLensForCV::SensorType::VisibleLightLeftFront,
		HoloLensForCV::SensorType::VisibleLightRightFront,
		c_timestampTolerance);
	
	if (_multiFrameBuffer->GetFrameForTime(HoloLensForCV::SensorType::VisibleLightLeftFront,
		commonTime,
		c_timestampTolerance) == nullptr || _multiFrameBuffer->GetFrameForTime(HoloLensForCV::SensorType::VisibleLightRightFront,
			commonTime,
			c_timestampTolerance) == nullptr  ) {
		dbg::trace(L"a frame nullptr!");
		return streamTask;
	}
	else {
		HoloLensForCV::SensorFrame^ leftFrame;
		HoloLensForCV::SensorFrame^ rightFrame;
		try {
			leftFrame = _multiFrameBuffer->GetFrameForTime(
				HoloLensForCV::SensorType::VisibleLightLeftFront,
				commonTime,
				c_timestampTolerance);

			rightFrame = _multiFrameBuffer->GetFrameForTime(
				HoloLensForCV::SensorType::VisibleLightRightFront,
				commonTime,
				c_timestampTolerance);

			if (leftFrame == nullptr || rightFrame == nullptr) {
				return streamTask;
			}
			/*}	catch (Exception^ f) {

				dbg::trace(L"StreamAsync: Exception!");
				delete leftFrame;
				delete rightFrame;

				return streamTask;

			}*/

			auto timeDiff100ns = leftFrame->Timestamp.UniversalTime - rightFrame->Timestamp.UniversalTime;

			if (std::abs(timeDiff100ns * 1e-7f) > 2e-3f)
			{
				dbg::trace(L"StreamAsync: times are different by %f seconds", std::abs(timeDiff100ns * 1e-7f));

			}


			if (commonTime.UniversalTime == s_previousTimestamp)
			{
				//dbg::trace(L"StreamAsync: timestamp did not change");
				return streamTask;
			}

			s_previousTimestamp = commonTime.UniversalTime;
			//code to retrieve Array<uint8:t>^ needs to be cleaned 
			auto outPair = std::make_pair(leftFrame, rightFrame);
			std::pair<uint8_t*, int32_t> leftData = sensorFrameToImageBufferPair(outPair.first);
			std::pair<uint8_t*, int32_t> rightData = sensorFrameToImageBufferPair(outPair.second);
			Platform::Array<uint8_t>^ imageBufferAsPlatformArrayLeft;
			Platform::Array<uint8_t>^ imageBufferAsPlatformArrayRight;

			imageBufferAsPlatformArrayLeft =
				ref new Platform::Array<uint8_t>(
					leftData.first,
					leftData.second);
			imageBufferAsPlatformArrayRight =
				ref new Platform::Array<uint8_t>(
					rightData.first,
					rightData.second);


			dbg::trace(
				L"BackgroundTask::StreamAsync: %i platform arrays created at %i, size %i",
				count,
				outPair.first->Timestamp,
				imageBufferAsPlatformArrayLeft->Value->Length);

			int i = 300000;
			dbg::trace(
				L"BackgroundTask::StreamAsync: data index %i value %i ",
				i,
				imageBufferAsPlatformArrayLeft->Value[i]);


			Windows::Foundation::Numerics::float4x4 leftTransform = leftFrame->CameraViewTransform;
			Windows::Foundation::Numerics::float4x4 rightTransform = rightFrame->CameraViewTransform;
			dbg::trace(L"Left Camera View Transform %i %i %i %i, %i %i %i %i, %i %i %i %i, %i %i %i %i ",
				leftTransform.m11, leftTransform.m12, leftTransform.m13, leftTransform.m14, leftTransform.m21, leftTransform.m22, leftTransform.m23, leftTransform.m24,
				leftTransform.m31, leftTransform.m32, leftTransform.m33, leftTransform.m34, leftTransform.m41, leftTransform.m42, leftTransform.m43, leftTransform.m44
			);
			dbg::trace(L"Right Camera View Transform %i %i %i %i, %i %i %i %i, %i %i %i %i, %i %i %i %i ",
				rightTransform.m11, rightTransform.m12, rightTransform.m13, rightTransform.m14, rightTransform.m21, rightTransform.m22, rightTransform.m23, rightTransform.m24,
				rightTransform.m31, rightTransform.m32, rightTransform.m33, rightTransform.m34, rightTransform.m41, rightTransform.m42, rightTransform.m43, rightTransform.m44
			);

			auto leftFrameToOrigin = leftFrame->FrameToOrigin;
			auto rightFrameToOrigin = rightFrame->FrameToOrigin;
			dbg::trace(L"Left Camera Frame To Origin %i %i %i %i, %i %i %i %i, %i %i %i %i, %i %i %i %i ",
				leftFrameToOrigin.m11, leftFrameToOrigin.m12, leftFrameToOrigin.m13, leftFrameToOrigin.m14, leftFrameToOrigin.m21, leftFrameToOrigin.m22, leftFrameToOrigin.m23, leftFrameToOrigin.m24,
				leftFrameToOrigin.m31, leftFrameToOrigin.m32, leftFrameToOrigin.m33, leftFrameToOrigin.m34, leftFrameToOrigin.m41, leftFrameToOrigin.m42, leftFrameToOrigin.m43, leftFrameToOrigin.m44
			);

			dbg::trace(L"Right Camera Frame To Origin %i %i %i %i, %i %i %i %i, %i %i %i %i, %i %i %i %i ",
				rightFrameToOrigin.m11, rightFrameToOrigin.m12, rightFrameToOrigin.m13, rightFrameToOrigin.m14, rightFrameToOrigin.m21, rightFrameToOrigin.m22, rightFrameToOrigin.m23, rightFrameToOrigin.m24,
				rightFrameToOrigin.m31, rightFrameToOrigin.m32, rightFrameToOrigin.m33, rightFrameToOrigin.m34, rightFrameToOrigin.m41, rightFrameToOrigin.m42, rightFrameToOrigin.m43, rightFrameToOrigin.m44
			);


			auto leftPose = GetAbsoluteCameraPose(leftFrame);
			auto rightPose = GetAbsoluteCameraPose(rightFrame);

			dbg::trace(L"Left Pose %i %i %i %i, %i %i %i %i, %i %i %i %i, %i %i %i %i ",
				leftPose.m11, leftPose.m12, leftPose.m13, leftPose.m14, leftPose.m21, leftPose.m22, leftPose.m23, leftPose.m24,
				leftPose.m31, leftPose.m32, leftPose.m33, leftPose.m34, leftPose.m41, leftPose.m42, leftPose.m43, leftPose.m44
			);
			dbg::trace(L"Right Pose %i %i %i %i, %i %i %i %i, %i %i %i %i, %i %i %i %i ",
				rightPose.m11, rightPose.m12, rightPose.m13, rightPose.m14, rightPose.m21, rightPose.m22, rightPose.m23, rightPose.m24,
				rightPose.m31, rightPose.m32, rightPose.m33, rightPose.m34, rightPose.m41, rightPose.m42, rightPose.m43, rightPose.m44
			);

			auto leftCamToWorld = GetCamToWorld(leftPose);
			auto rightCamToWorld = GetCamToWorld(rightPose);
			Windows::Foundation::Numerics::invert(GetCamToWorld(leftPose), &leftCamToWorld);

			dbg::trace(L"Left CamToWorld %i %i %i %i, %i %i %i %i, %i %i %i %i, %i %i %i %i ",
				leftCamToWorld.m11, leftCamToWorld.m12, leftCamToWorld.m13, leftCamToWorld.m14, leftCamToWorld.m21, leftCamToWorld.m22, leftCamToWorld.m23, leftCamToWorld.m24,
				leftCamToWorld.m31, leftCamToWorld.m32, leftCamToWorld.m33, leftCamToWorld.m34, leftCamToWorld.m41, leftCamToWorld.m42, leftCamToWorld.m43, leftCamToWorld.m44
			);
			dbg::trace(L"Right CamToWorld %i %i %i %i, %i %i %i %i, %i %i %i %i, %i %i %i %i ",
				rightCamToWorld.m11, rightCamToWorld.m12, rightCamToWorld.m13, rightCamToWorld.m14, rightCamToWorld.m21, rightCamToWorld.m22, rightCamToWorld.m23, rightCamToWorld.m24,
				rightCamToWorld.m31, rightCamToWorld.m32, rightCamToWorld.m33, rightCamToWorld.m34, rightCamToWorld.m41, rightCamToWorld.m42, rightCamToWorld.m43, rightCamToWorld.m44
			);

			/*auto leftCamToOrigin = GetCamToOrigin(leftFrame);
			auto rightCamToOrigin = GetCamToOrigin(rightFrame);



			dbg::trace(L"Left CamToOrigin %s %i %i %i, %i %i %i %i, %i %i %i %i, %i %i %i %i ",
				leftCamToOrigin.m11, leftCamToOrigin.m12, leftCamToOrigin.m13, leftCamToOrigin.m14, leftCamToOrigin.m21, leftCamToOrigin.m22, leftCamToOrigin.m23, leftCamToOrigin.m24,
				leftCamToOrigin.m31, leftCamToOrigin.m32, leftCamToOrigin.m33, leftCamToOrigin.m34, leftCamToOrigin.m41, leftCamToOrigin.m42, leftCamToOrigin.m43, leftCamToOrigin.m44
			);
			dbg::trace(L"Right CamToOrigin %i %i %i %i, %i %i %i %i, %i %i %i %i, %i %i %i %i ",
				rightCamToOrigin.m11, rightCamToOrigin.m12, rightCamToOrigin.m13, rightCamToOrigin.m14, rightCamToOrigin.m21, rightCamToOrigin.m22, rightCamToOrigin.m23, rightCamToOrigin.m24,
				rightCamToOrigin.m31, rightCamToOrigin.m32, rightCamToOrigin.m33, rightCamToOrigin.m34, rightCamToOrigin.m41, rightCamToOrigin.m42, rightCamToOrigin.m43, rightCamToOrigin.m44
			);
			*/
			if (connected) {

				Publish("/hololens", commonTime, leftFrame, imageBufferAsPlatformArrayLeft, rightFrame, imageBufferAsPlatformArrayRight);
				PublishHello("/listener", "Hello from the HoloLens.");
				pubCount++;
		
			}
		}
		catch (Exception^ e) {

			dbg::trace(L"StreamAsync: Exception.");
			return streamTask;
		}
		
		//NULL VALUES RECEIVED. Cameras need to be calibrated, and then calibration values, ie, core camera intrinsics, sent back to the HoloLens (How?)
		//This includes: Camera Matrix K with parameters fx, fy, cx, cy. 2 degrees of radial distortion and a tangential distortion.
		

		/*auto leftRadialDistortion = leftFrame->CoreCameraIntrinsics->RadialDistortion;
		auto leftTangentialDistortian = leftFrame->CoreCameraIntrinsics->TangentialDistortion;

		dbg::trace(L"radial distortian: %i %i %i tangential distortian: %i %i, focal length: %i %i, principal point: %i %i, image height: %i, width: %i",
			leftRadialDistortion.x, leftRadialDistortion.y, leftRadialDistortion.z,
			leftTangentialDistortian.x, leftTangentialDistortian.y,
			leftFrame->CoreCameraIntrinsics->FocalLength.x, leftFrame->CoreCameraIntrinsics->FocalLength.y,
			leftFrame->CoreCameraIntrinsics->PrincipalPoint.x, leftFrame->CoreCameraIntrinsics->PrincipalPoint.y,
			leftFrame->CoreCameraIntrinsics->ImageHeight, leftFrame->CoreCameraIntrinsics->ImageWidth);
			*/

			/*
			auto leftUndistProjTransform = leftFrame->CoreCameraIntrinsics->UndistortedProjectionTransform;
			dbg::trace(L"Left Camera Undistorted Projection Transform %i %i %i %i, %i %i %i %i, %i %i %i %i, %i %i %i %i ",
				leftUndistProjTransform.m11, leftUndistProjTransform.m12, leftUndistProjTransform.m13, leftUndistProjTransform.m14, leftUndistProjTransform.m21, leftUndistProjTransform.m22, leftUndistProjTransform.m23, leftUndistProjTransform.m24,
				leftUndistProjTransform.m31, leftUndistProjTransform.m32, leftUndistProjTransform.m33, leftUndistProjTransform.m34, leftUndistProjTransform.m41, leftUndistProjTransform.m42, leftUndistProjTransform.m43, leftUndistProjTransform.m44
			);
			


			//DEBUGGING LOOP FOR ARRAY
			auto printer = false;
			int i = 307200 - 5;
			int j = i + 5;
			while (!printer) {

				dbg::trace(
					L"BackgroundTask::StreamAsync: data index %i value %i ",
					i,
					imageBufferAsPlatformArrayLeft->Value[i]);
				i = i + 1;
				if (i >= j) {
					printer = true;
				}
			}
			//END DEBUGGING LOOP FOR ARRAY
			
			/*
				Platform::Array<uint8_t>^ imageBufferLeft = sensorFrameToImageBuffer(leftFrame);
				//int x = imageBufferLeft->Length - 150;

				if (leftFrame != nullptr && rightFrame != nullptr) {
					dbg::trace(L" %i timestamp, array of length %i, value %i at index %i ",
						count,
						leftFrame->Timestamp.UniversalTime,
						imageBufferLeft->Length,
						imageBufferLeft->Value[imageBufferLeft->Length - 50],
						imageBufferLeft->Length - 50 );
				}
				else {
					dbg::trace(L"Waiting for frames to not be null");
				}
			*/
		return streamTask;
	

	}
}

Windows::Foundation::Numerics::float4x4 BackgroundTask::GetAbsoluteCameraPose(HoloLensForCV::SensorFrame^ frame) {
	
	Windows::Foundation::Numerics::float4x4 interm;

	memset( &interm , 0, sizeof(interm));
	
	interm.m11 = 1; interm.m22 = 1; interm.m33 = 1; interm.m44 = 1;
	Windows::Foundation::Numerics::float4x4 InvFrameToOrigin;
	Windows::Foundation::Numerics::invert(frame->FrameToOrigin, &InvFrameToOrigin);

	auto pose = interm * frame->CameraViewTransform*InvFrameToOrigin;
	return pose;

}

Windows::Foundation::Numerics::float4x4 BackgroundTask::GetCamToWorld(Windows::Foundation::Numerics::float4x4 pose) {

	Windows::Foundation::Numerics::float4x4 output;
	Windows::Foundation::Numerics::invert(pose, &output);
	return output;
}

Windows::Foundation::Numerics::float4x4 BackgroundTask::GetCamToOrigin(HoloLensForCV::SensorFrame^ frame) {

	Windows::Foundation::Numerics::float4x4 camToRef;
	Windows::Foundation::Numerics::float4x4 camToOrigin;
	if (!Windows::Foundation::Numerics::invert(frame->CameraViewTransform, &camToRef)) {
		
		dbg::trace(L"Could not invert!");
		memset(&camToOrigin, 0, sizeof(camToOrigin));
	
	}
	else {

		camToOrigin = camToRef * frame->FrameToOrigin;
		//CAMERA EXTRINSICS
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
		//4x4 camToOrigin transform not affine. Projection warping(?, I'm not solid on the math behind this one) parameters observed in fourth row.

	}
		return camToOrigin;
	
}


std::pair<uint8_t*, int32_t>  BackgroundTask::sensorFrameToImageBufferPair(HoloLensForCV::SensorFrame^ sensorFrame) {

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

		bitmap =
			sensorFrame->SoftwareBitmap;

		imageWidth = bitmap->PixelWidth;
		imageHeight = bitmap->PixelHeight;

		bitmapBuffer =
			bitmap->LockBuffer(
				Windows::Graphics::Imaging::BitmapBufferAccessMode::Read);

		bitmapBufferReference =
			bitmapBuffer->CreateReference();

		uint32_t bitmapBufferDataSize = 0;

		uint8_t* bitmapBufferData =
			Io::GetTypedPointerToMemoryBuffer<uint8_t>(
				bitmapBufferReference,
				bitmapBufferDataSize);

		switch (bitmap->BitmapPixelFormat)
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
				L"AppMain::sensorFrameToImageBuffer: unrecognized bitmap pixel format, assuming 1 byte per pixel");
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

			break;
		}

		rowStride =
			imageWidth * pixelStride;

		imageBufferSize =
			imageHeight * rowStride;

		ASSERT(
			imageBufferSize == (int32_t)bitmapBufferDataSize);


		std::pair<uint8_t*, int32_t> outPair = std::make_pair(bitmapBufferData, imageBufferSize);

		/*imageBufferAsPlatformArray =
			ref new Platform::Array<uint8_t>(
				bitmapBufferData,
				imageBufferSize);*/
		return outPair;
	}
}

//This does not work; I don't know why. Probably some pointers related thing I'm not able to comprehend
//Hence a slightly messier route used, which involved std::pair objects.

Platform::Array<uint8_t>^ BackgroundTask::sensorFrameToImageBuffer(HoloLensForCV::SensorFrame^ sensorFrame) {

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

		bitmap =
			sensorFrame->SoftwareBitmap;

		imageWidth = bitmap->PixelWidth;
		imageHeight = bitmap->PixelHeight;

		bitmapBuffer =
			bitmap->LockBuffer(
				Windows::Graphics::Imaging::BitmapBufferAccessMode::Read);

		bitmapBufferReference =
			bitmapBuffer->CreateReference();

		uint32_t bitmapBufferDataSize = 0;

		uint8_t* bitmapBufferData =
			Io::GetTypedPointerToMemoryBuffer<uint8_t>(
				bitmapBufferReference,
				bitmapBufferDataSize);

		switch (bitmap->BitmapPixelFormat)
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
				L"AppMain::sensorFrameToImageBuffer: unrecognized bitmap pixel format, assuming 1 byte per pixel");
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

void BackgroundTask::OnCanceled(IBackgroundTaskInstance^ taskInstance, BackgroundTaskCancellationReason reason) {
	CancelRequested = true;
	CancelReason = reason;
}

	void BackgroundTask::SetBusy(bool value) {
		busy = value;
	}

	void BackgroundTask::Connect() {
		SetBusy(true);
		ConnectAsync().then([this]() {

			SetBusy(false);
			connected = true;

			Advertise("/listener", "std_msgs/String");
			Advertise("/hololens", "project/HoloLensStereo");
			PublishHello("/listener", "Hello from the HoloLens.");

		});
	}


	task<void> BackgroundTask::ConnectAsync() {
		Uri^ server = ref new Uri(
			"ws://141.3.81.144:9090/"
		);
		if (!server)
		{
			dbg::trace(L"Uri false");
			return task_from_result();
		}

		messageWebSocket = ref new MessageWebSocket();
		messageWebSocket->Control->MessageType = SocketMessageType::Utf8;
		
		/*  won't be receiving messages. refer to the Websockets section in Windows universal samples for UWP apps. Very comprehensive.
		//Could be useful for modelling communication with CV analyser as ROS services instead of ROS messages. This should be possible.
		
		messageWebSocket->MessageReceived += 
			ref new TypedEventHandler<
			MessageWebSocket^,
			MessageWebSocketMessageReceivedEventArgs^>(this, &MessageReceived);
		messageWebSocket->Closed += ref new TypedEventHandler<IWebSocket^, WebSocketClosedEventArgs^>(this, &OnClosed);
		*/

		return create_task(messageWebSocket->ConnectAsync(server))
			.then([this](task<void> previousTask)
		{
			try
			{
				// Reraise any exception that occurred in the task.
				previousTask.get();
				dbg::trace(L"web socket connected.");
			}
			catch (Exception^ ex)
			{
				// Error happened during connect operation.
				delete messageWebSocket;
				messageWebSocket = nullptr;
				dbg::trace(L"ConnectAsync: Error during connection operation.");
				return;
			}

	//	/* REMOVE
			// The default DataWriter encoding is Utf8.
			dbg::trace(L"ConnectAsync: Connected.");
			messageWriter = ref new DataWriter(messageWebSocket->OutputStream);
			dbg::trace(L"ConnectAsync: DataWriter initialized.");
		});


	}

	
	void BackgroundTask::Send(Platform::String^ message) {
		SetBusy(true);

		SendAsync(message).then([this]() {
			SetBusy(false);
		});

	}

	task<void> BackgroundTask::SendAsync(Platform::String^ message) {

		auto a = messageWriter->WriteString(message);
		dbg::trace(L"SendAsync: %i string length, in bytes. ", a);
		return concurrency::create_task(messageWriter->StoreAsync()).then(
			[this](task<unsigned int> previousTask) {
			previousTask.get();

			dbg::trace(L"Send Complete");
		});
	}

	void BackgroundTask::Send(Platform::Array<uint8_t>^ message) {
		
		SetBusy(true);
		SendAsync(message).then([this]() {
			SetBusy(false);
		});

	}

	task<void> BackgroundTask::SendAsync(Platform::Array<uint8_t>^ message) {

		messageWriter->WriteBytes(message->Value);
		//dbg::trace(L"SendAsync: %i array length, in bytes. ", message);
		return concurrency::create_task(messageWriter->StoreAsync()).then(
			[this](task<unsigned int> previousTask) {
			previousTask.get();

			dbg::trace(L"Send Complete");
		});

	}

	void BackgroundTask::Send(DateTime timestamp) {

		SetBusy(true);
		SendAsync(timestamp).then([this]() {
			SetBusy(false);
		});
	}

	task<void> BackgroundTask::SendAsync(DateTime timestamp) {
		messageWriter->WriteDateTime(timestamp);

		return concurrency::create_task(messageWriter->StoreAsync()).then(
			[this](task<unsigned int> previousTask) {

			previousTask.get();
			dbg::trace(L"Send Complete");

		});
	
	}


	void BackgroundTask::Advertise(Platform::String^ topic, Platform::String^ messageType) {

	
		JsonObject^ jsonObject = ref new JsonObject();
		jsonObject->Insert("op", JsonValue::CreateStringValue("advertise"));
		jsonObject->Insert("topic", JsonValue::CreateStringValue(topic));
		jsonObject->Insert("type", JsonValue::CreateStringValue(messageType));

		String^ output = jsonObject->Stringify();

		dbg::trace(L"Advertise: %s",
			output->Data()
		);
		Send(output);
	}

	void BackgroundTask::Publish(Platform::String^ topic, Windows::Foundation::DateTime commonTimestamp, HoloLensForCV::SensorFrame^ leftFrame, Platform::Array<uint8_t>^ leftData, HoloLensForCV::SensorFrame^ rightFrame, Platform::Array<uint8_t>^ rightData) {

		JsonObject^ jsonObject = ref new JsonObject();
		

		dbg::trace(L"bitmap pixel format %i", leftFrame->SoftwareBitmap->BitmapPixelFormat);

		//op and topic
		jsonObject->Insert("op", JsonValue::CreateStringValue("publish"));
		jsonObject->Insert("topic", JsonValue::CreateStringValue(topic));

		JsonObject^ msg = ref new JsonObject();
			msg->Insert("greeting", JsonValue::CreateStringValue("Hello project."));
			msg->Insert("height" ,JsonValue::CreateNumberValue(leftFrame->SensorStreamingCameraIntrinsics->ImageHeight));
			msg->Insert("width", JsonValue::CreateNumberValue(leftFrame->SensorStreamingCameraIntrinsics->ImageWidth));

			//Image buffer data as base64 encoded string. 
			Platform::String^ leftString = StringFromstd(base64_encode(leftData->Data, leftData->Length));

				//CryptographicBuffer::EncodeToBase64String(CryptographicBuffer::CreateFromByteArray(leftData)); UWP JSON encrypting. Can be useful if data being received by another UWP app. 
			Platform::String^ rightString = StringFromstd(base64_encode(rightData->Data, rightData->Length));
				//CryptographicBuffer::EncodeToBase64String(CryptographicBuffer::CreateFromByteArray(rightData));

			msg->Insert("left64", JsonValue::CreateStringValue(leftString));
			msg->Insert("right64", JsonValue::CreateStringValue(rightString));

			auto leftCamToOrigin = GetCamToOrigin(leftFrame);
			auto rightCamToOrigin = GetCamToOrigin(rightFrame);
		
			Eigen::Vector3f leftcamPinhole(leftCamToOrigin.m41, leftCamToOrigin.m42, leftCamToOrigin.m43);
			Eigen::Vector3f rightcamPinhole(rightCamToOrigin.m41, rightCamToOrigin.m42, rightCamToOrigin.m43);

			Eigen::Matrix3f leftCamToOriginR;
			Eigen::Matrix3f rightCamToOriginR;
		

			leftCamToOriginR(0, 0) = leftCamToOrigin.m11;
			leftCamToOriginR(0, 1) = leftCamToOrigin.m12;
			leftCamToOriginR(0, 2) = leftCamToOrigin.m13;
			leftCamToOriginR(1, 0) = leftCamToOrigin.m21;
			leftCamToOriginR(1, 1) = leftCamToOrigin.m22;
			leftCamToOriginR(1, 2) = leftCamToOrigin.m23;
			leftCamToOriginR(2, 0) = leftCamToOrigin.m31;
			leftCamToOriginR(2, 1) = leftCamToOrigin.m32;
			leftCamToOriginR(2, 2) = leftCamToOrigin.m33;
			rightCamToOriginR(0, 0) = rightCamToOrigin.m11;
			rightCamToOriginR(0, 1) = rightCamToOrigin.m12;
			rightCamToOriginR(0, 2) = rightCamToOrigin.m13;
			rightCamToOriginR(1, 0) = rightCamToOrigin.m21;
			rightCamToOriginR(1, 1) = rightCamToOrigin.m22;
			rightCamToOriginR(1, 2) = rightCamToOrigin.m23;
			rightCamToOriginR(2, 0) = rightCamToOrigin.m31;
			rightCamToOriginR(2, 1) = rightCamToOrigin.m32;
			rightCamToOriginR(2, 2) = rightCamToOrigin.m33;

			Eigen::Quaternionf leftRotQuat(leftCamToOriginR);
			leftRotQuat.normalize();
			Eigen::Quaternionf rightRotQuat(rightCamToOriginR);
			rightRotQuat.normalize();


			JsonObject^ leftTransform = ref new JsonObject();
				JsonObject^ leftTranslation = ref new JsonObject();
				leftTranslation->Insert("x", JsonValue::CreateNumberValue(leftcamPinhole(0) ));
				leftTranslation->Insert("y", JsonValue::CreateNumberValue(leftcamPinhole(1)));
				leftTranslation->Insert("z", JsonValue::CreateNumberValue(leftcamPinhole(2)));
				leftTransform->Insert("translation", JsonValue::Parse( leftTranslation->Stringify() ));
				JsonObject^ leftRotation = ref new JsonObject();
				leftRotation->Insert("x", JsonValue::CreateNumberValue(leftRotQuat.x()));
				leftRotation->Insert("y", JsonValue::CreateNumberValue(leftRotQuat.y())); 
				leftRotation->Insert("z", JsonValue::CreateNumberValue(leftRotQuat.z())); 
				leftRotation->Insert("w", JsonValue::CreateNumberValue(leftRotQuat.w()));
				leftTransform->Insert("rotation", JsonValue::Parse(leftRotation->Stringify()));
			msg->Insert("leftCamToWorld", JsonValue::Parse(leftTransform->Stringify()));
		
			JsonObject^ rightTransform = ref new JsonObject();
				JsonObject^ rightTranslation = ref new JsonObject();
				rightTranslation->Insert("x", JsonValue::CreateNumberValue(rightcamPinhole(0)));
				rightTranslation->Insert("y", JsonValue::CreateNumberValue(rightcamPinhole(1)));
				rightTranslation->Insert("z", JsonValue::CreateNumberValue(rightcamPinhole(2)));
				rightTransform->Insert("translation", JsonValue::Parse(rightTranslation->Stringify()));
				JsonObject^ rightRotation = ref new JsonObject();
				rightRotation->Insert("x", JsonValue::CreateNumberValue(rightRotQuat.x()));
				rightRotation->Insert("y", JsonValue::CreateNumberValue(rightRotQuat.y())); 
				rightRotation->Insert("z", JsonValue::CreateNumberValue(rightRotQuat.z())); 
				rightRotation->Insert("w", JsonValue::CreateNumberValue(rightRotQuat.w()));
				rightTransform->Insert("rotation", JsonValue::Parse(rightRotation->Stringify()));
			msg->Insert("rightCamToWorld", JsonValue::Parse(rightTransform->Stringify()));
		
			//Final Json publish
		jsonObject->Insert("msg", JsonValue::Parse( msg->Stringify() ));
		Send(jsonObject->Stringify());
			
	}


	void BackgroundTask::PublishHello(Platform::String^ topic, Platform::String^ message) {

		JsonObject^ jsonObject = ref new JsonObject();
		jsonObject->Insert("op", JsonValue::CreateStringValue("publish"));
		jsonObject->Insert("topic", JsonValue::CreateStringValue(topic));

		JsonObject^ dataX = ref new JsonObject();
		dataX->Insert("data", JsonValue::CreateStringValue(message));
		

		jsonObject->Insert("msg", JsonValue::Parse(dataX->Stringify()));
		dbg::trace(L"Publish Test Pair # %i", pubCount);

		Send(jsonObject->Stringify());

	}

	/*
	void BackgroundTask::Publish(Platform::String^ topic, Platform::String^ message) {
		//std::string call = "\"op\":\"publish\", \"topic\":\"" + topic + "\", \"msg\":" + message + "\"";
		//call = "{" + call + "}";
		
		JsonObject^ jsonObject = ref new JsonObject();
		jsonObject->Insert("op", JsonValue::CreateStringValue("publish"));
		jsonObject->Insert("topic", JsonValue::CreateStringValue(topic));
		
		Platform::String^ msg = "{data:'" + message + "'}";
		
		bool dataAsString;

		dataAsString = false;

		Platform::String^ data;

		if (!dataAsString) {

			//JsonObject^ dataX = ref new JsonObject();
			//dataX->Insert("data", JsonValue::CreateStringValue(message));
			
			dbg::trace(L"Publish: Data   %s ,  %s", message->Data(),
				msg->Data()
				//data->Stringify()->Data()  
			);

			jsonObject->Insert("msg", JsonValue::CreateStringValue(
				msg
				//	dataX->Stringify()
			));

			//data = dataX->Stringify();
			data = msg;

		}
		else if (dataAsString) {
			
			data = "{\"data\":\"" + message + "\"}";
			jsonObject->Insert("msg", JsonValue::CreateStringValue(data->ToString()));
		
		}
		
		auto output = jsonObject->Stringify(); // Connected. Publish: Expected a JSON object of type std_msgs/string but received a <type 'unicode'>
		// (with data as string)  {"op":"publish","topic":"/listener","msg":"{\"data\":\"Hello from the HoloLens\"}"} || {"op":"publish","topic":"/listener","msg":"[\"data\":\"Hello from the HoloLens\"]"}
		// (with data as jsonobject) {"op":"publish","topic":"/listener","msg":"{\"data\":\"Hello from the HoloLens\"}"}

		auto outputx = "{\r\n" +
			"\'op\':\'publish\',\r\n" +
			"\'topic\':\'/listener\',\r\n" +
			"\'msg\':\'" + data + "\',\r\n" +
			"}"; ////Connected. Nothing Echoed. Nothing Received. {} 'op' error  || []  No Error Message.
		auto outputy = "{\r\n" +
			"\"op\":\"publish\",\r\n" +
			"\"topic\": \"/listener\",\r\n" +
			"\"msg\":\"" + data + "\",\r\n" +
			"}";//Connected. Nothing Echoed. Nothing Received. {} 'op' error  || []  No Error Message. Did not disconnect the client?
		auto outputz = "{" +
			"\"op\":\"publish\"," +
			"\"topic\":\"/listener\"," +
			"\"msg\":\"" + data + "\"" +
			"}";//Connected. Nothing Echoed. Nothing Received. {} 'op' error  || [] No Error Message. 
		auto outputzz = "{" +
			"\'op\':\'publish\'," +
			"\'topic\':\'/listener\'," +
			"\'msg\':\'" + data + "\'" +
			"}";	//Connected. Nothing Echoed. Nothing Received. No Error Message.

		auto outputzzz = "{op:'publish',topic:'/listener',msg:" + msg + "}";

		//stringify {"op":"publish","topic":"/listener","msg":"{  \"data \" : \" Hello from the HoloLens \"  }  "}
		//tostring {"op":"publish","topic":"/listener","msg":"{  \"data \" : \" Hello from the HoloLens \"  }  "}

		dbg::trace(L"Publish: output stringify %s, x %s, y %s , z %s, zz %s zzz %s", output->Data(), outputx->Data(), outputy->Data() , outputz->Data(), outputzz->Data(), outputzzz->Data());
		//dbg::trace(L"Publish: output tostring %s", jsonObject->ToString());
		Send(outputzzz);
		
		
		/*
		JsonObject^ jsonObject = ref new JsonObject();
		auto parsed = JsonObject::TryParse(call, &jsonObject);
		dbg::trace(L"Publish: TryParse(call, &jsonObject) %s", parsed.ToString());
		if (parsed) {

		}
		*/
		//dbg::trace(L"Publish: %s", );
		//Send(call);
	//}
	


	void BackgroundTask::PublishArray(Platform::String^ topic, Platform::Array<uint8_t>^ inputArray) {
		JsonObject^ jsonObject = ref new JsonObject();
		JsonObject^ data = ref new JsonObject();
		auto outputArray = inputArray->Value;
		String^ msg = ref new String();
		
		unsigned int i = 0;
		while (i < sizeof(outputArray)) {
			if (i != 0 && i != ( sizeof(outputArray)-1 )) {
				msg += ", ";
			}
			msg += outputArray[i];
			i++;
		}
		
		msg = "[ " + msg + " ]";
		data->Insert("data", JsonValue::Parse(msg) );

		jsonObject->Insert("op", JsonValue::CreateStringValue("publish"));
		jsonObject->Insert("topic", JsonValue::CreateStringValue(topic));
		jsonObject->Insert("msg", 
			
			JsonValue::CreateStringValue( data->Stringify() )  // ? ? ? ? ? ? ? ?
		);

		auto output = jsonObject->Stringify();

		dbg::trace(L"sending array");
		Send(output);
		

	}


	/*
	void BackgroundTask::MessageReceived(MessageWebSocket^ sender, MessageWebSocketMessageReceivedEventArgs^ args)
	{
		// Dispatch the event to the UI thread so we can update UI.
		//Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, args]()
		//{
		dbg::trace(L"Message Received; Type: %s", args->MessageType.ToString());
		DataReader^ reader = args->GetDataReader();
		reader->UnicodeEncoding = UnicodeEncoding::Utf8;
		try
		{

	///* REMOVE

		String^ read = reader->ReadString(reader->UnconsumedBufferLength);
		//AppendOutputLine(read);
		}
		catch (Exception^ ex)
		{
		//	AppendOutputLine(BuildWebSocketError(ex));
			//AppendOutputLine(ex->Message);
			dbg::trace(L"MessageReceived: Error.");
		}
		///*   REMOVE
		delete reader;
		//}));
	}
	*/

	void BackgroundTask::OnDisconnect() {

		SetBusy(true);
		CloseSocket();
		SetBusy(false);

	}

	void BackgroundTask::OnClosed(IWebSocket^ sender, WebSocketClosedEventArgs^ args)
	{
		if (messageWebSocket == sender)
		{
			CloseSocket();
		}	
	}

	void BackgroundTask::CloseSocket() {
		if (messageWriter != nullptr) {
			messageWriter->DetachStream();
			delete messageWriter;
			messageWriter = nullptr;

		}

		if (messageWebSocket != nullptr) {
			messageWebSocket->Close(1000, "Closed.");
			messageWebSocket = nullptr;
		}
	}

	std::string BackgroundTask::base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
		std::string ret;
		int i = 0;
		int j = 0;
		unsigned char char_array_3[3];
		unsigned char char_array_4[4];

		while (in_len--) {
			char_array_3[i++] = *(bytes_to_encode++);
			if (i == 3) {
				char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
				char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
				char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
				char_array_4[3] = char_array_3[2] & 0x3f;

				for (i = 0; (i < 4); i++)
					ret += base64_chars[char_array_4[i]];
				i = 0;
			}
		}

		if (i)
		{
			for (j = i; j < 3; j++)
				char_array_3[j] = '\0';

			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

			for (j = 0; (j < i + 1); j++)
				ret += base64_chars[char_array_4[j]];

			while ((i++ < 3))
				ret += '=';

		}

		return ret;

	}

	std::string BackgroundTask::base64_decode(std::string const& encoded_string) {
		size_t in_len = encoded_string.size();
		int i = 0;
		int j = 0;
		int in_ = 0;
		unsigned char char_array_4[4], char_array_3[3];
		std::string ret;

		while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
			char_array_4[i++] = encoded_string[in_]; in_++;
			if (i == 4) {
				for (i = 0; i < 4; i++)
					char_array_4[i] = base64_chars.find(char_array_4[i]) & 0xff;

				char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
				char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
				char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

				for (i = 0; (i < 3); i++)
					ret += char_array_3[i];
				i = 0;
			}
		}

		if (i) {
			for (j = 0; j < i; j++)
				char_array_4[j] = base64_chars.find(char_array_4[j]) & 0xff;

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

			for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
		}

		return ret;
	}

	String^ BackgroundTask::StringFromstd(std::string input)
	{
		std::wstring wid_str = std::wstring(input.begin(), input.end());
		const wchar_t* w_char = wid_str.c_str();
		Platform::String^ p_string = ref new Platform::String(w_char);
		return p_string;

	}
	String^ BackgroundTask::StringFromAscIIChars(char* chars)
	{
		size_t newsize = strlen(chars) + 1;
		wchar_t * wcstring = new wchar_t[newsize];
		size_t convertedChars = 0;
		mbstowcs_s(&convertedChars, wcstring, newsize, chars, _TRUNCATE);
		String^ str = ref new Platform::String(wcstring);
		delete[] wcstring;
		return str;
	}/*
	Platform::String^ BackgroundTask::base64_encodep(unsigned char const* bytes_to_encode, unsigned int in_len) {
		Platform::String^ ret;
		int i = 0;
		int j = 0;
		unsigned char char_array_3[3];
		unsigned char char_array_4[4];

		while (in_len--) {
			char_array_3[i++] = *(bytes_to_encode++);
			if (i == 3) {
				char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
				char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
				char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
				char_array_4[3] = char_array_3[2] & 0x3f;

				for (i = 0; (i < 4); i++)
					ret += &base64_charsp->Data.[char_array_4[i]];

				i = 0;
			}
		}

		if (i)
		{
			for (j = i; j < 3; j++)
				char_array_3[j] = '\0';

			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

			for (j = 0; (j < i + 1); j++)
				ret += &base64_charsp->Data[char_array_4[j]];

			while ((i++ < 3))
				ret += '=';

		}

		return ret;

	}

	Platform::String^ BackgroundTask::base64_decodep(std::string const& encoded_string) { // not needed here
		size_t in_len = encoded_string.size();
		int i = 0;
		int j = 0;
		int in_ = 0;
		unsigned char char_array_4[4], char_array_3[3];
		Platform::String^ ret;

		while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
			char_array_4[i++] = encoded_string[in_]; in_++;
			if (i == 4) {
				for (i = 0; i < 4; i++)
					char_array_4[i] = base64_chars.find(char_array_4[i]) & 0xff;

				char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
				char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
				char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

				for (i = 0; (i < 3); i++)
					ret += char_array_3[i];
				i = 0;
			}
		}

		if (i) {
			for (j = 0; j < i; j++)
				char_array_4[j] = base64_charsp->Data.find(char_array_4[j]) & 0xff;

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

			for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
		}

		return ret;
	}*/
}