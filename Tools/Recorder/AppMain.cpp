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
#include "AppMain.h"

//#define RECORDER_USE_SPEECH
//#define ENABLE_RENDERING

using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;
using namespace Windows::Networking;
using namespace Windows::Networking::Connectivity;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;
using namespace Windows::Graphics::Holographic;
using namespace Windows::Perception::Spatial;
using namespace Windows::UI::Input::Spatial;

namespace Recorder
{
    AppMain::AppMain(const std::shared_ptr<Graphics::DeviceResources>& deviceResources)
        : Holographic::AppMainBase(deviceResources)
        , _speechSynthesizer(ref new Windows::Media::SpeechSynthesis::SpeechSynthesizer())
        , _mediaFrameSourceGroupStarted(false)
        , _sensorFrameRecorderStarted(false)
        , _cameraPreviewTimestamp()
        , _maxTimeToRecordInSeconds(0)
    {
    }

    void AppMain::OnHolographicSpaceChanged(
        Windows::Graphics::Holographic::HolographicSpace^ holographicSpace)
    {
        //
        // Initialize the camera preview hologram.
        //
        _slateRenderer =
            std::make_unique<Rendering::SlateRenderer>(
                _deviceResources);

        //
        // Initialize the HoloLens media frame readers.
        //
        StartHoloLensMediaFrameSourceGroup();

#ifdef RECORDER_USE_SPEECH
        StartRecognizeSpeechCommands();
        Platform::StringReference startSentence =
            L"Say 'start' to begin, and 'stop' to end recording";
#else
        Platform::StringReference startSentence =
            L"Air tap to begin and end recording";
#endif
        SaySentence(startSentence);
    }

    void AppMain::OnSpatialInput(
        _In_ Windows::UI::Input::Spatial::SpatialInteractionSourceState^ pointerState)
    {
#ifdef ENABLE_RENDERING
        // When a Pressed gesture is detected, the sample hologram will be repositioned
        // two meters in front of the user.
        Windows::Perception::Spatial::SpatialCoordinateSystem^ currentCoordinateSystem =
            _spatialPerception->GetOriginFrameOfReference()->CoordinateSystem;
        _slateRenderer->PositionHologram(
            pointerState->TryGetPointerPose(currentCoordinateSystem));
#endif 

#ifndef RECORDER_USE_SPEECH
        if (_sensorFrameRecorderStarted) {
            concurrency::create_async([&]() { StopRecording(); });
        }
        else
        {
            concurrency::create_async([&]() { StartRecording(); });
        }
#endif // ! RECORDER_USE_SPEECH
    }

    // Updates the application state once per frame.
    void AppMain::OnUpdate(
        _In_ Windows::Graphics::Holographic::HolographicFrame^ holographicFrame,
        _In_ const Graphics::StepTimer& stepTimer)
    {
        UNREFERENCED_PARAMETER(holographicFrame);

        dbg::TimerGuard timerGuard(
            L"AppMain::OnUpdate",
            30.0 /* minimum_time_elapsed_in_milliseconds */);

#ifdef ENABLE_RENDERING
        //
        // Update scene objects.
        //
        // Put time-based updates here. By default this code will run once per frame,
        // but if you change the StepTimer to use a fixed time step this code will
        // run as many times as needed to get to the current step.
        //
        _slateRenderer->Update(
            stepTimer);
#else
        UNREFERENCED_PARAMETER(stepTimer);
#endif

        // Check for timer elapse
        if (_sensorFrameRecorderStarted && (_maxTimeToRecordInSeconds != 0))
        {
            unsigned int elapsedTime = (int)(_recordTimer.GetMillisecondsFromStart() / 1000.0);
            if (elapsedTime >= _maxTimeToRecordInSeconds)
            {
                concurrency::create_async([&]() { StopRecording(); });
            }
        }

        //
        // Process sensor data received through the HoloLensForCV component.
        //
        if (!_mediaFrameSourceGroupStarted)
        {
            return;
        }

// TODO: Enable camera preview based on sensor selection
#ifdef ENABLE_RENDERING
        HoloLensForCV::SensorFrame^ latestCameraPreviewFrame =
            _mediaFrameSourceGroup->GetLatestSensorFrame(
                HoloLensForCV::SensorType::ShortThrowToFReflectivity);

        if (nullptr == latestCameraPreviewFrame)
        {
            return;
        }

        if (_cameraPreviewTimestamp.UniversalTime == latestCameraPreviewFrame->Timestamp.UniversalTime)
        {
            return;
        }

        _cameraPreviewTimestamp = latestCameraPreviewFrame->Timestamp;

        if (nullptr == _cameraPreviewTexture)
        {
            _cameraPreviewTexture =
                std::make_shared<Rendering::Texture2D>(
                    _deviceResources,
                    latestCameraPreviewFrame->SoftwareBitmap->PixelWidth,
                    latestCameraPreviewFrame->SoftwareBitmap->PixelHeight,
                    DXGI_FORMAT_B8G8R8A8_UNORM);
        }

        {
            void* mappedTexture =
                _cameraPreviewTexture->MapCPUTexture<void>(
                    D3D11_MAP_WRITE /* mapType */);

            Windows::Graphics::Imaging::SoftwareBitmap^ bitmap =
                latestCameraPreviewFrame->SoftwareBitmap;

            REQUIRES(Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8 == bitmap->BitmapPixelFormat);

            Windows::Graphics::Imaging::BitmapBuffer^ bitmapBuffer =
                bitmap->LockBuffer(
                    Windows::Graphics::Imaging::BitmapBufferAccessMode::Read);

            uint32_t pixelBufferDataLength = 0;

            uint8_t* pixelBufferData =
                Io::GetTypedPointerToMemoryBuffer<uint8_t>(
                    bitmapBuffer->CreateReference(),
                    pixelBufferDataLength);

            const int32_t bytesToCopy =
                _cameraPreviewTexture->GetWidth() * _cameraPreviewTexture->GetHeight() * 4;

            ASSERT(static_cast<uint32_t>(bytesToCopy) == pixelBufferDataLength);

            memcpy_s(
                mappedTexture,
                bytesToCopy,
                pixelBufferData,
                pixelBufferDataLength);

            _cameraPreviewTexture->UnmapCPUTexture();
        }

        _cameraPreviewTexture->CopyCPU2GPU();
#endif
    }

