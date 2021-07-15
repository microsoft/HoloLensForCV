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

namespace HoloLensForCV
{
    //
    // Responsible for maintaining an origin frame of reference common between
    // the recording/streaming code and the application.
    // 
    public ref class SpatialPerception sealed
    {
    public:
        SpatialPerception();

        void UnregisterHolographicEventHandlers();

        Windows::Perception::Spatial::SpatialLocator^ GetSpatialLocator();

        Windows::Perception::Spatial::SpatialStationaryFrameOfReference^ GetOriginFrameOfReference();

        Windows::Perception::PerceptionTimestamp^ CreatePerceptionTimestamp(
            _In_ Windows::Foundation::DateTime timestamp);

    private:
        // Used to notify the app when the positional tracking state changes.
        void OnLocatabilityChanged(
            _In_ Windows::Perception::Spatial::SpatialLocator^ sender,
            _In_ Platform::Object^ args);

    private:
        // SpatialLocator that is attached to the primary camera.
        Windows::Perception::Spatial::SpatialLocator^ _spatialLocator;

        // A reference frame attached to the holographic camera.
        Windows::Perception::Spatial::SpatialStationaryFrameOfReference^ _originFrameOfReference;

        // Event registration tokens.
        Windows::Foundation::EventRegistrationToken _locatabilityChangedToken;
    };
}
