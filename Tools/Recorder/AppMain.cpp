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

// By default all sensors are enabled. To only enable individual sensors, simply add types from HoloLensForCV::SensorType.
std::vector<HoloLensForCV::SensorType> kEnabledSensorTypes = {};

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
    , _photoVideoMediaFrameSourceGroupStarted(false)
    , _researchModeMediaFrameSourceGroupStarted(false)
    , _sensorFrameRecorderStarted(false)
	, _PVFrameRecorderStarted(false)
  {
  }

  void AppMain::OnHolographicSpaceChanged(
    Windows::Graphics::Holographic::HolographicSpace^ holographicSpace)
  {
    StartHoloLensMediaFrameSourceGroup();
	StartPVMediaFrameSourceGroup();

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
#ifndef RECORDER_USE_SPEECH
    if (_sensorFrameRecorderStarted) {
      concurrency::create_async([&]() { StopRecording(); });
    }
    else
    {
      concurrency::create_async([&]() { StartRecording(); });
    }
#endif // RECORDER_USE_SPEECH
  }

  void AppMain::OnUpdate(
    _In_ Windows::Graphics::Holographic::HolographicFrame^ holographicFrame,
    _In_ const Graphics::StepTimer& stepTimer) {
    UNREFERENCED_PARAMETER(holographicFrame);
    UNREFERENCED_PARAMETER(stepTimer);
  }

  void AppMain::LoadAppState()
  {
    StartHoloLensMediaFrameSourceGroup();
	StartPVMediaFrameSourceGroup();
  }

  void AppMain::SaveAppState()
  {
    if (_photoVideoMediaFrameSourceGroup == nullptr || _researchModeMediaFrameSourceGroup == nullptr) {
      return;
    };

	auto sensorFrameRecorderStopAsyncTask =
		concurrency::create_task(
			_mediaFrameSourceGroup->StopAsync());
	auto PVFrameRecorderStopAsyncTask =
		concurrency::create_task(
			_PVFrameSourceGroup->StopAsync());

    concurrency::create_task(_photoVideoMediaFrameSourceGroup->StopAsync()).then([&]()
    {
        concurrency::create_task(_researchModeMediaFrameSourceGroup->StopAsync()).then([&]()
        {
            StopRecording();
            
            delete _photoVideoMediaFrameSourceGroup;
            _photoVideoMediaFrameSourceGroup = nullptr;
            _photoVideoMediaFrameSourceGroupStarted = false;

            delete _researchModeMediaFrameSourceGroup;
            _researchModeMediaFrameSourceGroup = nullptr;
            _researchModeMediaFrameSourceGroupStarted = false;
        }).wait();
    }).wait();
  }

  void AppMain::OnPreRender() {}

  void AppMain::OnRender() {}

  void AppMain::StartRecording()
  {
    std::unique_lock<std::mutex> lock(_startStopRecordingMutex);

    if (!_photoVideoMediaFrameSourceGroupStarted || !_researchModeMediaFrameSourceGroupStarted || _sensorFrameRecorderStarted)
    {
      return;
    }

    SaySentence(Platform::StringReference(L"Beginning recording"));
	
	if (startSensorFrameRecorder)
	{
		auto sensorFrameRecorderStartAsyncTask =
			concurrency::create_task(
				_sensorFrameRecorder->StartAsync());

		sensorFrameRecorderStartAsyncTask.then([&]()
		{
			_sensorFrameRecorderStarted = true;
		});
	}

	// TODO: Can PV be better synchronized?
	if (startPVFrameRecorder)
	{
		auto PVFrameRecorderStartAsyncTask =
			concurrency::create_task(
				_PVFrameRecorder->StartAsync());
		PVFrameRecorderStartAsyncTask.then([&]()
		{
			_PVFrameRecorderStarted = true;
		});
	}
  }

  void AppMain::StopRecording()
  {
    std::unique_lock<std::mutex> lock(_startStopRecordingMutex);

    if (!_photoVideoMediaFrameSourceGroupStarted || !_researchModeMediaFrameSourceGroupStarted || !_sensorFrameRecorderStarted)
    {
      return;
    }

    SaySentence(Platform::StringReference(L"Ending recording, wait a moment to finish"));

	if (stopSensorFrameRecorder)
	{
		_sensorFrameRecorder->Stop();
		_sensorFrameRecorderStarted = false;
	}
	if (stopPVFrameRecorder)
	{
		_PVFrameRecorder->Stop();
		_PVFrameRecorderStarted = false;
	}	

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
        std::bind(&AppMain::OnSpeechResultGenerated, this, std::placeholders::_1, std::placeholders::_2)
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

  void AppMain::OnSpeechResultGenerated(
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
    REQUIRES(
      !_photoVideoMediaFrameSourceGroupStarted &&
      !_researchModeMediaFrameSourceGroupStarted &&
      !_sensorFrameRecorderStarted &&
      nullptr != _spatialPerception);

    _sensorFrameRecorder =
      ref new HoloLensForCV::SensorFrameRecorder();

    _photoVideoMediaFrameSourceGroup =
        ref new HoloLensForCV::MediaFrameSourceGroup(
            HoloLensForCV::MediaFrameSourceGroupType::PhotoVideoCamera,
            _spatialPerception, _sensorFrameRecorder);

    _researchModeMediaFrameSourceGroup =
        ref new HoloLensForCV::MediaFrameSourceGroup(
            HoloLensForCV::MediaFrameSourceGroupType::HoloLensResearchModeSensors,
            _spatialPerception, _sensorFrameRecorder);

    //
    // Enabling all of the Research Mode sensors at the same time can be quite expensive
    // performance-wise. It's best to scope down the list of enabled sensors to just those
    // that are required for a given task. In this example, all sensors are selected.
    // To only select a subset, use the commented code below.
    //


    if (kEnabledSensorTypes.empty())
    {
        _sensorFrameRecorder->EnableAll();
        _photoVideoMediaFrameSourceGroup->EnableAll();
        _researchModeMediaFrameSourceGroup->EnableAll();
    }
    else
    {
        for (const auto enabledSensorType : kEnabledSensorTypes)
        {
            _sensorFrameRecorder->Enable(enabledSensorType);
            if (enabledSensorType == HoloLensForCV::SensorType::PhotoVideo)
            {
                _photoVideoMediaFrameSourceGroup->Enable(enabledSensorType);
            }
            else
            {
                _researchModeMediaFrameSourceGroup->Enable(enabledSensorType);
            }
        }
    }

    //
    // Start the media frame source groups.
    //

    auto startPhotoVideoMediaFrameSourceGroupTask =
        concurrency::create_task(
            _photoVideoMediaFrameSourceGroup->StartAsync());

    startPhotoVideoMediaFrameSourceGroupTask.then([&]() {
        _photoVideoMediaFrameSourceGroupStarted = true;
    });

    auto startReseachModeMediaFrameSourceGroupTask =
        concurrency::create_task(
            _researchModeMediaFrameSourceGroup->StartAsync());

    startReseachModeMediaFrameSourceGroupTask.then([&]() {
        _researchModeMediaFrameSourceGroupStarted = true;
    });
  }

  void AppMain::StartPVMediaFrameSourceGroup()
  {
	  REQUIRES(
		  !_PVFrameSourceGroupStarted &&
		  !_PVFrameRecorderStarted &&
		  nullptr != _spatialPerception);

	  _PVFrameRecorder =
		  ref new HoloLensForCV::SensorFrameRecorder();
	  _PVFrameRecorder->SetPV();

	  _PVFrameSourceGroup =
		  ref new HoloLensForCV::MediaFrameSourceGroup(
			  HoloLensForCV::MediaFrameSourceGroupType::PhotoVideoCamera,
			  _spatialPerception, _PVFrameRecorder);
	  
	  // One could avoid initializing _PVFrameRecorder and _PVFrameSourceGroup if PV stream is enabled;
	  // however, those initializations help simplify the code at stop/start time
	  if (enablePV)
	  {
		  _PVFrameRecorder->Enable(HoloLensForCV::SensorType::PhotoVideo);
		  // This call will enable the PhotoVideo MediaFrameSourceGroup
		  _PVFrameSourceGroup->EnableAll();

		  auto startPVFrameSourceGroupTask =
			  concurrency::create_task(
				  _PVFrameSourceGroup->StartAsync());

		  startPVFrameSourceGroupTask.then([&]() {
			  _PVFrameSourceGroupStarted = true;
		  });
	  }
  }


}
