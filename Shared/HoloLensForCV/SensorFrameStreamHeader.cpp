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
    SensorFrameStreamHeader::SensorFrameStreamHeader()
    {
        Cookie = ProtocolCookie;
        VersionMajor = ProtocolVersionMajor;
        VersionMinor = ProtocolVersionMinor;
        FrameType = SensorType::Undefined;
        Timestamp = 0;
        ImageWidth = 0;
        ImageHeight = 0;
        PixelStride = 0;
        RowStride = 0;
    }

    /* static */ void SensorFrameStreamHeader::Read(
        _Inout_ Windows::Storage::Streams::DataReader^ dataReader,
        _Out_ SensorFrameStreamHeader^* headerReference)
    {
        SensorFrameStreamHeader^ header =
            ref new SensorFrameStreamHeader();

        header->Cookie = dataReader->ReadUInt32();
        header->VersionMajor = dataReader->ReadByte();
        header->VersionMinor = dataReader->ReadByte();
        header->FrameType = (SensorType)dataReader->ReadUInt16();
        header->Timestamp = dataReader->ReadUInt64();
        header->ImageWidth = dataReader->ReadUInt32();
        header->ImageHeight = dataReader->ReadUInt32();
        header->PixelStride = dataReader->ReadUInt32();
        header->RowStride = dataReader->ReadUInt32();

        *headerReference = header;
    }

    /* static */ void SensorFrameStreamHeader::Write(
        _In_ SensorFrameStreamHeader^ header,
        _Inout_ Windows::Storage::Streams::DataWriter^ dataWriter)
    {
        dataWriter->WriteUInt32(header->Cookie);
        dataWriter->WriteByte(header->VersionMajor);
        dataWriter->WriteByte(header->VersionMinor);
        dataWriter->WriteUInt16((uint16_t)header->FrameType);
        dataWriter->WriteUInt64(header->Timestamp);
        dataWriter->WriteUInt32(header->ImageWidth);
        dataWriter->WriteUInt32(header->ImageHeight);
        dataWriter->WriteUInt32(header->PixelStride);
        dataWriter->WriteUInt32(header->RowStride);
    }
}
