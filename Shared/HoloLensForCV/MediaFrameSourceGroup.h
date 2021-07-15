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
    // Handles media capture and exposing sensor frames for the specified sensor group.
    //
    public ref class MediaFrameSourceGroup sealed
    {
    public:
        MediaFrameSourceGroup(
            _In_ MediaFrameSourceGroupType mediaFrameSourceGroupType,
            _In_ SpatialPerception^ spacialPerception,
            _In_opt_ ISensorFrameSinkGroup^ optionalSensorFrameSinkGroup);

        void EnableAll();

        void Enable(
            _In_ SensorType sensorType);

        Windows::Foundation::IAsyncAction^ StartAsync();

		    Windows::Foundation::IAsyncAction^ StopAsync();

        SensorFrame^ GetLatestSensorFrame(
            SensorType sensorType);

		Windows::Media::Devices::Core::CameraIntrinsics^ GetCameraIntrinsics(SensorType sensorType);


    private:
        /// <summary>
        /// Returns true if the sensor was explicitly enabled by the user.
        /// </summary>
        bool IsEnabled(
            _In_ SensorType sensorType) const;

        /// <summary>
        /// Switch to the next eligible media source.
        /// </summary>
        Concurrency::task<void> InitializeMediaSourceWorkerAsync();

        /// <summary>
        /// </summary>
        SensorType GetSensorType(
            Windows::Media::Capture::Frames::MediaFrameSource^ source);

        /// <summary>
        /// </summary>
        Platform::String^ GetSubtypeForFrameReader(
            Windows::Media::Capture::Frames::MediaFrameSourceKind kind,
            Windows::Media::Capture::Frames::MediaFrameFormat^ format);

        /// <summary>
        /// Stop streaming from all readers and dispose all readers and media capture object.
        /// </summary>
        /// <remarks>
        /// Unregisters FrameArrived event handlers, stops and disposes frame readers
        /// and disposes the MediaCapture object.
        /// </remarks>
        concurrency::task<void> CleanupMediaCaptureAsync();

        /// <summary>
        /// Initialize the media capture object.
        /// Must be called from the UI thread.
        /// </summary>
        concurrency::task<bool> TryInitializeMediaCaptureAsync(
            Windows::Media::Capture::Frames::MediaFrameSourceGroup^ group);



    private:
        MediaFrameSourceGroupType _mediaFrameSourceGroupType;
        SpatialPerception^ _spatialPerception;

        Platform::Agile<Windows::Media::Capture::MediaCapture> _mediaCapture;

        std::vector<std::pair<Windows::Media::Capture::Frames::MediaFrameReader^, Windows::Foundation::EventRegistrationToken>> _frameEventRegistrations;

        std::array<bool, (size_t)SensorType::NumberOfSensorTypes> _enabledFrameReaders;
        std::array<MediaFrameReaderContext^, (size_t)SensorType::NumberOfSensorTypes> _frameReaders;

        ISensorFrameSinkGroup^ _optionalSensorFrameSinkGroup;
    };
}
