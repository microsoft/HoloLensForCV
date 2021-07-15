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
    SensorFrameReceiver::SensorFrameReceiver(
        _In_ Windows::Networking::Sockets::StreamSocket^ streamSocket)
        : _streamSocket(streamSocket)
    {
        _reader = ref new Windows::Storage::Streams::DataReader(
            _streamSocket->InputStream);

        _reader->UnicodeEncoding =
            Windows::Storage::Streams::UnicodeEncoding::Utf8;

        _reader->ByteOrder =
            Windows::Storage::Streams::ByteOrder::LittleEndian;
    }

    Concurrency::task<SensorFrameStreamHeader^> SensorFrameReceiver::ReceiveSensorFrameStreamHeaderAsync()
    {
        return concurrency::create_task(
            _reader->LoadAsync(
                SensorFrameStreamHeader::ProtocolHeaderLength)
        ).then([this](concurrency::task<unsigned int> headerBytesLoadedTaskResult)
        {
            //
            // Make sure that we have received exactly the number of bytes we have
            // asked for. Doing so will also implicitly check for the possible exceptions
            // that could have been thrown in the async call chain.
            //
            const size_t headerBytesLoaded = headerBytesLoadedTaskResult.get();

            if (SensorFrameStreamHeader::ProtocolHeaderLength != headerBytesLoaded)
            {
#if DBG_ENABLE_ERROR_LOGGING
                dbg::trace(
                    L"SensorFrameReceiver::ReceiveAsync: expected SensorFrameStreamHeader of %i bytes, got %i bytes",
                    SensorFrameStreamHeader::ProtocolHeaderLength,
                    headerBytesLoaded);
#endif /* DBG_ENABLE_ERROR_LOGGING */

                throw ref new Platform::FailureException();
            }

            SensorFrameStreamHeader^ header;

            SensorFrameStreamHeader::Read(
                _reader,
                &header);

            if (SensorFrameStreamHeader::ProtocolCookie != header->Cookie ||
                SensorFrameStreamHeader::ProtocolVersionMajor != header->VersionMajor ||
                SensorFrameStreamHeader::ProtocolVersionMinor != header->VersionMinor)
            {
#if DBG_ENABLE_ERROR_LOGGING
                dbg::trace(
                    L"SensorFrameReceiver::ReceiveAsync: expected ProtocolCookie/ProtocolVersionMajor/ProtocolVersionMinor of 0x%08x/0x%02x/0x%02x, got 0x%08x/0x%02x/0x%02x",
                    SensorFrameStreamHeader::ProtocolCookie,
                    SensorFrameStreamHeader::ProtocolVersionMajor,
                    SensorFrameStreamHeader::ProtocolVersionMinor,
                    header->Cookie,
                    header->VersionMajor,
                    header->VersionMinor);
#endif /* DBG_ENABLE_ERROR_LOGGING */

                throw ref new Platform::FailureException();
            }

#if DBG_ENABLE_INFORMATIONAL_LOGGING
            dbg::trace(
                L"SensorFrameReceiver::ReceiveAsync: seeing a %ix%i image with pixel stride %i at timestamp %llu",
                header->ImageWidth,
                header->ImageHeight,
                header->PixelStride,
                header->Timestamp);
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

            return header;
        });
    }

    Concurrency::task<SensorFrame^> SensorFrameReceiver::ReceiveSensorFrameAsync(
        SensorFrameStreamHeader^ header)
    {
        return concurrency::create_task(
            _reader->LoadAsync(
                header->ImageHeight * header->RowStride)).
            then([this, header](concurrency::task<unsigned int> frameBytesLoadedTaskResult)
        {
            //
            // Make sure that we have received exactly the number of bytes we have
            // asked for. Doing so will also implicitly check for the possible exceptions
            // that could have been thrown in the async call chain.
            //
            const size_t frameBytesLoaded = frameBytesLoadedTaskResult.get();

            if (header->ImageHeight * header->RowStride != frameBytesLoaded)
            {
#if DBG_ENABLE_ERROR_LOGGING
                dbg::trace(
                    L"SensorFrameReceiver::ReceiveAsync: expected image frame data of %i bytes, got %i bytes",
                    header->ImageHeight * header->RowStride,
                    frameBytesLoaded);
#endif /* DBG_ENABLE_ERROR_LOGGING */

                throw ref new Platform::FailureException();
            }

            Windows::Storage::Streams::IBuffer^ frameAsBuffer =
                _reader->ReadBuffer(
                    static_cast<uint32_t>(frameBytesLoaded));

            Windows::Graphics::Imaging::BitmapPixelFormat pixelFormat;
            uint32_t packedImageWidthMultiplier = 1;

            switch (header->FrameType)
            {
            case SensorType::PhotoVideo:
                pixelFormat = Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8;
                break;

            case SensorType::ShortThrowToFDepth:
            case SensorType::LongThrowToFDepth:
                pixelFormat = Windows::Graphics::Imaging::BitmapPixelFormat::Gray16;
                break;

            case SensorType::ShortThrowToFReflectivity:
            case SensorType::LongThrowToFReflectivity:
                pixelFormat = Windows::Graphics::Imaging::BitmapPixelFormat::Gray8;
                break;

            case SensorType::VisibleLightLeftLeft:
            case SensorType::VisibleLightLeftFront:
            case SensorType::VisibleLightRightFront:
            case SensorType::VisibleLightRightRight:
                pixelFormat = Windows::Graphics::Imaging::BitmapPixelFormat::Gray8;
                packedImageWidthMultiplier = 4;
                break;

            default:
#if DBG_ENABLE_ERROR_LOGGING
                dbg::trace(
                    L"SensorFrameReceiver::ReceiveAsync: unrecognized sensor type %i",
                    header->FrameType);
#endif /* DBG_ENABLE_ERROR_LOGGING */

                throw ref new Platform::FailureException();
            }

            Windows::Graphics::Imaging::SoftwareBitmap^ frameAsSoftwareBitmap =
                Windows::Graphics::Imaging::SoftwareBitmap::CreateCopyFromBuffer(
                    frameAsBuffer,
                    pixelFormat,
                    header->ImageWidth * packedImageWidthMultiplier,
                    header->ImageHeight,
                    Windows::Graphics::Imaging::BitmapAlphaMode::Ignore);

            //
            // Timestamps on the wire are encoded as universal time
            //
            Windows::Foundation::DateTime frameTimestamp;

            frameTimestamp.UniversalTime =
                header->Timestamp;

            SensorFrame^ sensorFrame =
                ref new SensorFrame(
                    header->FrameType,
                    frameTimestamp,
                    frameAsSoftwareBitmap);

            //TODO: add support for sending and receiving camera intrinsics and extrinsics

            return sensorFrame;
        });
    }

    Windows::Foundation::IAsyncOperation<SensorFrame^>^ SensorFrameReceiver::ReceiveAsync()
    {
        return concurrency::create_async(
            [this]()
        {
            return ReceiveSensorFrameStreamHeaderAsync().then(
                [this](concurrency::task<SensorFrameStreamHeader^> header)
            {
                return ReceiveSensorFrameAsync(
                    header.get());
            });
        });
    }
}
