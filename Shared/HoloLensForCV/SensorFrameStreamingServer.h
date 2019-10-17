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
    public ref class SensorFrameStreamingServer sealed
        : public ISensorFrameSink
    {
    public:
        SensorFrameStreamingServer(
            _In_ Platform::String^ serviceName);

        virtual void Send(
            SensorFrame^ sensorFrame);

    private:
        ~SensorFrameStreamingServer();

        void OnConnection(
            Windows::Networking::Sockets::StreamSocketListener^ listener,
            Windows::Networking::Sockets::StreamSocketListenerConnectionReceivedEventArgs^ object);

        void SendImage(
            SensorFrameStreamHeader^ header,
            const Platform::Array<uint8_t>^ data);

    private:
        Windows::Networking::Sockets::StreamSocketListener^ _listener;
        Windows::Networking::Sockets::StreamSocket^ _socket;
        Windows::Storage::Streams::DataWriter^ _writer;
        bool _writeInProgress;
    };
}
