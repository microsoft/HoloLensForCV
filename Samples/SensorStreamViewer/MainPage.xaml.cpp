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
#include "SensorStreamViewer.xaml.h"

using namespace SensorStreaming;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::UI::Xaml::Interop;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

MainPage^ MainPage::Current = nullptr;

MainPage::MainPage()
{
    InitializeComponent();
    // This is a static public property that allows downstream pages to get a handle to the MainPage instance
    // in order to call methods that are in this class.
    MainPage::Current = this;
}

void SensorStreaming::MainPage::OnSuspending(Platform::Object^ , Windows::ApplicationModel::SuspendingEventArgs^ )
{
    auto ssvPage = dynamic_cast<SensorStreamViewer^>(this->ScenarioFrame->Content );
    if (ssvPage != nullptr)
    {
        ssvPage->Stop();
    }

}

void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
    // Clear the status block when changing scenarios
    NotifyUser("", NotifyType::StatusMessage);

    // Navigate to the selected scenario.
    TypeName scenarioType = { "SensorStreaming.SensorStreamViewer", TypeKind::Custom };
    ScenarioFrame->Navigate(scenarioType, this);
}

void MainPage::NotifyUser(String^ strMessage, NotifyType type)
{
    if (Dispatcher->HasThreadAccess)
    {
        UpdateStatus(strMessage, type);
    }
    else
    {
        Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([strMessage, type, this]()
        {
            UpdateStatus(strMessage, type);
        }));
    }
}

void MainPage::UpdateStatus(String^ strMessage, NotifyType type)
{
    switch (type)
    {
    case NotifyType::StatusMessage:
        StatusBorder->Background = ref new SolidColorBrush(Windows::UI::Colors::Green);
        break;
    case NotifyType::ErrorMessage:
        StatusBorder->Background = ref new SolidColorBrush(Windows::UI::Colors::Red);
        break;
    default:
        break;
    }

    StatusBlock->Text = strMessage;

    // Collapse the StatusBlock if it has no text to conserve real estate.
    if (StatusBlock->Text != "")
    {
        StatusBorder->Visibility = Windows::UI::Xaml::Visibility::Visible;
        StatusPanel->Visibility = Windows::UI::Xaml::Visibility::Visible;
    }
    else
    {
        StatusBorder->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        StatusPanel->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }
}

