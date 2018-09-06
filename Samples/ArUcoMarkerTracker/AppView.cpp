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
#include "AppView.h"

// The main function is only used to initialize our IFrameworkView class.
// Under most circumstances, you should not need to modify this function.
[Platform::MTAThread]
int main(
    Platform::Array<Platform::String^>^)
{
    ArUcoMarkerTracker::AppViewSource^ appViewSource =
        ref new ArUcoMarkerTracker::AppViewSource();

    Windows::ApplicationModel::Core::CoreApplication::Run(
        appViewSource);

    return 0;
}

namespace ArUcoMarkerTracker
{
    Windows::ApplicationModel::Core::IFrameworkView^ AppViewSource::CreateView()
    {
        return ref new AppView();
    }

    AppView::AppView()
    {
    }

    std::shared_ptr<Holographic::AppMainBase> AppView::InitializeCore()
    {
        return std::make_shared<AppMain>(
            _deviceResources);
    }
}
