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

    private:
        // Initializes access to HoloLens sensors.
        void StartHoloLensMediaFrameSourceGroup();

    private:
        std::vector<std::shared_ptr<Rendering::SlateRenderer> >_slateRendererList;
        std::shared_ptr<Rendering::SlateRenderer> _currentSlateRenderer;

        // Selected HoloLens media frame source group
        HoloLensForCV::MediaFrameSourceGroupType _selectedHoloLensMediaFrameSourceGroupType;
        HoloLensForCV::MediaFrameSourceGroup^ _holoLensMediaFrameSourceGroup;
        bool _holoLensMediaFrameSourceGroupStarted;

        Windows::Foundation::DateTime _latestSelectedCameraTimestamp;

        cv::Mat _undistortMap1;
        cv::Mat _undistortMap2;
        bool _undistortMapsInitialized;

        cv::Mat _undistortedPVCameraImage;
        cv::Mat _resizedPVCameraImage;
        cv::Mat _blurredPVCameraImage;
        cv::Mat _cannyPVCameraImage;

        std::vector<Rendering::Texture2DPtr> _visualizationTextureList;
        Rendering::Texture2DPtr _currentVisualizationTexture;

        bool _isActiveRenderer;
    };
}
