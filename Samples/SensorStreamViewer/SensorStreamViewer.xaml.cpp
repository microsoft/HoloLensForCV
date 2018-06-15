//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the Microsoft Public License.
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "pch.h"
#include "SensorImageControl.xaml.h"
#include "SensorStreamViewer.xaml.h"
#include "SensorStreamViewer.g.h"
#include "FrameRenderer.h"
#include <unordered_set>

using namespace SensorStreaming;

using namespace concurrency;
using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media::Capture;
using namespace Windows::Media::Capture::Frames;
using namespace Windows::UI::Xaml::Media::Imaging;



SensorStreamViewer::SensorStreamViewer()
{
    InitializeComponent();
    m_logger = ref new SimpleLogger(outputTextBlock);
    this->PlayStopButton->IsEnabled = false;
}

SensorStreaming::SensorStreamViewer::~SensorStreamViewer()
{
    CleanupMediaCaptureAsync();
}

void SensorStreaming::SensorStreamViewer::Start()
{
    LoadMediaSourceAsync();
}

void SensorStreaming::SensorStreamViewer::Stop()
{
    auto cleanUpTask = CleanupMediaCaptureAsync();
    cleanUpTask.wait();
}

void SensorStreamViewer::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e)
{
    LoadMediaSourceAsync();
}

void SensorStreamViewer::OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e)
{
    CleanupMediaCaptureAsync();
}

void SensorStreamViewer::NextButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    LoadMediaSourceAsync();
}

task<void> SensorStreamViewer::LoadMediaSourceAsync()
{
    return LoadMediaSourceWorkerAsync()
        .then([this]()
    {
    }, task_continuation_context::use_current());
}

int MFSourceIdToStreamId(const std::wstring& sourceIdStr)
{
    size_t start = sourceIdStr.find_first_of(L'#');
    size_t end = sourceIdStr.find_first_of(L'@');

    VERIFY(start != std::wstring::npos);
    VERIFY(end != std::wstring::npos);
    VERIFY(end > start);

    std::wstring idStr = sourceIdStr.substr(start + 1, end - start - 1);
    VERIFY(idStr.size() != 0);

    std::wstringstream wss(idStr);
    int id = 0;

    VERIFY(wss >> id);
    return id;
}

