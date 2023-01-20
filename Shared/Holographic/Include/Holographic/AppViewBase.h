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

namespace Holographic
{
    // IFrameworkView class. Connects the app with the Windows shell and handles application lifecycle events.
    ref class AppViewBase abstract
        : public Windows::ApplicationModel::Core::IFrameworkView
    {
    public:
        // IFrameworkView methods.
        virtual void Initialize(
            Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);

        virtual void SetWindow(
            Windows::UI::Core::CoreWindow^ window);

        virtual void Load(
            Platform::String^ entryPoint);

        virtual void Run();

        virtual void Uninitialize();

    protected:
        // Application lifecycle event handlers.
        void OnViewActivated(
            Windows::ApplicationModel::Core::CoreApplicationView^ sender,
            Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);

        void OnSuspending(
            Platform::Object^ sender,
            Windows::ApplicationModel::SuspendingEventArgs^ args);

        void OnResuming(
            Platform::Object^ sender,
            Platform::Object^ args);

        // Window event handlers.
        void OnVisibilityChanged(
            Windows::UI::Core::CoreWindow^ sender,
            Windows::UI::Core::VisibilityChangedEventArgs^ args);

        void OnWindowClosed(
            Windows::UI::Core::CoreWindow^ sender,
            Windows::UI::Core::CoreWindowEventArgs^ args);

        // CoreWindow input event handlers.
        void OnKeyPressed(
            Windows::UI::Core::CoreWindow^ sender,
            Windows::UI::Core::KeyEventArgs^ args);

    protected private:
        AppViewBase();

        virtual std::shared_ptr<AppMainBase> InitializeCore() = 0;

    protected private:
        std::shared_ptr<AppMainBase> _appMain;

        std::shared_ptr<Graphics::DeviceResources> _deviceResources;
        bool _windowClosed = false;
        bool _windowVisible = true;

        // The holographic space the app will use for rendering.
        Windows::Graphics::Holographic::HolographicSpace^ _holographicSpace = nullptr;
    };
}

