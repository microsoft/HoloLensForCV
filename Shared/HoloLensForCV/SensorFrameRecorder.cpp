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
    SensorFrameRecorder::SensorFrameRecorder()
    {
    }

    SensorFrameRecorder::~SensorFrameRecorder()
    {
        Stop();
    }

    void SensorFrameRecorder::EnableAll()
    {
        Enable(SensorType::PhotoVideo);

#if ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS
        Enable(SensorType::ShortThrowToFDepth);
        Enable(SensorType::ShortThrowToFReflectivity);
        Enable(SensorType::LongThrowToFDepth);
        Enable(SensorType::LongThrowToFReflectivity);
        Enable(SensorType::VisibleLightLeftLeft);
        Enable(SensorType::VisibleLightLeftFront);
        Enable(SensorType::VisibleLightRightFront);
        Enable(SensorType::VisibleLightRightRight);
#endif /* ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS */
    }

    void SensorFrameRecorder::Enable(
        _In_ SensorType sensorType)
    {
        std::lock_guard<std::mutex> recorderLockGuard(
            _recorderMutex);

        const int32_t sensorTypeAsIndex =
            (int32_t)sensorType;

        REQUIRES(
            0 <= sensorTypeAsIndex &&
            sensorTypeAsIndex < (int32_t)_sensorFrameSinks.size());

        SensorFrameRecorderSink^ sensorFrameSink =
            ref new SensorFrameRecorderSink(
                sensorType,
                ref new Platform::String(
                    GetSensorName(
                        sensorType)));

        _sensorFrameSinks[sensorTypeAsIndex] =
            sensorFrameSink;
    }

    Windows::Foundation::IAsyncAction^ SensorFrameRecorder::StartAsync()
    {
        return concurrency::create_async(
            [this]()
        {
            Windows::Storage::StorageFolder^ temporaryStorageFolder =
                Windows::Storage::ApplicationData::Current->TemporaryFolder;

            return concurrency::create_task(temporaryStorageFolder->CreateFolderAsync(
                L"archiveSource",
                Windows::Storage::CreationCollisionOption::ReplaceExisting)).then(
                    [&](Windows::Storage::StorageFolder^ archiveSourceFolder)
                {
                    std::lock_guard<std::mutex> recorderLockGuard(
                        _recorderMutex);

                    _archiveSourceFolder = archiveSourceFolder;

                    for (SensorFrameRecorderSink^ sensorFrameSink : _sensorFrameSinks)
                    {
                        if (nullptr == sensorFrameSink)
                        {
                            continue;
                        }

                        sensorFrameSink->Start(_archiveSourceFolder);
                    }
                });
        });
    }

    void SensorFrameRecorder::Stop()
    {
        std::lock_guard<std::mutex> recorderLockGuard(
            _recorderMutex);

        //
        // Build a list of files to archive.
        //
        std::vector<std::wstring> sourceFiles;

		for (SensorFrameRecorderSink^ sensorFrameSink : _sensorFrameSinks)
		{
			if (nullptr == sensorFrameSink)
			{
				continue;
			}

			sensorFrameSink->Stop();

		}

		for (SensorFrameRecorderSink^ sensorFrameSink : _sensorFrameSinks)
		{
			if (nullptr == sensorFrameSink)
			{
				continue;
			}

            sensorFrameSink->ReportArchiveSourceFiles(sourceFiles);
        }

        //
        // Add recording version information.
        //
        ReportRecorderVersioningInformation(sourceFiles);

        //
        // Add a description of camera intrinsics.
        //
        ReportCameraCalibrationInformation(sourceFiles);

        //
        // Create a TAR file containing all the recording files reported so far.
        //
        {
            const auto timeNow =
                _time64(
                    nullptr /* timer */);

            std::tm timeNowUtc;

            ASSERT(0 == _gmtime64_s(
                &timeNowUtc,
                &timeNow));

            wchar_t archiveName[MAX_PATH] = {};

            swprintf_s(
                archiveName,
                L"HoloLensRecording__%04i_%02i_%02i__%02i_%02i_%02i",
                timeNowUtc.tm_year + 1900,
                timeNowUtc.tm_mon + 1,
                timeNowUtc.tm_mday,
                timeNowUtc.tm_hour,
                timeNowUtc.tm_min,
                timeNowUtc.tm_sec);

            Platform::String^ archiveNameString =
                ref new Platform::String(archiveName);
            _archiveSourceFolder->RenameAsync(archiveNameString);
        }

        _archiveSourceFolder = nullptr;
    }

    void SensorFrameRecorder::ReportRecorderVersioningInformation(
        _Inout_ std::vector<std::wstring>& sourceFiles)
    {
        wchar_t fileName[MAX_PATH] = {};

        swprintf_s(
            fileName,
            L"%s\\recording_version_information.csv",
            _archiveSourceFolder->Path->Data());

        sourceFiles.push_back(
            L"recording_version_information.csv");

        CsvWriter csvWriter(
            fileName);

        {
            std::vector<std::wstring> columns;

            columns.push_back(L"VersionMajor");
            columns.push_back(L"VersionMinor");

            csvWriter.WriteHeader(
                columns);
        }

        bool writeComma = false;

        csvWriter.WriteInt32(
            RecordingVersionMajor,
            &writeComma);

        csvWriter.WriteInt32(
            RecordingVersionMinor,
            &writeComma);

        csvWriter.EndLine();
    }

    void SensorFrameRecorder::ReportCameraCalibrationInformation(
        _Inout_ std::vector<std::wstring>& sourceFiles)
    {
        // TODO: Support PV calibration.

        for (SensorFrameRecorderSink^ sensorFrameSink : _sensorFrameSinks)
        {
            if (nullptr == sensorFrameSink)
            {
                continue;
            }

            auto cameraIntrinsics =
                sensorFrameSink->GetCameraIntrinsics();

            if (nullptr == cameraIntrinsics)
            {
                continue;
            }

            wchar_t fileName[MAX_PATH] = {};

            swprintf_s(
                fileName,
                L"%s\\%s_camera_space_projection.bin",
                _archiveSourceFolder->Path->Data(),
                sensorFrameSink->GetSensorName()->Data());

            sourceFiles.push_back(fileName);

            std::vector<Windows::Foundation::Point> pointList;
            pointList.resize(cameraIntrinsics->ImageWidth * cameraIntrinsics->ImageHeight);
            size_t index = 0;
            for (unsigned int x = 0; x < cameraIntrinsics->ImageWidth; ++x)
            {
                for (unsigned int y = 0; y < cameraIntrinsics->ImageHeight; ++y)
                {
                    Windows::Foundation::Point uv = { float(x), float(y) }, xy;
                    cameraIntrinsics->MapImagePointToCameraUnitPlane(uv, &xy);
                    pointList[index++] = xy;
                }
            }

            //TODO: Better conversion to char*
            std::wstring ws(fileName);
            std::string outputFilePath;
            outputFilePath.assign(ws.begin(), ws.end());

            FILE* file = nullptr;
            ASSERT(0 == fopen_s(&file, outputFilePath.c_str(), "wb"));

            size_t expectedSize = pointList.size() * sizeof(Windows::Foundation::Point);
            ASSERT(expectedSize == fwrite(
                reinterpret_cast<uint8_t*>(&pointList[0]),
                sizeof(uint8_t),
                expectedSize,
                file));

            ASSERT(0 == fclose(file));

        }
    }

    ISensorFrameSink^ SensorFrameRecorder::GetSensorFrameSink(
        _In_ SensorType sensorType)
    {
        ISensorFrameSink^ sensorFrameSink;

        {
            std::lock_guard<std::mutex> recorderLockGuard(
                _recorderMutex);

            const int32_t sensorTypeAsIndex =
                (int32_t)sensorType;

            REQUIRES(
                0 <= sensorTypeAsIndex &&
                sensorTypeAsIndex < (int32_t)_sensorFrameSinks.size());

            sensorFrameSink = _sensorFrameSinks[
                sensorTypeAsIndex];
        }

        return sensorFrameSink;
    }

    const wchar_t* SensorFrameRecorder::GetSensorName(
        SensorType sensorType)
    {
        switch (sensorType)
        {
        case SensorType::PhotoVideo:
            return L"pv";

#if ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS
        case SensorType::ShortThrowToFDepth:
            return L"short_throw_depth";

        case SensorType::ShortThrowToFReflectivity:
            return L"short_throw_reflectivity";

        case SensorType::LongThrowToFDepth:
            return L"long_throw_depth";

        case SensorType::LongThrowToFReflectivity:
            return L"long_throw_reflectivity";

        case SensorType::VisibleLightLeftLeft:
            return L"vlc_ll";

        case SensorType::VisibleLightLeftFront:
            return L"vlc_lf";

        case SensorType::VisibleLightRightFront:
            return L"vlc_rf";

        case SensorType::VisibleLightRightRight:
            return L"vlc_rr";
#endif /* ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS */

        default:
            throw std::logic_error("unexpected sensor type");
        }
    }
}
