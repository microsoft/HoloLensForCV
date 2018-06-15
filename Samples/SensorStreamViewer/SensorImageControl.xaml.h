//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the Microsoft Public License.
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

//
// SensorImage.xaml.h
// Declaration of the SensorImage class
//

#pragma once

#include "SensorImageControl.g.h"
#include "FrameRenderer.h"

namespace SensorStreaming
{
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class SensorImageControl sealed
	{
	public:
		SensorImageControl(int Id, Platform::String^ sensorName);

        FrameRenderer^ GetRenderer();

        int GetId();
    private:
        Windows::Media::Capture::Frames::MediaFrameSourceKind m_kind;
        FrameRenderer^ m_renderer;
        int m_id;
    };
}