    void AppMain::OnPreRender()
    {
    }

    // Renders the current frame to each holographic camera, according to the
    // current application and spatial positioning state.
    void AppMain::OnRender()
    {
#ifdef ENABLE_RENDERING
        // Draw the sample hologram.
        _slateRenderer->Render(
            _cameraPreviewTexture);
#endif
    }

    // Notifies classes that use Direct3D device resources that the device resources
    // need to be released before this method returns.
    void AppMain::OnDeviceLost()
    {
#ifdef ENABLE_RENDERING
        _slateRenderer->ReleaseDeviceDependentResources();
        _cameraPreviewTexture.reset();
        _cameraPreviewTimestamp.UniversalTime = 0;
#endif
    }

    // Notifies classes that use Direct3D device resources that the device resources
    // may now be recreated.
    void AppMain::OnDeviceRestored()
    {
#ifdef ENABLE_RENDERING
        _slateRenderer->CreateDeviceDependentResources();
#endif
    }

    void AppMain::StartRecording()
    {
        std::unique_lock<std::mutex> lock(_startStopRecordingMutex);
        
        if (!_mediaFrameSourceGroupStarted || _sensorFrameRecorderStarted)
        {
            return;
        }

        SaySentence(Platform::StringReference(L"Beginning recording"));

        auto sensorFrameRecorderStartAsyncTask =
            concurrency::create_task(
                _sensorFrameRecorder->StartAsync());

        sensorFrameRecorderStartAsyncTask.then([&]()
        {
            _sensorFrameRecorderStarted = true;
        });
    }

    void AppMain::StopRecording()
    {
        std::unique_lock<std::mutex> lock(_startStopRecordingMutex);

        if (!_mediaFrameSourceGroupStarted || !_sensorFrameRecorderStarted)
        {
            return;
        }

        SaySentence(Platform::StringReference(L"Ending recording, wait a moment to finish"));

        _sensorFrameRecorder->Stop();
        _sensorFrameRecorderStarted = false;

        SaySentence(Platform::StringReference(L"Finished recording"));
    }

    concurrency::task<void> AppMain::StopCurrentRecognizerIfExists()
    {
        if (_speechRecognizer != nullptr)
        {
            return concurrency::create_task(_speechRecognizer->StopRecognitionAsync()).
                then([this]()
            {
                _speechRecognizer->RecognitionQualityDegrading -=
                    _speechRecognitionQualityDegradedToken;

                if (_speechRecognizer->ContinuousRecognitionSession != nullptr)
                {
                    _speechRecognizer->ContinuousRecognitionSession->ResultGenerated -=
                        _speechRecognizerResultEventToken;
                }
            });
        }
        else
        {
            return concurrency::create_task([this]() {});
        }
    }

