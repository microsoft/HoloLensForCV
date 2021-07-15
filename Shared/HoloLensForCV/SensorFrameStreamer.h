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
    // Collects sensor frames for all the enabled sensors. Opens a stream socket for each
    // of the sensors and streams the sensor images to connected clients.
    //
    public ref class SensorFrameStreamer sealed
        : public ISensorFrameSinkGroup
    {
    public:
        SensorFrameStreamer();

        void EnableAll();

        void Enable(
            _In_ SensorType sensorType);

        virtual ISensorFrameSink^ GetSensorFrameSink(
            _In_ SensorType sensorType);

    private:
        std::array<SensorFrameStreamingServer^, (size_t)SensorType::NumberOfSensorTypes> _sensorFrameStreamingServers;
    };
}
