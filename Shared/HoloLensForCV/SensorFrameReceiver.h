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
    // On the device side, the sensor frame streamer will open a stream socket for each
    // of the sensors.
    //
    // On the client side, connect to that socket and use this class to await on the
    // ReceiveAsync call to obtain sensor frames.
    //
    public ref class SensorFrameReceiver sealed
    {
    public:
        SensorFrameReceiver(
            _In_ Windows::Networking::Sockets::StreamSocket^ streamSocket);

        Windows::Foundation::IAsyncOperation<SensorFrame^>^ ReceiveAsync();

    private:
        Concurrency::task<SensorFrameStreamHeader^> ReceiveSensorFrameStreamHeaderAsync();

        Concurrency::task<SensorFrame^> ReceiveSensorFrameAsync(
            SensorFrameStreamHeader^ header);

    private:
        Windows::Networking::Sockets::StreamSocket^ _streamSocket;
        Windows::Storage::Streams::DataReader^ _reader;
    };
}
