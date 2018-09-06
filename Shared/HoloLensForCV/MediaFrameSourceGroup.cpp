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
    static const Platform::Guid c_MF_MT_USER_DATA(
        0xb6bc765f, 0x4c3b, 0x40a4, 0xbd, 0x51, 0x25, 0x35, 0xb6, 0x6f, 0xe0, 0x9d);

    MediaFrameSourceGroup::MediaFrameSourceGroup(
        _In_ MediaFrameSourceGroupType mediaFrameSourceGroupType,
        _In_ SpatialPerception^ spatialPerception,
        _In_opt_ ISensorFrameSinkGroup^ optionalSensorFrameSinkGroup)
        : _mediaFrameSourceGroupType(mediaFrameSourceGroupType)
        , _spatialPerception(spatialPerception)
        , _optionalSensorFrameSinkGroup(optionalSensorFrameSinkGroup)
    {
    }

    void MediaFrameSourceGroup::EnableAll()
    {
        switch (_mediaFrameSourceGroupType)
        {
        case MediaFrameSourceGroupType::PhotoVideoCamera:
            Enable(SensorType::PhotoVideo);
            break;

#if ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS
        case MediaFrameSourceGroupType::HoloLensResearchModeSensors:
            Enable(SensorType::ShortThrowToFDepth);
            Enable(SensorType::ShortThrowToFReflectivity);
            Enable(SensorType::LongThrowToFDepth);
            Enable(SensorType::LongThrowToFReflectivity);
            Enable(SensorType::VisibleLightLeftLeft);
            Enable(SensorType::VisibleLightLeftFront);
            Enable(SensorType::VisibleLightRightFront);
            Enable(SensorType::VisibleLightRightRight);
            break;
#endif /* ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS */
        }
    }

    void MediaFrameSourceGroup::Enable(
        _In_ SensorType sensorType)
    {
        const int32_t sensorTypeAsIndex =
            (int32_t)sensorType;

        REQUIRES(
            0 <= sensorTypeAsIndex &&
            sensorTypeAsIndex < (int32_t)_enabledFrameReaders.size());

        _enabledFrameReaders[sensorTypeAsIndex] =
            true;
    }

    bool MediaFrameSourceGroup::IsEnabled(
        _In_ SensorType sensorType) const
    {
        const int32_t sensorTypeAsIndex =
            (int32_t)sensorType;

        REQUIRES(
            0 <= sensorTypeAsIndex &&
            sensorTypeAsIndex < (int32_t)_enabledFrameReaders.size());

        return _enabledFrameReaders[sensorTypeAsIndex];
    }

    Windows::Foundation::IAsyncAction^ MediaFrameSourceGroup::StartAsync()
    {
        return concurrency::create_async(
            [this]()
            {
                return InitializeMediaSourceWorkerAsync();
            });
    }

	Windows::Foundation::IAsyncAction^ MediaFrameSourceGroup::StopAsync()
	{
		return concurrency::create_async(
			[this]()
		{
			return CleanupMediaCaptureAsync();
		});
	}

    SensorFrame^ MediaFrameSourceGroup::GetLatestSensorFrame(
        SensorType sensorType)
    {
        const int32_t sensorTypeAsIndex =
            (int32_t)sensorType;

        REQUIRES(
            0 <= sensorTypeAsIndex &&
            sensorTypeAsIndex < (int32_t)_frameReaders.size());

        if (_frameReaders[sensorTypeAsIndex] == nullptr)
        {
            return nullptr;
        }

        return _frameReaders[sensorTypeAsIndex]->GetLatestSensorFrame();
    }

    Concurrency::task<void> MediaFrameSourceGroup::InitializeMediaSourceWorkerAsync()
    {
        return CleanupMediaCaptureAsync()
            .then([this]()
        {
            return Concurrency::create_task(
                Windows::Media::Capture::Frames::MediaFrameSourceGroup::FindAllAsync());

        }, Concurrency::task_continuation_context::get_current_winrt_context())
            .then([this](Windows::Foundation::Collections::IVectorView<Windows::Media::Capture::Frames::MediaFrameSourceGroup^>^ sourceGroups)
        {
            Windows::Media::Capture::Frames::MediaFrameSourceGroup^ selectedSourceGroup;

            for (Windows::Media::Capture::Frames::MediaFrameSourceGroup^ sourceGroup : sourceGroups)
            {
                const wchar_t* sourceGroupDisplayName =
                    sourceGroup->DisplayName->Data();

                //
                // Note: this is the display name of the media frame source group associated
                // with the photo/video camera (or, RGB camera) on HoloLens Development Edition.
                //
                const wchar_t* c_HoloLensDevelopmentEditionPhotoVideoSourceGroupDisplayName =
                    L"MN34150";

                if (MediaFrameSourceGroupType::PhotoVideoCamera == _mediaFrameSourceGroupType &&
                    (0 == wcscmp(c_HoloLensDevelopmentEditionPhotoVideoSourceGroupDisplayName, sourceGroupDisplayName)))
                {
#if DBG_ENABLE_INFORMATIONAL_LOGGING
                    dbg::trace(
                        L"MediaFrameSourceGroup::InitializeMediaSourceWorkerAsync: found the photo-video media frame source group.");
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

                    selectedSourceGroup =
                        sourceGroup;

                    break;
                }
#if ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS
                else if (MediaFrameSourceGroupType::HoloLensResearchModeSensors == _mediaFrameSourceGroupType)
                {
#if DBG_ENABLE_INFORMATIONAL_LOGGING
                    dbg::trace(
                        L"MediaFrameSourceGroup::InitializeMediaSourceWorkerAsync: assuming '%s' is the HoloLens Sensor Streaming media frame source group.",
                        sourceGroupDisplayName);
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

                    selectedSourceGroup =
                        sourceGroup;

                    break;
                }
#endif /* ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS */
            }

            if (nullptr == selectedSourceGroup)
            {
#if DBG_ENABLE_INFORMATIONAL_LOGGING
                dbg::trace(
                    L"MediaFrameSourceGroup::InitializeMediaSourceWorkerAsync: selected media frame source group not found.");
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

                return Concurrency::task_from_result();
            }

            //
            // Initialize MediaCapture with the selected group.
            //
            return TryInitializeMediaCaptureAsync(selectedSourceGroup)
                .then([this, selectedSourceGroup](bool initialized)
            {
                if (!initialized)
                {
                    return CleanupMediaCaptureAsync();
                }

                //
                // Set up frame readers, register event handlers and start streaming.
                //
                auto startedSensors =
                    std::make_shared<std::unordered_set<SensorType, SensorTypeHash>>();

                Concurrency::task<void> createReadersTask =
                    Concurrency::task_from_result();

#if DBG_ENABLE_INFORMATIONAL_LOGGING
                dbg::trace(
                    L"MediaFrameSourceGroup::InitializeMediaSourceWorkerAsync: selected group has %i media frame sources",
                    _mediaCapture->FrameSources->Size);
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

                for (Windows::Foundation::Collections::IKeyValuePair<Platform::String^, Windows::Media::Capture::Frames::MediaFrameSource^>^ kvp : _mediaCapture->FrameSources)
                {
                    Windows::Media::Capture::Frames::MediaFrameSource^ source =
                        kvp->Value;

                    createReadersTask = createReadersTask.then([this, startedSensors, source]()
                    {
                        SensorType sensorType =
                            GetSensorType(
                                source);

                        if (SensorType::Undefined == sensorType)
                        {
                            //
                            // We couldn't map the source to a Research Mode sensor type. Ignore this source.
                            //
#if DBG_ENABLE_INFORMATIONAL_LOGGING
                            dbg::trace(
                                L"MediaFrameSourceGroup::InitializeMediaSourceWorkerAsync: could not map the media frame source to a Research Mode sensor type!");
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

                            return Concurrency::task_from_result();
                        }
                        else if (startedSensors->find(sensorType) != startedSensors->end())
                        {
                            //
                            // We couldn't map the source to a Research Mode sensor type. Ignore this source.
                            //
#if DBG_ENABLE_INFORMATIONAL_LOGGING
                            dbg::trace(
                                L"MediaFrameSourceGroup::InitializeMediaSourceWorkerAsync: sensor type %s has already been initialized!",
                                sensorType.ToString());
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

                            return Concurrency::task_from_result();
                        }
                        else if (!IsEnabled(sensorType))
                        {
                            //
                            // The sensor type was not explicitly enabled by user. Ignore this source.
                            //
#if DBG_ENABLE_INFORMATIONAL_LOGGING
                            dbg::trace(
                                L"MediaFrameSourceGroup::InitializeMediaSourceWorkerAsync: sensor type %s has not been enabled!",
                                sensorType.ToString());
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

                            return Concurrency::task_from_result();
                        }

                        //
                        // Look for a format which the FrameRenderer can render.
                        //
                        Platform::String^ requestedSubtype =
                            nullptr;

                        auto found =
                            std::find_if(
                                begin(source->SupportedFormats),
                                end(source->SupportedFormats),
                                [&](Windows::Media::Capture::Frames::MediaFrameFormat^ format)
                            {
                                requestedSubtype =
                                    GetSubtypeForFrameReader(
                                        source->Info->SourceKind,
                                        format);

                                return requestedSubtype != nullptr;
                            });

                        if (requestedSubtype == nullptr)
                        {
                            //
                            // No acceptable format was found. Ignore this source.
                            //
                            return Concurrency::task_from_result();
                        }

                        //
                        // Tell the source to use the format we can render.
                        //
                        return Concurrency::create_task(
                            source->SetFormatAsync(*found))
                            .then([this, source, requestedSubtype]()
                        {
                            return Concurrency::create_task(
                                _mediaCapture->CreateFrameReaderAsync(
                                    source,
                                    requestedSubtype));

                        }, Concurrency::task_continuation_context::get_current_winrt_context())
                            .then([this, sensorType](Windows::Media::Capture::Frames::MediaFrameReader^ frameReader)
                        {
                            ISensorFrameSink^ optionalSensorFrameSink =
                                nullptr != _optionalSensorFrameSinkGroup
                                ? _optionalSensorFrameSinkGroup->GetSensorFrameSink(
                                    sensorType)
                                : nullptr;

                            MediaFrameReaderContext^ frameReaderContext =
                                ref new MediaFrameReaderContext(
                                    sensorType,
                                    _spatialPerception,
                                    optionalSensorFrameSink);

                            _frameReaders[(int32_t)sensorType] =
                                frameReaderContext;

                            Windows::Foundation::EventRegistrationToken token =
                                frameReader->FrameArrived +=
                                    ref new Windows::Foundation::TypedEventHandler<
                                        Windows::Media::Capture::Frames::MediaFrameReader^,
                                        Windows::Media::Capture::Frames::MediaFrameArrivedEventArgs^>(
                                            frameReaderContext,
                                            &MediaFrameReaderContext::FrameArrived);

                            //
                            // Keep track of created reader and event handler so it can be stopped later.
                            //
                            _frameEventRegistrations.push_back(
                                std::make_pair(
                                    frameReader,
                                    token));

#if DBG_ENABLE_INFORMATIONAL_LOGGING
                            dbg::trace(
                                L"MediaFrameSourceGroup::InitializeMediaSourceWorkerAsync: created the '%s' frame reader",
                                sensorType.ToString()->Data());
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

                            return Concurrency::create_task(
                                frameReader->StartAsync());

                        }, Concurrency::task_continuation_context::get_current_winrt_context())
                            .then([this, sensorType, startedSensors](Windows::Media::Capture::Frames::MediaFrameReaderStartStatus status)
                        {
                            if (status == Windows::Media::Capture::Frames::MediaFrameReaderStartStatus::Success)
                            {
#if DBG_ENABLE_INFORMATIONAL_LOGGING
                                dbg::trace(
                                    L"MediaFrameSourceGroup::InitializeMediaSourceWorkerAsync: started the '%s' frame reader",
                                    sensorType.ToString()->Data());
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

                                startedSensors->insert(
                                    sensorType);
                            }
                            else
                            {
#if DBG_ENABLE_ERROR_LOGGING
                                dbg::trace(
                                    L"MediaFrameSourceGroup::InitializeMediaSourceWorkerAsync: unable to start the '%s' frame reader. Error: %s",
                                    sensorType.ToString()->Data(),
                                    status.ToString()->Data());
#endif /* DBG_ENABLE_ERROR_LOGGING */
                            }

                        }, Concurrency::task_continuation_context::get_current_winrt_context());

                    }, Concurrency::task_continuation_context::get_current_winrt_context());
                }

                //
                // Run the loop and see if any sources were used.
                //
                return createReadersTask.then([this, startedSensors, selectedSourceGroup]()
                {
                    if (startedSensors->size() == 0)
                    {
#if DBG_ENABLE_INFORMATIONAL_LOGGING
                        dbg::trace(
                            L"MediaFrameSourceGroup::InitializeMediaSourceWorkerAsync: no eligible sources in '%s'",
                            selectedSourceGroup->DisplayName->Data());
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */
                    }

                }, Concurrency::task_continuation_context::get_current_winrt_context());

            }, Concurrency::task_continuation_context::get_current_winrt_context());

        }, Concurrency::task_continuation_context::get_current_winrt_context());
    }

    SensorType MediaFrameSourceGroup::GetSensorType(
        Windows::Media::Capture::Frames::MediaFrameSource^ source)
    {
        //
        // First check if the request is concerning the PV camera.
        //
        if (MediaFrameSourceGroupType::PhotoVideoCamera == _mediaFrameSourceGroupType)
        {
#if DBG_ENABLE_INFORMATIONAL_LOGGING
            dbg::trace(
                L"MediaFrameSourceGroup::GetSensorType:: assuming SensorType::PhotoVideo per _mediaFrameSourceGroupType check (source id is '%s')",
                source->Info->Id->Data());
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

            return SensorType::PhotoVideo;
        }

        //
        // The sensor streaming DMFT exposes the sensor names through the MF_MT_USER_DATA
        // property (GUID for that property is {b6bc765f-4c3b-40a4-bd51-2535b66fe09d}).
        //
        if (!source->Info->Properties->HasKey(c_MF_MT_USER_DATA))
        {
#if DBG_ENABLE_INFORMATIONAL_LOGGING
            dbg::trace(
                L"MediaFrameSourceGroup::GetSensorType:: assuming SensorType::Undefined given missing MF_MT_USER_DATA");
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

            return SensorType::Undefined;
        }

        Platform::Object^ mfMtUserData =
            source->Info->Properties->Lookup(
                c_MF_MT_USER_DATA);

        Platform::Array<byte>^ sensorNameAsPlatformArray =
            safe_cast<Platform::IBoxArray<byte>^>(
                mfMtUserData)->Value;

        const wchar_t* sensorName =
            reinterpret_cast<wchar_t*>(
                sensorNameAsPlatformArray->Data);

#if DBG_ENABLE_INFORMATIONAL_LOGGING
        dbg::trace(
            L"MediaFrameSourceGroup::GetSensorType:: found sensor name '%s' in MF_MT_USER_DATA (blob has %i bytes)",
            sensorName,
            sensorNameAsPlatformArray->Length);
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

#if ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS
        if (0 == wcscmp(sensorName, L"Short Throw ToF Depth"))
        {
            return SensorType::ShortThrowToFDepth;
        }
        else if (0 == wcscmp(sensorName, L"Short Throw ToF Reflectivity"))
        {
            return SensorType::ShortThrowToFReflectivity;
        }
        if (0 == wcscmp(sensorName, L"Long Throw ToF Depth"))
        {
            return SensorType::LongThrowToFDepth;
        }
        else if (0 == wcscmp(sensorName, L"Long Throw ToF Reflectivity"))
        {
            return SensorType::LongThrowToFReflectivity;
        }
        else if (0 == wcscmp(sensorName, L"Visible Light Left-Left"))
        {
            return SensorType::VisibleLightLeftLeft;
        }
        else if (0 == wcscmp(sensorName, L"Visible Light Left-Front"))
        {
            return SensorType::VisibleLightLeftFront;
        }
        else if (0 == wcscmp(sensorName, L"Visible Light Right-Front"))
        {
            return SensorType::VisibleLightRightFront;
        }
        else if (0 == wcscmp(sensorName, L"Visible Light Right-Right"))
        {
            return SensorType::VisibleLightRightRight;
        }
        else
#endif /* ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS */
        {
#if DBG_ENABLE_INFORMATIONAL_LOGGING
            dbg::trace(
                L"MediaFrameSourceGroup::GetSensorType:: could not match sensor name to SensorType enumeration");
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

            return SensorType::Undefined;
        }
    }

    Platform::String^ MediaFrameSourceGroup::GetSubtypeForFrameReader(
        Windows::Media::Capture::Frames::MediaFrameSourceKind kind,
        Windows::Media::Capture::Frames::MediaFrameFormat^ format)
    {
        //
        // Note that media encoding subtypes may differ in case.
        // https://docs.microsoft.com/en-us/uwp/api/Windows.Media.MediaProperties.MediaEncodingSubtypes
        //
        switch (kind)
        {
        case Windows::Media::Capture::Frames::MediaFrameSourceKind::Color:
#if DBG_ENABLE_INFORMATIONAL_LOGGING
            dbg::trace(
                L"MediaFrameSourceGroup::GetSubtypeForFrameReader: evaluating MediaFrameSourceKind::Color with format %s-%s @%i/%iHz",
                format->MajorType->Data(),
                format->Subtype->Data(),
                format->FrameRate->Numerator,
                format->FrameRate->Denominator);
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

            //
            // For color sources, we accept anything and request that it be converted to Bgra8.
            //
            return Windows::Media::MediaProperties::MediaEncodingSubtypes::Bgra8;

#if ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS
        case Windows::Media::Capture::Frames::MediaFrameSourceKind::Depth:
        {
            if (MediaFrameSourceGroupType::HoloLensResearchModeSensors == _mediaFrameSourceGroupType)
            {
                //
                // The only depth format we can render is D16.
                //
                dbg::trace(
                    L"MediaFrameSourceGroup::GetSubtypeForFrameReader: evaluating MediaFrameSourceKind::Depth with format %s-%s @%i/%iHz",
                    format->MajorType->Data(),
                    format->Subtype->Data(),
                    format->FrameRate->Numerator,
                    format->FrameRate->Denominator);

                const bool isD16 =
                    CompareStringOrdinal(
                        format->Subtype->Data(),
                        -1 /* cchCount1 */,
                        Windows::Media::MediaProperties::MediaEncodingSubtypes::D16->Data(),
                        -1 /* cchCount2 */,
                        TRUE /* bIgnoreCase */) == CSTR_EQUAL;

                return isD16 ? format->Subtype : nullptr;
            }
            else
            {
                return nullptr;
            }
        }

        case Windows::Media::Capture::Frames::MediaFrameSourceKind::Infrared:
        {
            if (MediaFrameSourceGroupType::HoloLensResearchModeSensors == _mediaFrameSourceGroupType)
            {
                //
                // The only infrared formats we can render are L8 and L16.
                //
                dbg::trace(
                    L"MediaFrameSourceGroup::GetSubtypeForFrameReader: evaluating MediaFrameSourceKind::Infrared with format %s-%s @%i/%iHz",
                    format->MajorType->Data(),
                    format->Subtype->Data(),
                    format->FrameRate->Numerator,
                    format->FrameRate->Denominator);

                const bool isL8 =
                    CompareStringOrdinal(
                        format->Subtype->Data(),
                        -1 /* cchCount1 */,
                        Windows::Media::MediaProperties::MediaEncodingSubtypes::L8->Data(),
                        -1 /* cchCount2 */,
                        TRUE /* bIgnoreCase */) == CSTR_EQUAL;

                const bool isL16 =
                    CompareStringOrdinal(
                        format->Subtype->Data(),
                        -1 /* cchCount1 */,
                        Windows::Media::MediaProperties::MediaEncodingSubtypes::L16->Data(),
                        -1 /* cchCount2 */,
                        TRUE /* bIgnoreCase */) == CSTR_EQUAL;

                return (isL8 || isL16) ? format->Subtype : nullptr;
            }
            else
            {
                return nullptr;
            }
        }
#endif /* ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS */

        default:
            //
            // No other source kinds are supported by this class.
            //
            return nullptr;
        }
    }

    Concurrency::task<bool> MediaFrameSourceGroup::TryInitializeMediaCaptureAsync(
        Windows::Media::Capture::Frames::MediaFrameSourceGroup^ group)
    {
        if (_mediaCapture != nullptr)
        {
            //
            // Already initialized.
            //
            return Concurrency::task_from_result(true);
        }

        //
        // Initialize media capture with the source group.
        //
        _mediaCapture =
            ref new Windows::Media::Capture::MediaCapture();

        auto settings =
            ref new Windows::Media::Capture::MediaCaptureInitializationSettings();

        //
        // Select the source we will be reading from.
        //
        settings->SourceGroup =
            group;

        //
        // This media capture can share streaming with other apps.
        //
        settings->SharingMode =
            Windows::Media::Capture::MediaCaptureSharingMode::SharedReadOnly;

        //
        // Only stream video and don't initialize audio capture devices.
        //
        settings->StreamingCaptureMode =
            Windows::Media::Capture::StreamingCaptureMode::Video;

        //
        // Set to CPU to ensure frames always contain CPU SoftwareBitmap images,
        // instead of preferring GPU D3DSurface images.
        //
        settings->MemoryPreference =
            Windows::Media::Capture::MediaCaptureMemoryPreference::Cpu;

        //
        // Only stream video and don't initialize audio capture devices.
        //
        settings->StreamingCaptureMode =
            Windows::Media::Capture::StreamingCaptureMode::Video;

        //
        // Initialize MediaCapture with the specified group.
        // This must occur on the UI thread because some device families
        // (such as Xbox) will prompt the user to grant consent for the
        // app to access cameras.
        // This can raise an exception if the source no longer exists,
        // or if the source could not be initialized.
        //
        return Concurrency::create_task(
            _mediaCapture->InitializeAsync(
                settings))
            .then([this](Concurrency::task<void> initializeMediaCaptureTask)
        {
            try
            {
                //
                // Get the result of the initialization. This call will throw if initialization failed
                // This pattern is documented at https://msdn.microsoft.com/en-us/library/dd997692.aspx
                //
                initializeMediaCaptureTask.get();

#if DBG_ENABLE_INFORMATIONAL_LOGGING
                dbg::trace(
                    L"MediaFrameSourceGroup::TryInitializeMediaCaptureAsync: MediaCapture is successfully initialized in shared mode.");
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */

                return true;
            }
            catch (Platform::Exception^ exception)
            {
#if DBG_ENABLE_ERROR_LOGGING
                dbg::trace(
                    L"MediaFrameSourceGroup::TryInitializeMediaCaptureAsync: failed to initialize media capture: %s",
                    exception->Message->Data());
#endif /* DBG_ENABLE_ERROR_LOGGING */

                return false;
            }
        });
    }

    Concurrency::task<void> MediaFrameSourceGroup::CleanupMediaCaptureAsync()
    {
        Concurrency::task<void> cleanupTask =
            Concurrency::task_from_result();

        if (_mediaCapture != nullptr)
        {
            for (auto& readerAndToken : _frameEventRegistrations)
            {
                Windows::Media::Capture::Frames::MediaFrameReader^ reader =
                    readerAndToken.first;

                Windows::Foundation::EventRegistrationToken token =
                    readerAndToken.second;

                reader->FrameArrived -= token;

                cleanupTask =
                    cleanupTask &&
                    Concurrency::create_task(
                        reader->StopAsync());
            }

            _frameEventRegistrations.clear();

            _mediaCapture = nullptr;
        }

        return cleanupTask;
    }
}