task<void> SensorStreamViewer::LoadMediaSourceWorkerAsync()
{
    return CleanupMediaCaptureAsync()
        .then([this]()
    {
        return create_task(MediaFrameSourceGroup::FindAllAsync());
    }, task_continuation_context::get_current_winrt_context())
        .then([this](IVectorView<MediaFrameSourceGroup^>^ allGroups)
    {
        if (allGroups->Size == 0)
        {
            m_logger->Log("No source groups found.");
            return task_from_result();
        }

        // Pick the first group.
        // TODO: Select by name
        m_selectedSourceGroupIndex = 0;
        MediaFrameSourceGroup^ selectedGroup = allGroups->GetAt(m_selectedSourceGroupIndex);

        m_logger->Log("Found " + allGroups->Size.ToString() + " groups and " +
            "selecting index [" + m_selectedSourceGroupIndex.ToString() + "] : " +
            selectedGroup->DisplayName);

        // Initialize MediaCapture with selected group.
        return TryInitializeMediaCaptureAsync(selectedGroup)
            .then([this, selectedGroup](bool initialized)
        {
            if (!initialized)
            {
                return CleanupMediaCaptureAsync();
            }

            // Set up frame readers, register event handlers and start streaming.
            auto startedKinds = std::make_shared<std::unordered_set<MediaFrameSourceKind>>();
            task<void> createReadersTask = task_from_result();

            for (IKeyValuePair<String^, MediaFrameSource^>^ kvp : m_mediaCapture->FrameSources)
            {
                std::lock_guard<std::mutex> lockGuard(m_volatileState.m_mutex);

                MediaFrameSource^ source = kvp->Value;
                MediaFrameSourceKind kind = source->Info->SourceKind;

                std::wstring sourceIdStr(source->Info->Id->Data());
                int id = MFSourceIdToStreamId(sourceIdStr);

                Platform::String^ sensorName(kind.ToString());

#if DEBUG_PRINT_PROPERTIES 
                DebugOutputAllProperties(source->Info->Properties);
#endif

                GetSensorName(source, sensorName);

                // Create the Sensor views
                SensorImageControl^ imageControl = ref new SensorImageControl(id, sensorName);

                m_volatileState.m_frameRenderers[id] = imageControl->GetRenderer();
                m_volatileState.m_frameRenderers[id]->SetSensorName(sensorName);
                
                Windows::UI::Core::CoreDispatcher^ uiThreadDispatcher =
                    Windows::ApplicationModel::Core::CoreApplication::MainView->CoreWindow->Dispatcher;

                uiThreadDispatcher->RunAsync(
                    Windows::UI::Core::CoreDispatcherPriority::Normal,
                    ref new Windows::UI::Core::DispatchedHandler(
                        [this, imageControl, id]()
                {
                    StreamGrid->Children->Append(imageControl);
                    imageControl->SetValue(Windows::UI::Xaml::Controls::Grid::RowProperty, id / 4);
                    imageControl->SetValue(Windows::UI::Xaml::Controls::Grid::ColumnProperty, id - ((id / 4) * 4));
#if 0
                    imageControl->PointerPressed += ref new Windows::UI::Xaml::Input::PointerEventHandler(this, &SensorStreaming::SensorStreamViewer::OnPointerPressed);
#endif
                    imageControl->Background = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Green);
                }));


                // Read all frames the first time
                if (m_volatileState.m_firstRunComplete && (id != m_volatileState.m_selectedStreamId))
                {
                    continue;
                }

                createReadersTask = createReadersTask.then([this, startedKinds, source, kind, id]()
                {
                    // Look for a format which the FrameRenderer can render.
                    String^ requestedSubtype = nullptr;
                    auto found = std::find_if(begin(source->SupportedFormats), end(source->SupportedFormats),
                        [&](MediaFrameFormat^ format)
                    {
                        requestedSubtype = FrameRenderer::GetSubtypeForFrameReader(kind, format);
                        return requestedSubtype != nullptr;
                    });
                    if (requestedSubtype == nullptr)
                    {
                        // No acceptable format was found. Ignore this source.
                        return task_from_result();
                    }

                    // Tell the source to use the format we can render.
                    return create_task(source->SetFormatAsync(*found))
                        .then([this, source, requestedSubtype]()
                    {
                        return create_task(m_mediaCapture->CreateFrameReaderAsync(source, requestedSubtype));
                    }, task_continuation_context::get_current_winrt_context())
                        .then([this, kind, source, id](MediaFrameReader^ frameReader)
                    {
                        std::lock_guard<std::mutex> lockGuard(m_volatileState.m_mutex);

                        // Add frame reader to the internal lookup
                        m_volatileState.m_FrameReadersToSourceIdMap[frameReader->GetHashCode()] = id;

                        EventRegistrationToken token = frameReader->FrameArrived +=
                            ref new TypedEventHandler<MediaFrameReader^, MediaFrameArrivedEventArgs^>(this, &SensorStreamViewer::FrameReader_FrameArrived);

                        // Keep track of created reader and event handler so it can be stopped later.
                        m_volatileState.m_readers.push_back(std::make_pair(frameReader, token));

                        m_logger->Log(kind.ToString() + " reader created");

                        return create_task(frameReader->StartAsync());
                    }, task_continuation_context::get_current_winrt_context())
                        .then([this, kind, startedKinds](MediaFrameReaderStartStatus status)
                    {
                        if (status == MediaFrameReaderStartStatus::Success)
                        {
                            m_logger->Log("Started " + kind.ToString() + " reader.");
                            startedKinds->insert(kind);
                        }
                        else
                        {
                            m_logger->Log("Unable to start " + kind.ToString() + "  reader. Error: " + status.ToString());
                        }
                    }, task_continuation_context::get_current_winrt_context());
                }, task_continuation_context::get_current_winrt_context());
            }
            // Run the loop and see if any sources were used.
            return createReadersTask.then([this, startedKinds, selectedGroup]()
            {
                if (startedKinds->size() == 0)
                {
                    m_logger->Log("No eligible sources in " + selectedGroup->DisplayName + ".");
                }
            }, task_continuation_context::get_current_winrt_context());
        }, task_continuation_context::get_current_winrt_context());
    }, task_continuation_context::get_current_winrt_context());
}