    bool AppMain::InitializeSpeechRecognizer()
    {
        _speechRecognizer =
            ref new Windows::Media::SpeechRecognition::SpeechRecognizer();

        if (!_speechRecognizer)
        {
            return false;
        }

        _speechRecognitionQualityDegradedToken = _speechRecognizer->RecognitionQualityDegrading +=
            ref new TypedEventHandler<Windows::Media::SpeechRecognition::SpeechRecognizer^, Windows::Media::SpeechRecognition::SpeechRecognitionQualityDegradingEventArgs^>(
                std::bind(&AppMain::OnSpeechQualityDegraded, this, std::placeholders::_1, std::placeholders::_2)
                );

        _speechRecognizerResultEventToken = _speechRecognizer->ContinuousRecognitionSession->ResultGenerated +=
            ref new TypedEventHandler<Windows::Media::SpeechRecognition::SpeechContinuousRecognitionSession^, Windows::Media::SpeechRecognition::SpeechContinuousRecognitionResultGeneratedEventArgs^>(
                std::bind(&AppMain::OnResultGenerated, this, std::placeholders::_1, std::placeholders::_2)
                );

        return true;
    }

    concurrency::task<bool> AppMain::StartRecognizeSpeechCommands()
    {
        return StopCurrentRecognizerIfExists().then([this]()
        {
            if (!InitializeSpeechRecognizer())
            {
                return concurrency::task_from_result<bool>(false);
            }

            // Here, we compile the list of voice commands by reading them from the map.
            Platform::Collections::Vector<Platform::String^>^ speechCommandList =
                ref new Platform::Collections::Vector<Platform::String^>();

            speechCommandList->Append(L"start");
            speechCommandList->Append(L"stop");

            Windows::Media::SpeechRecognition::SpeechRecognitionListConstraint^ spConstraint =
                ref new Windows::Media::SpeechRecognition::SpeechRecognitionListConstraint(
                    speechCommandList);
            
            _speechRecognizer->Constraints->Clear();
            _speechRecognizer->Constraints->Append(spConstraint);
            
            return concurrency::create_task(
                _speechRecognizer->CompileConstraintsAsync()).
                then([this](concurrency::task<Windows::Media::SpeechRecognition::SpeechRecognitionCompilationResult^> previousTask)
            {
                try
                {
                    Windows::Media::SpeechRecognition::SpeechRecognitionCompilationResult^ compilationResult =
                        previousTask.get();

                    if (compilationResult->Status == Windows::Media::SpeechRecognition::SpeechRecognitionResultStatus::Success)
                    {
                        // If compilation succeeds, we can start listening for results.
                        return concurrency::create_task(
                            _speechRecognizer->ContinuousRecognitionSession->StartAsync()).
                            then([this](concurrency::task<void> startAsyncTask)
                        {
                            try
                            {
                                // StartAsync may throw an exception if your app doesn't have Microphone permissions. 
                                // Make sure they're caught and handled appropriately (otherwise the app may silently not work as expected)
                                startAsyncTask.get();
                                return true;
                            }
                            catch (Platform::Exception^ exception)
                            {
                                dbg::trace(
                                    L"Exception while trying to start speech recognition: %s",
                                    exception->Message->Data());

                                return false;
                            }
                        });
                    }
                    else
                    {
                        dbg::trace(
                            L"Could not initialize constraint-based speech engine!");

                        // Handle errors here.
                        return concurrency::create_task([this] { return false; });
                    }
                }
                catch (Platform::Exception^ exception)
                {
                    // Note that if you get an "Access is denied" exception, you might need to enable the microphone 
                    // privacy setting on the device and/or add the microphone capability to your app manifest.

                    dbg::trace(
                        L"Exception while trying to initialize speech command list: %s",
                        exception->Message->Data());

                    // Handle exceptions here.
                    return concurrency::create_task([this] { return false; });
                }
            });
        });
    }

    void AppMain::SaySentence(Platform::StringReference sentence)
    {
        concurrency::create_task(
            _speechSynthesizer->SynthesizeTextToStreamAsync(sentence),
            concurrency::task_continuation_context::use_current()).
            then([this](
                concurrency::task<Windows::Media::SpeechSynthesis::SpeechSynthesisStream^> synthesisStreamTask)
        {
            try
            {
                auto stream =
                    synthesisStreamTask.get();

                auto hr =
                    _speechSynthesisSound.Initialize(
                        stream,
                        0 /* loopCount */);

                if (SUCCEEDED(hr))
                {
                    _speechSynthesisSound.SetEnvironment(HrtfEnvironment::Small);
                    _speechSynthesisSound.Start();
                }
            }
            catch (Platform::Exception^ exception)
            {
                dbg::trace(
                    L"Exception while trying to synthesize speech: %s",
                    exception->Message->Data());
            }
        });
    }

