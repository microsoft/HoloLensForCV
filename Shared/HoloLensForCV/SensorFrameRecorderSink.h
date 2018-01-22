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
    // Subset of sensor frame metadata relevant to the recording process.
    //
    struct SensorFrameRecorderLogEntry
    {
        Windows::Foundation::DateTime Timestamp;
        Windows::Foundation::Numerics::float4x4 FrameToOrigin;
        Windows::Foundation::Numerics::float4x4 CameraViewTransform;
        std::wstring RelativeImagePath;
    };

    //
    // Saves sensor images originated on device to disk and collects sensor frame
    // metadata that will be used to create the per-sensor recording manifest CSV
    // file.
    //
    public ref class SensorFrameRecorderSink sealed
        : public ISensorFrameSink
    {
    public:
        SensorFrameRecorderSink(
            _In_ SensorType sensorType,
            _In_ Platform::String^ sensorName);

        void Start(
            _In_ Windows::Storage::StorageFolder^ archiveSourceFolder);

        void Stop();

        virtual void Send(
            SensorFrame^ sensorFrame);

    internal:
        Platform::String^ GetSensorName();

        CameraIntrinsics^ GetCameraIntrinsics();

        void ReportArchiveSourceFiles(
            _Inout_ std::vector<std::wstring>& sourceFiles);

    private:
        ~SensorFrameRecorderSink();

    private:
        Platform::String^ _sensorName;

        SensorType _sensorType;

        std::mutex _sinkMutex;

        Windows::Storage::StorageFolder^ _archiveSourceFolder;
        Windows::Storage::StorageFolder^ _dataArchiveSourceFolder;

        std::vector<SensorFrameRecorderLogEntry> _recorderLog;

        CameraIntrinsics^ _cameraIntrinsics;
    };
}
