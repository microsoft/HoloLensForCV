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
    //
    // Base class for basic holographic applications -- allowing samples and tools to
    // implement just the simplified IAppMain interface.
    //
    class AppMainBase : public Graphics::IDeviceNotify, public IAppMain
    {
    public:
        AppMainBase(
            const std::shared_ptr<Graphics::DeviceResources>& deviceResources);

        ~AppMainBase();

        // Sets the holographic space. This is our closest analogue to setting a new window
        // for the app.
        void SetHolographicSpace(
            Windows::Graphics::Holographic::HolographicSpace^ holographicSpace);

        // Starts the holographic frame and updates the content.
        Windows::Graphics::Holographic::HolographicFrame^ Update();

        // Renders holograms, including world-locked content.
        bool Render(
            Windows::Graphics::Holographic::HolographicFrame^ holographicFrame);

        // Handle saving and loading of app state owned by AppMain.
        virtual void SaveAppState();
        virtual void LoadAppState();

        // IDeviceNotify
        void OnDeviceLost() override;
        void OnDeviceRestored() override;

    private:
        // Asynchronously creates resources for new holographic cameras.
        void OnCameraAdded(
            Windows::Graphics::Holographic::HolographicSpace^ sender,
            Windows::Graphics::Holographic::HolographicSpaceCameraAddedEventArgs^ args);

        // Synchronously releases resources for holographic cameras that are no longer
        // attached to the system.
        void OnCameraRemoved(
            Windows::Graphics::Holographic::HolographicSpace^ sender,
            Windows::Graphics::Holographic::HolographicSpaceCameraRemovedEventArgs^ args);

        // Clears event registration state. Used when changing to a new HolographicSpace
        // and when tearing down AppMain.
        void UnregisterHolographicEventHandlers();

    protected:
        // Listens for the Pressed spatial input event.
        std::shared_ptr<SpatialInputHandler> _spatialInputHandler;

        // Cached pointer to device resources.
        Graphics::DeviceResourcesPtr _deviceResources;

        // Represents the holographic space around the user.
        Windows::Graphics::Holographic::HolographicSpace^ _holographicSpace;

        // Spatial perception object shared with the HoloLensForCV components.
        HoloLensForCV::SpatialPerception^ _spatialPerception;

        // Optional focus point.
        bool _hasFocusPoint{ false };
        Windows::Foundation::Numerics::float3 _optionalFocusPoint;

    private:
        // Render loop timer.
        Graphics::StepTimer _timer;

        // Event registration tokens.
        Windows::Foundation::EventRegistrationToken _cameraAddedToken;
        Windows::Foundation::EventRegistrationToken _cameraRemovedToken;
    };
}
