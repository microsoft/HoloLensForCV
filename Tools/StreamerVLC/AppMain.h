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
#include <collection.h>

//using namespace Platform;
using namespace Windows::ApplicationModel::Background;
using namespace Windows::Storage;


namespace StreamerVLC
{

	class AppMain : public Holographic::AppMainBase
	{
	public:
		AppMain(
			const std::shared_ptr<Graphics::DeviceResources>& deviceResources);

		// IDeviceNotify
		virtual void OnDeviceLost() override;

		virtual void OnDeviceRestored() override;

		// IAppMain
		virtual void OnHolographicSpaceChanged(
			_In_ Windows::Graphics::Holographic::HolographicSpace^ holographicSpace) override;

		virtual void OnSpatialInput(
			_In_ Windows::UI::Input::Spatial::SpatialInteractionSourceState^ pointerState) override;

		virtual void OnUpdate(
			_In_ Windows::Graphics::Holographic::HolographicFrame^ holographicFrame,
			_In_ const Graphics::StepTimer& stepTimer) override;

		virtual void OnPreRender() override;

		virtual void OnRender() override;

		virtual void LoadAppState() override;

		virtual void SaveAppState() override;

		Windows::ApplicationModel::Background::ApplicationTrigger^ _appTrigger;
		/*
				static void UpdateBackgroundTaskRegistrationStatus(String^ name, bool registered);
				static String^ ApplicationTriggerTaskProgress;
				static bool ApplicationTriggerTaskRegistered;
				static String^ ApplicationTriggerTaskResult;

				void AttachProgressAndCompletedHandlers(IBackgroundTaskRegistration^ task);
		  */
	private:

		// Initializes access to HoloLens sensors.
		 //void StartHoloLensMediaFrameSourceGroup();


		// Selected HoloLens media frame source group
		void StartHoloLensMediaFrameSourceGroup();
		HoloLensForCV::MediaFrameSourceGroupType _selectedHoloLensMediaFrameSourceGroupType;
		HoloLensForCV::MediaFrameSourceGroup^ _holoLensMediaFrameSourceGroup;
		bool _holoLensMediaFrameSourceGroupStarted;

		// HoloLens media frame server manager
		HoloLensForCV::SensorFrameStreamer^ _sensorFrameStreamer;
		HoloLensForCV::MultiFrameBuffer^ _multiFrameBuffer;

		// Camera preview
		std::unique_ptr<Rendering::SlateRenderer> _slateRenderer;
		Rendering::Texture2DPtr _cameraPreviewTexture;
		Windows::Foundation::DateTime _cameraPreviewTimestamp;

		boolean taskRegistered;

		void InitializeBackgroundStreamer();
		
		//BackgroundTaskRegistration^ RegisterBackgroundTask(String^ taskEntryPoint, String^ name, IBackgroundTrigger^ trigger, IBackgroundCondition^ condition, BackgroundTaskRegistrationGroup^ group);
		//void AttachProgressAndCompletedHandlers(Windows::ApplicationModel::Background::IBackgroundTaskRegistration^ task);
		//void RegisterBackgroundTask(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		//void UnregisterBackgroundTask(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		//void OnProgress(Windows::ApplicationModel::Background::BackgroundTaskRegistration^ task, Windows::ApplicationModel::Background::BackgroundTaskProgressEventArgs^ args);
		//void OnCompleted(Windows::ApplicationModel::Background::BackgroundTaskRegistration^ task, Windows::ApplicationModel::Background::BackgroundTaskCompletedEventArgs^ args);
		//void UpdateUI();
	
	};
}