task<bool> SensorStreamViewer::TryInitializeMediaCaptureAsync(MediaFrameSourceGroup^ group)
{
    if (m_mediaCapture != nullptr)
    {
        // Already initialized.
        return task_from_result(true);
    }

    // Initialize mediacapture with the source group.
    m_mediaCapture = ref new MediaCapture();

    auto settings = ref new MediaCaptureInitializationSettings();

    // Select the source we will be reading from.
    settings->SourceGroup = group;

    // This media capture can share streaming with other apps.
    settings->SharingMode = MediaCaptureSharingMode::SharedReadOnly;

    // Only stream video and don't initialize audio capture devices.
    settings->StreamingCaptureMode = StreamingCaptureMode::Video;

    // Set to CPU to ensure frames always contain CPU SoftwareBitmap images,
    // instead of preferring GPU D3DSurface images.
    settings->MemoryPreference = MediaCaptureMemoryPreference::Cpu;

    // Only stream video and don't initialize audio capture devices.
    settings->StreamingCaptureMode = StreamingCaptureMode::Video;

    // Initialize MediaCapture with the specified group.
    // This must occur on the UI thread because some device families
    // (such as Xbox) will prompt the user to grant consent for the
    // app to access cameras.
    // This can raise an exception if the source no longer exists,
    // or if the source could not be initialized.
    return create_task(m_mediaCapture->InitializeAsync(settings))
        .then([this](task<void> initializeMediaCaptureTask)
    {
        try
        {
            // Get the result of the initialization. This call will throw if initialization failed
            // This pattern is docuemnted at https://msdn.microsoft.com/en-us/library/dd997692.aspx
            initializeMediaCaptureTask.get();
            m_logger->Log("MediaCapture is successfully initialized in shared mode.");
            return true;
        }
        catch (Exception^ exception)
        {
            m_logger->Log("Failed to initialize media capture: " + exception->Message);
            return false;
        }
    });
}

/// Unregisters FrameArrived event handlers, stops and disposes frame readers
/// and disposes the MediaCapture object.
task<void> SensorStreamViewer::CleanupMediaCaptureAsync()
{
    task<void> cleanupTask = task_from_result();

    if (m_mediaCapture != nullptr)
    {
        std::lock_guard<std::mutex> lockGuard(m_volatileState.m_mutex);

        for (auto& readerAndToken : m_volatileState.m_readers)
        {
            MediaFrameReader^ reader = readerAndToken.first;
            EventRegistrationToken token = readerAndToken.second;

            reader->FrameArrived -= token;
            cleanupTask = cleanupTask && create_task(reader->StopAsync());
        }
        cleanupTask = cleanupTask.then([this] {
            m_logger->Log("Cleaning up MediaCapture...");
            m_volatileState.m_readers.clear();
            m_volatileState.m_frameRenderers.clear();
            m_volatileState.m_FrameReadersToSourceIdMap.clear();
            m_volatileState.m_frameCount.clear();
            m_mediaCapture = nullptr;
        });
    }
    return cleanupTask;
}

static Platform::String^ PropertyGuidToString(Platform::Guid& guid)
{
    if (guid == Platform::Guid(MFSampleExtension_DeviceTimestamp))
    {
        return L"MFSampleExtension_DeviceTimestamp";
    }
    else if (guid == Platform::Guid(MFSampleExtension_Spatial_CameraViewTransform))
    {
        return L"MFSampleExtension_Spatial_CameraViewTransform";
    }
    else if (guid == Platform::Guid(MFSampleExtension_Spatial_CameraCoordinateSystem))
    {
        return L"MFSampleExtension_Spatial_CameraCoordinateSystem";
    }
    else if (guid == Platform::Guid(MFSampleExtension_Spatial_CameraProjectionTransform))
    {
        return L"MFSampleExtension_Spatial_CameraProjectionTransform";
    }
    else if (guid == Platform::Guid(MFSampleExtension_DeviceReferenceSystemTime))
    {
        return L"MFSampleExtension_DeviceReferenceSystemTime";
    }
    else if (guid == Platform::Guid(MFSampleExtension_CameraExtrinsics))
    {
        return L"MFSampleExtension_CameraExtrinsics";
    }
    else if (guid == Platform::Guid(MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES))
    {
        return L"MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES";
    }
    else if (guid == Platform::Guid(MF_MT_USER_DATA))
    {
        return L"MF_MT_USER_DATA";
    }
    else
    {
        return guid.ToString();
    }
}

