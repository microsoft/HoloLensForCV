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

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Holographic;
using namespace Windows::UI::Core;

namespace Holographic
{
    AppViewBase::AppViewBase()
    {
    }

    // IFrameworkView methods

    // The first method called when the IFrameworkView is being created.
    // Use this method to subscribe for Windows shell events and to initialize your app.
    void AppViewBase::Initialize(
        CoreApplicationView^ applicationView)
    {
        applicationView->Activated +=
            ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &AppViewBase::OnViewActivated);

        // Register event handlers for app lifecycle.
        CoreApplication::Suspending +=
            ref new EventHandler<SuspendingEventArgs^>(this, &AppViewBase::OnSuspending);

        CoreApplication::Resuming +=
            ref new EventHandler<Platform::Object^>(this, &AppViewBase::OnResuming);

        // At this point we have access to the device and we can create device-dependent
        // resources.
        _deviceResources = std::make_shared<Graphics::DeviceResources>();

        _appMain = InitializeCore();
    }

    // Called when the CoreWindow object is created (or re-created).
    void AppViewBase::SetWindow(
        CoreWindow^ window)
    {
        // Register for keypress notifications.
        window->KeyDown +=
            ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &AppViewBase::OnKeyPressed);

        // Register for notification that the app window is being closed.
        window->Closed +=
            ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &AppViewBase::OnWindowClosed);

        // Register for notifications that the app window is losing focus.
        window->VisibilityChanged +=
            ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &AppViewBase::OnVisibilityChanged);

        // Create a holographic space for the core window for the current view.
        // Presenting holographic frames that are created by this holographic space will put
        // the app into exclusive mode.
        _holographicSpace = HolographicSpace::CreateForCoreWindow(window);

        // The DeviceResources class uses the preferred DXGI adapter ID from the holographic
        // space (when available) to create a Direct3D device. The HolographicSpace
        // uses this ID3D11Device to create and manage device-based resources such as
        // swap chains.
        _deviceResources->SetHolographicSpace(_holographicSpace);

        // The main class uses the holographic space for updates and rendering.
        _appMain->SetHolographicSpace(_holographicSpace);
    }

    // The Load method can be used to initialize scene resources or to load a
    // previously saved app state.
    void AppViewBase::Load(
        Platform::String^ entryPoint)
    {
    }

    // This method is called after the window becomes active. It oversees the
    // update, draw, and present loop, and it also oversees window message processing.
    void AppViewBase::Run()
    {
        while (!_windowClosed)
        {
            if (_windowVisible && (_holographicSpace != nullptr))
            {
                CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(
                    CoreProcessEventsOption::ProcessAllIfPresent);

                HolographicFrame^ holographicFrame =
                    _appMain->Update();

                if (_appMain->Render(
                    holographicFrame))
                {
                    // The holographic frame has an API that presents the swap chain for each
                    // holographic camera.
                    _deviceResources->Present(
                        holographicFrame);
                }
            }
            else
            {
                CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(
                    CoreProcessEventsOption::ProcessOneAndAllPending);
            }
        }
    }

    // Terminate events do not cause Uninitialize to be called. It will be called if your IFrameworkView
    // class is torn down while the app is in the foreground.
    // This method is not often used, but IFrameworkView requires it and it will be called for
    // holographic apps.
    void AppViewBase::Uninitialize()
    {
    }


    // Application lifecycle event handlers

    // Called when the app view is activated. Activates the app's CoreWindow.
    void AppViewBase::OnViewActivated(
        CoreApplicationView^ sender,
        IActivatedEventArgs^ args)
    {
        // Run() won't start until the CoreWindow is activated.
        sender->CoreWindow->Activate();
    }

    void AppViewBase::OnSuspending(
        Platform::Object^ sender,
        SuspendingEventArgs^ args)
    {
        // Save app state asynchronously after requesting a deferral. Holding a deferral
        // indicates that the application is busy performing suspending operations. Be
        // aware that a deferral may not be held indefinitely; after about five seconds,
        // the app will be forced to exit.
        SuspendingDeferral^ deferral =
            args->SuspendingOperation->GetDeferral();

        concurrency::create_task([this, deferral]()
        {
            _deviceResources->Trim();

            if (_appMain != nullptr)
            {
                _appMain->SaveAppState();
            }

            //
            // TODO: Insert code here to save your app state.
            //

            deferral->Complete();
        });
    }

    void AppViewBase::OnResuming(
        Platform::Object^ sender,
        Platform::Object^ args)
    {
        // Restore any data or state that was unloaded on suspend. By default, data
        // and state are persisted when resuming from suspend. Note that this event
        // does not occur if the app was previously terminated.

        if (_appMain != nullptr)
        {
            _appMain->LoadAppState();
        }

        //
        // TODO: Insert code here to load your app state.
        //
    }


    // Window event handlers

    void AppViewBase::OnVisibilityChanged(
        CoreWindow^ sender,
        VisibilityChangedEventArgs^ args)
    {
        _windowVisible =
            args->Visible;
    }

    void AppViewBase::OnWindowClosed(
        CoreWindow^ sender,
        CoreWindowEventArgs^ args)
    {
        _windowClosed =
            true;
    }


    // Input event handlers

    void AppViewBase::OnKeyPressed(
        CoreWindow^ sender,
        KeyEventArgs^ args)
    {
        //
        // TODO: Bluetooth keyboards are supported by HoloLens. You can use this method for
        //       keyboard input if you want to support it as an optional input method for
        //       your holographic app.
        //
    }
}
