#include "pch.h"
#include "UnityArUcoMarkerDetectorPlugin.h"
#include "ArUcoMarkerDetector.h"

using namespace UnityArUcoMarkerDetectorPlugin;

ArUcoMarkerDetector* markerDetector = nullptr;

extern "C" __declspec(dllexport) bool __stdcall CheckPlugin()
{
    return true;
}

extern "C" __declspec(dllexport) void __stdcall Initialize(int* spatialCoordinateSystem, int arucoDictionaryId, float markerSize)
{
    if (!markerDetector)
    {
        markerDetector = new ArUcoMarkerDetector();
    }

    markerDetector->Initialize(spatialCoordinateSystem, arucoDictionaryId, markerSize);
}

extern "C" __declspec(dllexport) void __stdcall Dispose()
{
    if (markerDetector)
    {
        markerDetector->Dispose();
    }
}

extern "C" __declspec(dllexport) bool __stdcall DetectMarkers()
{
    if (markerDetector)
    {
        return markerDetector->DetectMarkers();
    }

    return false;
}

extern "C" __declspec(dllexport) int __stdcall GetDetectedMarkersCount()
{
    if (markerDetector)
    {
        return markerDetector->GetDetectedMarkersCount();
    }

    return -1;
}

extern "C" __declspec(dllexport) bool __stdcall GetDetectedMarkerIds(unsigned int* detectedIds, int size)
{
    if (markerDetector)
    {
        return markerDetector->GetDetectedMarkerIds(detectedIds, size);
    }

    return false;
}


extern "C" __declspec(dllexport) bool __stdcall GetDetectedMarkerPose(
    int detectedId,
    float* position,
    float* rotation,
    float* cameraToWorldUnity)
{
    if (markerDetector == NULL)
    {
        return false;
    }

    return markerDetector->GetDetectedMarkerPose(detectedId, position, rotation, cameraToWorldUnity);
}