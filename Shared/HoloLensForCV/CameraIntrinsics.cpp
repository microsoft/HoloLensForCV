#include "pch.h"
#include "CameraIntrinsics.h"

namespace HoloLensForCV
{
    CameraIntrinsics::CameraIntrinsics(
        Microsoft::WRL::ComPtr<SensorStreaming::ICameraIntrinsics> intrinsics,
        _In_ unsigned int imageWidth,
        _In_ unsigned int imageHeight) :
        _intrinsics(intrinsics)
    {
        ImageWidth = imageWidth;
        ImageHeight = imageHeight;

    }

    Windows::Foundation::Point CameraIntrinsics::MapImagePointToCameraUnitPlane(
        Windows::Foundation::Point pixel)
    {
        float uv[2] = { pixel.X, pixel.Y };
        float xy[2];
        HRESULT hr = _intrinsics->MapImagePointToCameraUnitPlane(uv, xy);

        if (FAILED(hr))
        {
            throw ref new Platform::COMException(hr, "Failed to map image point to the camera unit plane");
        }

        Windows::Foundation::Point unitPlanePoint = {xy[0], xy[1]};
        
        return unitPlanePoint;
    }
}