    void AppMain::OnResultGenerated(
        Windows::Media::SpeechRecognition::SpeechContinuousRecognitionSession ^sender,
        Windows::Media::SpeechRecognition::SpeechContinuousRecognitionResultGeneratedEventArgs ^args)
    {
        // For our list of commands, medium confidence is good enough. 
        // We also accept results that have high confidence.
        if ((args->Result->Confidence == Windows::Media::SpeechRecognition::SpeechRecognitionConfidence::High) ||
            (args->Result->Confidence == Windows::Media::SpeechRecognition::SpeechRecognitionConfidence::Medium))
        {
            if (args->Result->Text == L"start")
            {
                concurrency::create_async([&]() { StartRecording(); });
            }
            else if (args->Result->Text == L"stop")
            {
                concurrency::create_async([&]() { StopRecording(); });
            }

            // When the debugger is attached, we can print information to the debug console.
            dbg::trace(
                L"Last voice command was: '%s'",
                args->Result->Text->Data());
        }
        else
        {
            dbg::trace(
                L"Recognition confidence not high enough.");
        }
    }

    void AppMain::OnSpeechQualityDegraded(
        Windows::Media::SpeechRecognition::SpeechRecognizer^ recognizer,
        Windows::Media::SpeechRecognition::SpeechRecognitionQualityDegradingEventArgs^ args)
    {
        switch (args->Problem)
        {
        case Windows::Media::SpeechRecognition::SpeechRecognitionAudioProblem::TooFast:
            dbg::trace(L"The user spoke too quickly.");
            break;

        case Windows::Media::SpeechRecognition::SpeechRecognitionAudioProblem::TooSlow:
            dbg::trace(L"The user spoke too slowly.");
            break;

        case Windows::Media::SpeechRecognition::SpeechRecognitionAudioProblem::TooQuiet:
            dbg::trace(L"The user spoke too softly.");
            break;

        case Windows::Media::SpeechRecognition::SpeechRecognitionAudioProblem::TooLoud:
            dbg::trace(L"The user spoke too loudly.");
            break;

        case Windows::Media::SpeechRecognition::SpeechRecognitionAudioProblem::TooNoisy:
            dbg::trace(L"There is too much noise in the signal.");
            break;

        case Windows::Media::SpeechRecognition::SpeechRecognitionAudioProblem::NoSignal:
            dbg::trace(L"There is no signal.");
            break;

        case Windows::Media::SpeechRecognition::SpeechRecognitionAudioProblem::None:
        default:
            dbg::trace(L"An error was reported with no information.");
            break;
        }
    }

    void AppMain::PlayVoiceCommandRecognitionSound()
    {
        // The user should be given a cue when recognition is complete. 
        auto hr = _voiceCommandRecognitionSound.GetInitializationStatus();
        if (SUCCEEDED(hr))
        {
            // re-initialize the sound so it can be replayed.
            _voiceCommandRecognitionSound.Initialize(
                L"Audio\\BasicResultsEarcon.wav",
                0 /* loopCount */);

            _voiceCommandRecognitionSound.SetEnvironment(
                HrtfEnvironment::Small);

            _voiceCommandRecognitionSound.Start();
        }
    }

    void AppMain::StartHoloLensMediaFrameSourceGroup()
    {
        std::vector<HoloLensForCV::SensorType> enabledSensorTypes;

        //
        // Enabling all of the Research Mode sensors at the same time can be quite expensive
        // performance-wise. It's best to scope down the list of enabled sensors to just those
        // that are required for a given task. In this example, we will select just the visible
        // light sensors.
        //
        enabledSensorTypes.emplace_back(
            HoloLensForCV::SensorType::VisibleLightLeftLeft);

        enabledSensorTypes.emplace_back(
            HoloLensForCV::SensorType::VisibleLightLeftFront);

        enabledSensorTypes.emplace_back(
            HoloLensForCV::SensorType::VisibleLightRightFront);

        enabledSensorTypes.emplace_back(
            HoloLensForCV::SensorType::VisibleLightRightRight);

        REQUIRES(
            !_mediaFrameSourceGroupStarted &&
            !_sensorFrameRecorderStarted &&
            nullptr != _spatialPerception);

        _sensorFrameRecorder =
            ref new HoloLensForCV::SensorFrameRecorder();

        for (const auto enabledSensorType : enabledSensorTypes)
        {
            _sensorFrameRecorder->Enable(
                enabledSensorType);
        }

        _mediaFrameSourceGroup =
            ref new HoloLensForCV::MediaFrameSourceGroup(
                HoloLensForCV::MediaFrameSourceGroupType::HoloLensResearchModeSensors,
                _spatialPerception,
                _sensorFrameRecorder);

        for (const auto enabledSensorType : enabledSensorTypes)
        {
            _mediaFrameSourceGroup->Enable(
                enabledSensorType);
        }

        auto captureSartAsyncTask =
            concurrency::create_task(
                _mediaFrameSourceGroup->StartAsync());

        captureSartAsyncTask.then([&]() {
            _mediaFrameSourceGroupStarted = true;
        });
    }
}
