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
		std::lock_guard<std::mutex> guard(_sinkMutex);

		// Remember the root folder for the recorded sensor meta-data.
		REQUIRES(nullptr == _archiveSourceFolder);
		_archiveSourceFolder = archiveSourceFolder;

		// Create the tarball for the bitmap files.
		
		{
			wchar_t fileName[MAX_PATH] = {};
			swprintf_s(
				fileName,
				L"%s\\%s.tar",
				_archiveSourceFolder->Path->Data(),
				_sensorName->Data());
			_bitmapTarball.reset(new Io::Tarball(fileName));
		}
		

		// Create the csv file for the frame information.

		{
			wchar_t fileName[MAX_PATH] = {};
			swprintf_s(
				fileName,
				L"%s\\%s.csv",
				_archiveSourceFolder->Path->Data(),
				_sensorName->Data());
			_csvWriter.reset(new CsvWriter(fileName));
		}

		// Write header information to csv file.

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
	}

	void SensorFrameRecorderSink::Stop()
	{
		std::lock_guard<std::mutex> guard(_sinkMutex);
		_bitmapTarball.reset();
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

		if (nullptr == _archiveSourceFolder)
		{
			return;
		}

		// Store a reference to the camera intrinsics.
		if (nullptr == _cameraIntrinsics)
		{
			_cameraIntrinsics = sensorFrame->SensorStreamingCameraIntrinsics;
		}

		// Avoid duplicate sensor frame recordings.
		if (_prevFrameTimestamp.Equals(sensorFrame->Timestamp)) {
			return;
		}

		_prevFrameTimestamp = sensorFrame->Timestamp;

		//
		// Write the sensor frame as a bitmap to the archive.
		//

		wchar_t bitmapPath[MAX_PATH] = {};
		swprintf_s(
			bitmapPath, L"%s\\%020llu.pgm",
			_sensorName->Data(), sensorFrame->Timestamp.UniversalTime);

#if DBG_ENABLE_VERBOSE_LOGGING
		dbg::trace(
			L"SensorFrameRecorderSink::Send: saving sensor frame to %s",
			bitmapPath);
#endif /* DBG_ENABLE_VERBOSE_LOGGING */

		Windows::Graphics::Imaging::SoftwareBitmap^ softwareBitmap =
			sensorFrame->SoftwareBitmap;

		{
			// Determine metadata information about frame.

			int maxBitmapValue = 0;
			int actualBitmapWidth = softwareBitmap->PixelWidth;

			switch (softwareBitmap->BitmapPixelFormat)
			{

			case Windows::Graphics::Imaging::BitmapPixelFormat::Gray16:
				maxBitmapValue = 65535;
				break;

			case Windows::Graphics::Imaging::BitmapPixelFormat::Gray8:
				maxBitmapValue = 255;
				break;

			case Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8:
				if ((_sensorType == SensorType::VisibleLightLeftFront) ||
					(_sensorType == SensorType::VisibleLightLeftLeft) ||
					(_sensorType == SensorType::VisibleLightRightFront) ||
					(_sensorType == SensorType::VisibleLightRightRight))
				{
					maxBitmapValue = 255;
					actualBitmapWidth = actualBitmapWidth * 4;
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

			// Compose PGM header string.
			std::stringstream header;
			header << "P5\n"
				<< actualBitmapWidth << " "
				<< softwareBitmap->PixelHeight << "\n"
				<< maxBitmapValue << "\n";
			const std::string headerString = header.str();

			// Get bitmap buffer object of the frame.
			Windows::Graphics::Imaging::BitmapBuffer^ bitmapBuffer =
				softwareBitmap->LockBuffer(
					Windows::Graphics::Imaging::BitmapBufferAccessMode::Read);

			// Get raw pointer to the buffer object.
			uint32_t pixelBufferDataLength = 0;
			const uint8_t* pixelBufferData =
				Io::GetTypedPointerToMemoryBuffer<uint8_t>(
					bitmapBuffer->CreateReference(),
					pixelBufferDataLength);

			// Allocate data for PGM bitmap file.
			std::vector<uint8_t> bitmapData;
			bitmapData.reserve(headerString.size() + pixelBufferDataLength);

			// Add PGM header data.
			bitmapData.insert(
				bitmapData.end(),
				headerString.c_str(), headerString.c_str() + headerString.size());

			// Add raw pixel data.
			bitmapData.insert(
				bitmapData.end(),
				pixelBufferData, pixelBufferData + pixelBufferDataLength);

			// Add the bitmap to the tarball.
			_bitmapTarball->AddFile(bitmapPath, bitmapData.data(), bitmapData.size());
		}

		//
		// Record the sensor frame meta data to the csv file.
		//

		bool writeComma = false;

		_csvWriter->WriteUInt64(
			sensorFrame->Timestamp.UniversalTime, &writeComma);

		{
			_csvWriter->WriteText(
				bitmapPath, &writeComma);
		}

		_csvWriter->WriteFloat4x4(
			sensorFrame->FrameToOrigin, &writeComma);

		_csvWriter->WriteFloat4x4(
			sensorFrame->CameraViewTransform, &writeComma);

		_csvWriter->WriteFloat4x4(
			sensorFrame->CameraProjectionTransform, &writeComma);

		_csvWriter->EndLine();
	}
}
