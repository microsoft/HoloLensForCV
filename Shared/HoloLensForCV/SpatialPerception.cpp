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

namespace HoloLensForCV
{
    SpatialPerception::SpatialPerception()
    {
        //
        // Use the default SpatialLocator to track the motion of the device.
        //
        _spatialLocator =
            Windows::Perception::Spatial::SpatialLocator::GetDefault();

        //
        // Be able to respond to changes in the positional tracking state.
        //
        _locatabilityChangedToken =
            _spatialLocator->LocatabilityChanged +=
            ref new Windows::Foundation::TypedEventHandler<Windows::Perception::Spatial::SpatialLocator^, Platform::Object^>(
                this,
                &SpatialPerception::OnLocatabilityChanged);

        //
        // The simplest way to render world-locked holograms is to create a stationary reference frame
        // when the app is launched. This is roughly analogous to creating a "world" coordinate system
        // with the origin placed at the device's position as the app is launched.
        //
        _originFrameOfReference =
            _spatialLocator->CreateStationaryFrameOfReferenceAtCurrentLocation();

        //
        // Notes on spatial tracking APIs:
        // * Stationary reference frames are designed to provide a best-fit position relative to the
        //   overall space. Individual positions within that reference frame are allowed to drift slightly
        //   as the device learns more about the environment.
        // * When precise placement of individual holograms is required, a SpatialAnchor should be used to
        //   anchor the individual hologram to a position in the real world - for example, a point the user
        //   indicates to be of special interest. Anchor positions do not drift, but can be corrected; the
        //   anchor will use the corrected position starting in the next frame after the correction has
        //   occurred.
        //
    }

    void SpatialPerception::UnregisterHolographicEventHandlers()
    {
        if (nullptr != _spatialLocator)
        {
            _spatialLocator->LocatabilityChanged -=
                _locatabilityChangedToken;
        }
    }

    void SpatialPerception::OnLocatabilityChanged(
        _In_ Windows::Perception::Spatial::SpatialLocator^ sender,
        _In_ Platform::Object^ args)
    {
        switch (sender->Locatability)
        {
        case Windows::Perception::Spatial::SpatialLocatability::Unavailable:
        {
            // Holograms cannot be rendered.
#if DBG_ENABLE_INFORMATIONAL_LOGGING
            dbg::trace(
                L"SpatialPerception::OnLocatabilityChanged: warning positional tracking is %s!\n",
                sender->Locatability.ToString());
#endif /* DBG_ENABLE_INFORMATIONAL_LOGGING */
        }
        break;

        // In the following three cases, it is still possible to place holograms using a
        // SpatialLocatorAttachedFrameOfReference.
        case Windows::Perception::Spatial::SpatialLocatability::PositionalTrackingActivating:
            // The system is preparing to use positional tracking.

        case Windows::Perception::Spatial::SpatialLocatability::OrientationOnly:
            // Positional tracking has not been activated.

        case Windows::Perception::Spatial::SpatialLocatability::PositionalTrackingInhibited:
            // Positional tracking is temporarily inhibited. User action may be required
            // in order to restore positional tracking.
            break;

        case Windows::Perception::Spatial::SpatialLocatability::PositionalTrackingActive:
            // Positional tracking is active. World-locked content can be rendered.
            break;
        }
    }

    Windows::Perception::Spatial::SpatialLocator^ SpatialPerception::GetSpatialLocator()
    {
        return _spatialLocator;
    }

    Windows::Perception::Spatial::SpatialStationaryFrameOfReference^ SpatialPerception::GetOriginFrameOfReference()
    {
        return _originFrameOfReference;
    }

    Windows::Perception::PerceptionTimestamp^ SpatialPerception::CreatePerceptionTimestamp(
        _In_ Windows::Foundation::DateTime timestamp)
    {
        return Windows::Perception::PerceptionTimestampHelper::FromHistoricalTargetTime(
            timestamp);
    }
}
