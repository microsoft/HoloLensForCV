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
	SensorFrameRecorderSink::SensorFrameRecorderSink(
		_In_ SensorType sensorType,
		_In_ Platform::String^ sensorName)
		: _sensorType(sensorType), _sensorName(sensorName)
	{
	}

	SensorFrameRecorderSink::~SensorFrameRecorderSink()
	{
		Stop();
	}

	void SensorFrameRecorderSink::Start(
		_In_ Windows::Storage::StorageFolder^ archiveSourceFolder)
	{

		// Remember the root folder for the recorded sensor meta-data.

		{
			std::lock_guard<std::mutex> guard(_sinkMutex);
			REQUIRES(nullptr == _archiveSourceFolder);
			_archiveSourceFolder = archiveSourceFolder;
		}

		// Create a sub-folder for the recorder sensor stream data.

		concurrency::create_task(archiveSourceFolder->CreateFolderAsync(
			_sensorName, Windows::Storage::CreationCollisionOption::ReplaceExisting)
		).then([&](Windows::Storage::StorageFolder^ dataArchiveSourceFolder) {
			std::lock_guard<std::mutex> guard(_sinkMutex);
			REQUIRES(nullptr == _dataArchiveSourceFolder);
			_dataArchiveSourceFolder = dataArchiveSourceFolder;

			wchar_t fileName[MAX_PATH] = {};

			swprintf_s(
				fileName,
				L"%s\\%s.csv",
				_archiveSourceFolder->Path->Data(),
				_sensorName->Data());

			_csvWriter.reset(new CsvWriter(fileName));

			{
				std::vector<std::wstring> columns;

				columns.push_back(L"Timestamp");
				columns.push_back(L"ImageFileName");

				columns.push_back(L"FrameToOrigin.m11"); columns.push_back(L"FrameToOrigin.m12"); columns.push_back(L"FrameToOrigin.m13"); columns.push_back(L"FrameToOrigin.m14");
				columns.push_back(L"FrameToOrigin.m21"); columns.push_back(L"FrameToOrigin.m22"); columns.push_back(L"FrameToOrigin.m23"); columns.push_back(L"FrameToOrigin.m24");
				columns.push_back(L"FrameToOrigin.m31"); columns.push_back(L"FrameToOrigin.m32"); columns.push_back(L"FrameToOrigin.m33"); columns.push_back(L"FrameToOrigin.m34");
				columns.push_back(L"FrameToOrigin.m41"); columns.push_back(L"FrameToOrigin.m42"); columns.push_back(L"FrameToOrigin.m43"); columns.push_back(L"FrameToOrigin.m44");

				columns.push_back(L"CameraViewTransform.m11"); columns.push_back(L"CameraViewTransform.m12"); columns.push_back(L"CameraViewTransform.m13"); columns.push_back(L"CameraViewTransform.m14");
				columns.push_back(L"CameraViewTransform.m21"); columns.push_back(L"CameraViewTransform.m22"); columns.push_back(L"CameraViewTransform.m23"); columns.push_back(L"CameraViewTransform.m24");
				columns.push_back(L"CameraViewTransform.m31"); columns.push_back(L"CameraViewTransform.m32"); columns.push_back(L"CameraViewTransform.m33"); columns.push_back(L"CameraViewTransform.m34");
				columns.push_back(L"CameraViewTransform.m41"); columns.push_back(L"CameraViewTransform.m42"); columns.push_back(L"CameraViewTransform.m43"); columns.push_back(L"CameraViewTransform.m44");

				columns.push_back(L"CameraProjectionTransform.m11"); columns.push_back(L"CameraProjectionTransform.m12"); columns.push_back(L"CameraProjectionTransform.m13"); columns.push_back(L"CameraProjectionTransform.m14");
				columns.push_back(L"CameraProjectionTransform.m21"); columns.push_back(L"CameraProjectionTransform.m22"); columns.push_back(L"CameraProjectionTransform.m23"); columns.push_back(L"CameraProjectionTransform.m24");
				columns.push_back(L"CameraProjectionTransform.m31"); columns.push_back(L"CameraProjectionTransform.m32"); columns.push_back(L"CameraProjectionTransform.m33"); columns.push_back(L"CameraProjectionTransform.m34");
				columns.push_back(L"CameraProjectionTransform.m41"); columns.push_back(L"CameraProjectionTransform.m42"); columns.push_back(L"CameraProjectionTransform.m43"); columns.push_back(L"CameraProjectionTransform.m44");

				_csvWriter->WriteHeader(columns);
			}

		});
	}

	void SensorFrameRecorderSink::Stop()
	{
		std::lock_guard<std::mutex> guard(_sinkMutex);
		_csvWriter.reset();
		_archiveSourceFolder = nullptr;
	}

	Platform::String^ SensorFrameRecorderSink::GetSensorName()
	{
		return _sensorName;
	}

	CameraIntrinsics^ SensorFrameRecorderSink::GetCameraIntrinsics()
	{
		return _cameraIntrinsics;
	}

	void SensorFrameRecorderSink::ReportArchiveSourceFiles(
		_Inout_ std::vector<std::wstring>& sourceFiles)
	{
		wchar_t csvFileName[MAX_PATH] = {};

		swprintf_s(
			csvFileName,
			L"%s.csv",
			_sensorName->Data());

		sourceFiles.push_back(csvFileName);
	}

	void SensorFrameRecorderSink::Send(
		SensorFrame^ sensorFrame)
	{
		dbg::TimerGuard timerGuard(
			L"SensorFrameRecorderSink::Send: synchrounous I/O",
			20.0 /* minimum_time_elapsed_in_milliseconds */);

		std::lock_guard<std::mutex> lockGuard(_sinkMutex);

		if (nullptr == _archiveSourceFolder || nullptr == _dataArchiveSourceFolder)
		{
			return;
		}

		//
		// Store a reference to the camera intrinsics.
		//
		if (nullptr == _cameraIntrinsics)
		{
			_cameraIntrinsics =
				sensorFrame->SensorStreamingCameraIntrinsics;
		}

		//
		// Save the uncompressed image to disk. This is faster
		// than trying to compress images on the fly.
		//
		char absolutePath[MAX_PATH] = {};

		sprintf_s(
			absolutePath,
			"%S\\%020llu_%S.pgm",
			_dataArchiveSourceFolder->Path->Data(),
			sensorFrame->Timestamp.UniversalTime,
			_sensorName->Data());

#if DBG_ENABLE_VERBOSE_LOGGING
		dbg::trace(
			L"SensorFrameRecorderSink::Send: saving sensor frame to %S",
			absolutePath);
#endif /* DBG_ENABLE_VERBOSE_LOGGING */

		Windows::Graphics::Imaging::SoftwareBitmap^ softwareBitmap =
			sensorFrame->SoftwareBitmap;

		{
			FILE* file = nullptr;

			ASSERT(0 == fopen_s(&file, absolutePath, "wb"));

			int maxValue = 0;
			int actualPixelWidth = softwareBitmap->PixelWidth;
			switch (softwareBitmap->BitmapPixelFormat)
			{

			case Windows::Graphics::Imaging::BitmapPixelFormat::Gray16:
				maxValue = 65535;
				break;

			case Windows::Graphics::Imaging::BitmapPixelFormat::Gray8:
				maxValue = 255;
				break;

			case Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8:
				if ((_sensorType == SensorType::VisibleLightLeftFront) ||
					(_sensorType == SensorType::VisibleLightLeftLeft) ||
					(_sensorType == SensorType::VisibleLightRightFront) ||
					(_sensorType == SensorType::VisibleLightRightRight))
				{
					maxValue = 255;
					actualPixelWidth = actualPixelWidth * 4;
				}
				else
				{
					ASSERT(false);
				}

				break;

			default:
				// Unsupported by PGM format. Need to update save logic
#if DBG_ENABLE_INFORMATIONAL_LOGGING
				dbg::trace(
					L"SensorFrameRecorderSink::Send: unsupported bitmap pixel format for PGM");
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

				ASSERT(false);
				break;
			}

			// Write Header
			std::stringstream header;
			header << "P5\n"
				<< actualPixelWidth << " "
				<< softwareBitmap->PixelHeight << "\n"
				<< maxValue << "\n";

			std::string headerString = header.str();
			ASSERT(headerString.size() == fwrite(
				headerString.c_str(),
				sizeof(uint8_t) /* _ElementSize */,
				headerString.size() /* _ElementCount */,
				file));

			Windows::Graphics::Imaging::BitmapBuffer^ bitmapBuffer =
				softwareBitmap->LockBuffer(
					Windows::Graphics::Imaging::BitmapBufferAccessMode::Read);

			uint32_t pixelBufferDataLength = 0;

			uint8_t* pixelBufferData =
				Io::GetTypedPointerToMemoryBuffer<uint8_t>(
					bitmapBuffer->CreateReference(),
					pixelBufferDataLength);

			ASSERT(pixelBufferDataLength == fwrite(
				pixelBufferData,
				sizeof(uint8_t) /* _ElementSize */,
				pixelBufferDataLength /* _ElementCount */,
				file));

			ASSERT(0 == fclose(
				file));
		}

		bool writeComma = false;

		_csvWriter->WriteUInt64(sensorFrame->Timestamp.UniversalTime, &writeComma);

		{
			wchar_t relativePath[MAX_PATH] = {};

			swprintf_s(
				relativePath,
				L"%020llu_%s.raw",
				sensorFrame->Timestamp.UniversalTime,
				_sensorName->Data());

			_csvWriter->WriteText(relativePath, &writeComma);
		}

		_csvWriter->WriteFloat4x4(sensorFrame->FrameToOrigin, &writeComma);

		_csvWriter->WriteFloat4x4(sensorFrame->CameraViewTransform, &writeComma);

		_csvWriter->WriteFloat4x4(
      sensorFrame->CameraProjectionTransform, &writeComma);

		_csvWriter->EndLine();
	}
}
