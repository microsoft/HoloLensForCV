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
#include "MainPage.xaml.h"

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

namespace ComputeOnDesktop
{
    MainPage::MainPage()
    {
        InitializeComponent();
    }

    void MainPage::ConnectSocket_Click()
    {
        //
        // By default 'HostNameForConnect' is disabled and host name validation is not required. When enabling the
        // text box validating the host name is required since it was received from an untrusted source
        // (user input). The host name is validated by catching ArgumentExceptions thrown by the HostName
        // constructor for invalid input.
        //
        Windows::Networking::HostName^ hostName;

        try
        {
            hostName = ref new Windows::Networking::HostName(
                HostNameForConnect->Text);
        }
        catch (Platform::Exception^)
        {
            return;
        }

        Windows::Networking::Sockets::StreamSocket^ pvCameraSocket =
            ref new Windows::Networking::Sockets::StreamSocket();

        pvCameraSocket->Control->KeepAlive = true;

        //
        // Save the socket, so subsequent steps can use it.
        //
        concurrency::create_task(
            pvCameraSocket->ConnectAsync(hostName, _pvServiceName->Text)
        ).then(
            [this, pvCameraSocket]()
        {
#if DBG_ENABLE_VERBOSE_LOGGING
            dbg::trace(
                L"MainPage::ConnectSocket_Click: server connection established");
#endif /* DBG_ENABLE_VERBOSE_LOGGING */

            HoloLensForCV::SensorFrameReceiver^ receiver =
                ref new HoloLensForCV::SensorFrameReceiver(
                    pvCameraSocket);

            ReceiverLoop(
                receiver);
        });
    }

    void MainPage::ReceiverLoop(
        HoloLensForCV::SensorFrameReceiver^ receiver)
    {
        concurrency::create_task(
            receiver->ReceiveAsync()).then(
                [this, receiver](concurrency::task<HoloLensForCV::SensorFrame^> sensorFrameTask)
        {
            HoloLensForCV::SensorFrame^ sensorFrame =
                sensorFrameTask.get();

#if DBG_ENABLE_VERBOSE_LOGGING
            dbg::trace(
                L"MainPage::ReceiverLoop: receiving a %ix%i image of type %i with timestamp %llu",
                sensorFrame->SoftwareBitmap->PixelWidth,
                sensorFrame->SoftwareBitmap->PixelHeight,
                (int32_t)sensorFrame->FrameType,
                sensorFrame->Timestamp);
#endif /* DBG_ENABLE_VERBOSE_LOGGING */

            OnFrameReceived(
                sensorFrame);

            Windows::UI::Core::CoreDispatcher^ uiThreadDispatcher =
                Windows::ApplicationModel::Core::CoreApplication::MainView->CoreWindow->Dispatcher;

            uiThreadDispatcher->RunAsync(
                Windows::UI::Core::CoreDispatcherPriority::Normal,
                ref new Windows::UI::Core::DispatchedHandler(
                    [this, sensorFrame]()
            {

                switch (sensorFrame->FrameType)
                {
                case HoloLensForCV::SensorType::PhotoVideo:
                {
                    Windows::UI::Xaml::Media::Imaging::SoftwareBitmapSource^ imageSource =
                        ref new Windows::UI::Xaml::Media::Imaging::SoftwareBitmapSource();

                    concurrency::create_task(
                        imageSource->SetBitmapAsync(sensorFrame->SoftwareBitmap)
                    ).then(
                        [this, imageSource]()
                    {
                        _pvImage->Source = imageSource;

                    }, concurrency::task_continuation_context::use_current());
                }
                break;

                default:
                    throw new std::logic_error("invalid frame type");
                }
            }));

            ReceiverLoop(
                receiver);
        });
    }

    void MainPage::OnFrameReceived(
        HoloLensForCV::SensorFrame^ sensorFrame)
    {
        cv::Mat wrappedImage;

        rmcv::WrapHoloLensSensorFrameWithCvMat(
            sensorFrame,
            wrappedImage);

        cv::Mat blurredImage;

        cv::medianBlur(
            wrappedImage,
            blurredImage,
            3 /* ksize */);

        cv::Mat cannyImage;

        cv::Canny(
            blurredImage,
            cannyImage,
            50.0,
            200.0);

        for (int32_t y = 0; y < blurredImage.rows; ++y)
        {
            for (int32_t x = 0; x < blurredImage.cols; ++x)
            {
                if (cannyImage.at<uint8_t>(y, x) > 64)
                {
                    wrappedImage.at<uint32_t>(y, x) = 0xFF00FF00;
                }
            }
        }
    }
}
