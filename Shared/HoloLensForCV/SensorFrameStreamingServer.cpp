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
    SensorFrameStreamingServer::SensorFrameStreamingServer(
        _In_ Platform::String^ serviceName)
        : _writeInProgress(false)
    {
        _listener = ref new Windows::Networking::Sockets::StreamSocketListener();

        _listener->ConnectionReceived +=
            ref new Windows::Foundation::TypedEventHandler<
                Windows::Networking::Sockets::StreamSocketListener^,
                Windows::Networking::Sockets::StreamSocketListenerConnectionReceivedEventArgs^>(
                    this,
                    &SensorFrameStreamingServer::OnConnection);

        _listener->Control->KeepAlive = true;

        // Don't limit traffic to an address or an adapter.
        Concurrency::create_task(_listener->BindServiceNameAsync(serviceName)).then(
            [this](Concurrency::task<void> previousTask)
        {
            try
            {
                // Try getting an exception.
                previousTask.get();
            }
            catch (Platform::Exception^ exception)
            {
#if DBG_ENABLE_ERROR_LOGGING
                dbg::trace(
                    L"SensorFrameStreamingServer::SensorFrameStreamingServer: %s",
                    exception->Message->Data());
#endif /* DBG_ENABLE_ERROR_LOGGING */
            }
        });
    }

    SensorFrameStreamingServer::~SensorFrameStreamingServer()
    {
        // The listener can be closed in two ways:
        //  - explicit: by using delete operator (the listener is closed even if there are outstanding references to it).
        //  - implicit: by removing last reference to it (i.e. falling out-of-scope).
        // In this case this is the last reference to the listener so both will yield the same result.
        delete _listener;
        _listener = nullptr;
    }

    void SensorFrameStreamingServer::OnConnection(
        Windows::Networking::Sockets::StreamSocketListener^ listener,
        Windows::Networking::Sockets::StreamSocketListenerConnectionReceivedEventArgs^ object)
    {
        _socket = object->Socket;

        _writeInProgress = false;

        _writer = ref new Windows::Storage::Streams::DataWriter(
            _socket->OutputStream);

        _writer->UnicodeEncoding =
            Windows::Storage::Streams::UnicodeEncoding::Utf8;

        _writer->ByteOrder =
            Windows::Storage::Streams::ByteOrder::LittleEndian;
    }

    void SensorFrameStreamingServer::Send(
        SensorFrame^ sensorFrame)
    {
        if (nullptr == _socket)
        {
#if DBG_ENABLE_VERBOSE_LOGGING
            dbg::trace(
                L"SensorFrameStreamingServer::Consume: image dropped -- no connection!");
#endif /* DBG_ENABLE_VERBOSE_LOGGING */

            return;
        }

        if (_writeInProgress)
        {
#if DBG_ENABLE_INFORMATIONAL_LOGGING
            dbg::trace(
                L"SensorFrameStreamingServer::Send: image dropped -- previous send operation is in progress!");
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

            return;
        }

        Windows::Graphics::Imaging::SoftwareBitmap^ bitmap;
        Windows::Graphics::Imaging::BitmapBuffer^ bitmapBuffer;
        Windows::Foundation::IMemoryBufferReference^ bitmapBufferReference;

        int32_t imageWidth = 0;
        int32_t imageHeight = 0;
        int32_t pixelStride = 1;
        int32_t rowStride = 0;

        Platform::Array<uint8_t>^ imageBufferAsPlatformArray;
        int32_t imageBufferSize = 0;

        {
#if DBG_ENABLE_INFORMATIONAL_LOGGING
            dbg::TimerGuard timerGuard(
                L"SensorFrameStreamingServer::Send: buffer preparation",
                4.0 /* minimum_time_elapsed_in_milliseconds */);
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

            bitmap =
                sensorFrame->SoftwareBitmap;

            imageWidth = bitmap->PixelWidth;
            imageHeight = bitmap->PixelHeight;

            bitmapBuffer =
                bitmap->LockBuffer(
                    Windows::Graphics::Imaging::BitmapBufferAccessMode::Read);

            bitmapBufferReference =
                bitmapBuffer->CreateReference();

            uint32_t bitmapBufferDataSize = 0;

            uint8_t* bitmapBufferData =
                Io::GetTypedPointerToMemoryBuffer<uint8_t>(
                    bitmapBufferReference,
                    bitmapBufferDataSize);

            switch (bitmap->BitmapPixelFormat)
            {
            case Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8:
                pixelStride = 4;
                break;

            case Windows::Graphics::Imaging::BitmapPixelFormat::Gray16:
                pixelStride = 2;
                break;

            case Windows::Graphics::Imaging::BitmapPixelFormat::Gray8:
                pixelStride = 1;
                break;

            default:
#if DBG_ENABLE_INFORMATIONAL_LOGGING
                dbg::trace(
                    L"SensorFrameStreamingServer::Send: unrecognized bitmap pixel format, assuming 1 byte per pixel");
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

                break;
            }

            rowStride =
                imageWidth * pixelStride;

            imageBufferSize =
                imageHeight * rowStride;

            ASSERT(
                imageBufferSize == (int32_t)bitmapBufferDataSize);

            imageBufferAsPlatformArray =
                ref new Platform::Array<uint8_t>(
                    bitmapBufferData,
                    imageBufferSize);
        }

        SensorFrameStreamHeader^ header =
            ref new SensorFrameStreamHeader();

        header->FrameType = sensorFrame->FrameType;
        header->Timestamp = sensorFrame->Timestamp.UniversalTime;
        header->ImageWidth = imageWidth;
        header->ImageHeight = imageHeight;
        header->PixelStride = pixelStride;
        header->RowStride = rowStride;

        SendImage(
            header,
            imageBufferAsPlatformArray);
    }

    void SensorFrameStreamingServer::SendImage(
        SensorFrameStreamHeader^ header,
        const Platform::Array<uint8_t>^ data)
    {
        if (nullptr == _socket)
        {
#if DBG_ENABLE_VERBOSE_LOGGING
            dbg::trace(
                L"SensorFrameStreamingServer::SendImage: image dropped -- no connection!");
#endif /* DBG_ENABLE_VERBOSE_LOGGING */

            return;
        }

        if (_writeInProgress)
        {
#if DBG_ENABLE_INFORMATIONAL_LOGGING
            dbg::trace(
                L"SensorFrameStreamingServer::SendImage: image dropped -- previous StoreAsync task is still in progress!");
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

            return;
        }

        _writeInProgress = true;

        {
#if DBG_ENABLE_INFORMATIONAL_LOGGING
            dbg::TimerGuard timerGuard(
                L"SensorFrameStreamingServer::SendImage: writer operations",
                4.0 /* minimum_time_elapsed_in_milliseconds */);
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

            SensorFrameStreamHeader::Write(
                header,
                _writer);

            _writer->WriteBytes(
                data);
        }

#if DBG_ENABLE_INFORMATIONAL_LOGGING
        dbg::TimerGuard timerGuard(
            L"SensorFrameStreamingServer::SendImage: StoreAsync task creation",
            10.0 /* minimum_time_elapsed_in_milliseconds */);
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

        Concurrency::create_task(_writer->StoreAsync()).then(
            [&](Concurrency::task<unsigned int> writeTask)
        {
            try
            {
                // Try getting an exception.
                writeTask.get();

                _writeInProgress = false;
            }
            catch (Platform::Exception^ exception)
            {
#if DBG_ENABLE_ERROR_LOGGING
                dbg::trace(
                    L"SensorFrameStreamingServer::SendImage: StoreAsync call failed with error: %s",
                    exception->Message->Data());
#endif /* DBG_ENABLE_ERROR_LOGGING */

                _socket = nullptr;
            }
        });
    }
}
