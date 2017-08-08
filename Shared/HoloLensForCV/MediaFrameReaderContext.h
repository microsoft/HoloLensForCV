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
    // Receives media frames from the MediaFrameReader
    // and exposes them as sensor frames to the app.
    //
    public ref class MediaFrameReaderContext sealed
    {
    public:
        MediaFrameReaderContext(
            _In_ SensorType sensorType,
            _In_ SpatialPerception^ spatialPerception,
            _In_opt_ ISensorFrameSink^ sensorFrameSink);

        SensorFrame^ GetLatestSensorFrame();

        /// <summary>
        /// Handler for frames which arrive from the MediaFrameReader.
        /// </summary>
        void FrameArrived(
            Windows::Media::Capture::Frames::MediaFrameReader^ sender,
            Windows::Media::Capture::Frames::MediaFrameArrivedEventArgs^ args);

    private:
        SensorType _sensorType;
        SpatialPerception^ _spatialPerception;
        ISensorFrameSink^ _sensorFrameSink;

        Io::TimeConverter _timeConverter;

        std::mutex _latestSensorFrameMutex;
        SensorFrame^ _latestSensorFrame;
    };
}
