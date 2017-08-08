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
    // Network header for sensor frame streaming.
    //
    public ref class SensorFrameStreamHeader sealed
    {
    public:
        SensorFrameStreamHeader();

        static property uint32_t ProtocolHeaderLength
        {
            uint32_t get()
            {
                return
                    sizeof(uint32_t) /* Cookie */ +
                    2 * sizeof(uint8_t) /* VersionMajor, VersionMinor */ +
                    sizeof(uint16_t) /* FrameType */ +
                    sizeof(uint64_t) /* Timestamp */ +
                    4 * sizeof(uint32_t) /* ImageWidth, ImageHeight, PixelStride, RowStride */;
            }
        }

        static property uint32_t ProtocolCookie
        {
            uint32_t get() { return 0x484c524d; }
        }

        static property uint8_t ProtocolVersionMajor
        {
            uint8_t get() { return 0x00; }
        }

        static property uint8_t ProtocolVersionMinor
        {
            uint8_t get() { return 0x01; }
        }

        property uint32_t Cookie;
        property uint8_t VersionMajor;
        property uint8_t VersionMinor;
        property SensorType FrameType;
        property uint64_t Timestamp;
        property uint32_t ImageWidth;
        property uint32_t ImageHeight;
        property uint32_t PixelStride;
        property uint32_t RowStride;

        static void Read(
            _Inout_ Windows::Storage::Streams::DataReader^ dataReader,
            _Out_ SensorFrameStreamHeader^* header);

        static void Write(
            _In_ SensorFrameStreamHeader^ header,
            _Inout_ Windows::Storage::Streams::DataWriter^ dataWriter);
    };
}