void SensorStreamViewer::DebugOutputAllProperties(Windows::Foundation::Collections::IMapView<Platform::Guid, Platform::Object^>^ properties)
{
    if (IsDebuggerPresent())
    {
        auto itr = properties->First();
        while (itr->HasCurrent)
        {
            auto current = itr->Current;

            auto keyString = PropertyGuidToString(current->Key);
            OutputDebugStringW(reinterpret_cast<const wchar_t*>(keyString->Data()));
            OutputDebugStringW(L"\n");
            itr->MoveNext();
        }
    }
}

void SensorStreamViewer::FrameReader_FrameArrived(MediaFrameReader^ sender, MediaFrameArrivedEventArgs^ args)
{
    // TryAcquireLatestFrame will return the latest frame that has not yet been acquired.
    // This can return null if there is no such frame, or if the reader is not in the
    // "Started" state. The latter can occur if a FrameArrived event was in flight
    // when the reader was stopped.
    if (sender == nullptr)
    {
        return;
    }

    if (MediaFrameReference^ frame = sender->TryAcquireLatestFrame())
    {
        if (frame != nullptr)
        {
            FrameRenderer^ frameRenderer = nullptr;

            {
                std::lock_guard<std::mutex> lockGuard(m_volatileState.m_mutex);

                // Find the corresponding source id
                VERIFY(m_volatileState.m_FrameReadersToSourceIdMap.count(sender->GetHashCode()) != 0);
                int sourceId = m_volatileState.m_FrameReadersToSourceIdMap[sender->GetHashCode()];

                frameRenderer = m_volatileState.m_frameRenderers[sourceId];
                m_volatileState.m_frameCount[sourceId]++;

                if (!m_volatileState.m_firstRunComplete)
                {
                    // first run is complete if all the streams have atleast one frame
                    bool allStreamsGotFrames = (m_volatileState.m_frameCount.size() == m_volatileState.m_frameRenderers.size());

                    m_volatileState.m_firstRunComplete = allStreamsGotFrames;

#if 0
                    if (allStreamsGotFrames)
                    {
                        m_volatileState.m_selectedStreamId = 1;
                        LoadMediaSourceAsync();
                    }
#endif
                }
            }

            if (frameRenderer)
            {
                frameRenderer->ProcessFrame(frame);
            }
        }
    }
}

bool SensorStreamViewer::GetSensorName(Windows::Media::Capture::Frames::MediaFrameSource^ source, Platform::String^& name)
{
    bool success = false;
    if (source->Info->Properties->HasKey(MF_MT_USER_DATA))
    {

        Platform::Object^ mfMtUserData =
            source->Info->Properties->Lookup(
                MF_MT_USER_DATA);

        Platform::Array<byte>^ sensorNameAsPlatformArray =
            safe_cast<Platform::IBoxArray<byte>^>(
                mfMtUserData)->Value;

        name = ref new Platform::String(
            reinterpret_cast<const wchar_t*>(
                sensorNameAsPlatformArray->Data));

        success = true;
    }
    return success;
}


void SensorStreaming::SensorStreamViewer::PlayStopButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    this->Stop();
}


void SensorStreaming::SensorStreamViewer::OnPointerPressed(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    auto imageControl = dynamic_cast<SensorImageControl^>(sender);
    if (imageControl != nullptr)
    {
        std::lock_guard<std::mutex> lockGuard(m_volatileState.m_mutex);
        if (imageControl->GetId() != m_volatileState.m_selectedStreamId)
        {
            m_volatileState.m_selectedStreamId = imageControl->GetId();
            LoadMediaSourceAsync();
        }
    }
}
