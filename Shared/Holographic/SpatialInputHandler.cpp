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

namespace Holographic
{
    // Creates and initializes a GestureRecognizer that listens to a Person.
    SpatialInputHandler::SpatialInputHandler()
    {
        // The interaction manager provides an event that informs the app when
        // spatial interactions are detected.
        _interactionManager =
            Windows::UI::Input::Spatial::SpatialInteractionManager::GetForCurrentView();

        // Bind a handler to the SourcePressed event.
        _sourcePressedEventToken =
            _interactionManager->SourcePressed +=
                ref new Windows::Foundation::TypedEventHandler<
                    Windows::UI::Input::Spatial::SpatialInteractionManager^,
                    Windows::UI::Input::Spatial::SpatialInteractionSourceEventArgs^>(
                        std::bind(
                            &SpatialInputHandler::OnSourcePressed,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2));
    }

    SpatialInputHandler::~SpatialInputHandler()
    {
        // Unregister our handler for the OnSourcePressed event.
        _interactionManager->SourcePressed -=
            _sourcePressedEventToken;
    }

    // Checks if the user performed an input gesture since the last call to this method.
    // Allows the main update loop to check for asynchronous changes to the user
    // input state.
    Windows::UI::Input::Spatial::SpatialInteractionSourceState^ SpatialInputHandler::CheckForInput()
    {
        Windows::UI::Input::Spatial::SpatialInteractionSourceState^ sourceState =
            _sourceState;

        _sourceState = nullptr;

        return sourceState;
    }

    void SpatialInputHandler::OnSourcePressed(
        Windows::UI::Input::Spatial::SpatialInteractionManager^ sender,
        Windows::UI::Input::Spatial::SpatialInteractionSourceEventArgs^ args)
    {
        _sourceState = args->State;
    }
}
