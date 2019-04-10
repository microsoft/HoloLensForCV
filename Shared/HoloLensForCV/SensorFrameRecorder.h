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
    // Collects sensor frames for all the enabled sensors. Uses the recorder sink to save
    // the individual sensor frames to disk. Once recording is stopped, collects all the
    // images and meta-data and combines them into a single tarball.
    //
    // Refer to 'Samples\BatchProcessing' for an example use of the recorded information.
    //
    public ref class SensorFrameRecorder sealed
        : public ISensorFrameSinkGroup
    {
    public:
        SensorFrameRecorder();

        static property uint8_t RecordingVersionMajor
        {
            uint8_t get() { return 0x00; }
        }

        static property uint8_t RecordingVersionMinor
        {
            uint8_t get() { return 0x01; }
        }

        void EnableAll();

        void Enable(
            _In_ SensorType sensorType);

        Windows::Foundation::IAsyncAction^ StartAsync();

        void Stop();

        virtual ISensorFrameSink^ GetSensorFrameSink(
            _In_ SensorType sensorType);

    private:
        ~SensorFrameRecorder();

        const wchar_t* GetSensorName(
            SensorType sensorType);

        void ReportRecorderVersioningInformation(
            _Inout_ std::vector<std::wstring>& sourceFiles);

        void ReportCameraCalibrationInformation(
            _Inout_ std::vector<std::wstring>& sourceFiles);

    private:
        std::mutex _recorderMutex;

        Windows::Storage::StorageFolder^ _archiveSourceFolder;

        std::array<SensorFrameRecorderSink^, (size_t)SensorType::NumberOfSensorTypes> _sensorFrameSinks;
    };
}
