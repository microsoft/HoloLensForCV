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
		SensorImageControl(Platform::String^ sensorName);

        FrameRenderer^ GetRenderer();

    private:
        Windows::Media::Capture::Frames::MediaFrameSourceKind m_kind;
        FrameRenderer^ m_renderer;

	};
}
