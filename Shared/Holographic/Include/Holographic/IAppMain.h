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
    interface IAppMain
    {
        virtual void OnHolographicSpaceChanged(
            Windows::Graphics::Holographic::HolographicSpace^ holographicSpace) = 0;

        virtual void OnSpatialInput(
            Windows::UI::Input::Spatial::SpatialInteractionSourceState^ pointerState) = 0;

        virtual void OnUpdate(
            _In_ _In_ Windows::Graphics::Holographic::HolographicFrame^ holographicFrame,
            _In_ const Graphics::StepTimer& timer) = 0;

        virtual void OnPreRender() = 0;

        virtual void OnRender() = 0;
    };
}
