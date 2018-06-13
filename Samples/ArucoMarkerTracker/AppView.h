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

namespace ArucoMarkerTracker
{
    // IFrameworkView class. Connects the app with the Windows shell and handles application lifecycle events.
    ref class AppView sealed : public Holographic::AppViewBase
    {
    public:
        AppView();

    protected private:
        virtual std::shared_ptr<Holographic::AppMainBase> InitializeCore() override;
    };

    // The entry point for the app.
    ref class AppViewSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
    {
    public:
        virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
    };
}
