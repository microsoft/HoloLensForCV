//
// SensorImage.xaml.cpp
// Implementation of the SensorImage class
//

#include "pch.h"
#include "SensorImageControl.xaml.h"

using namespace SensorStreaming;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The User Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234236

SensorImageControl::SensorImageControl(Platform::String ^ sensorName)
{
	InitializeComponent();
    m_renderer = ref new FrameRenderer(SensorImage);
    SensorName->Text = sensorName;
}



FrameRenderer^ SensorImageControl::GetRenderer()
{
    return m_renderer;
}
