#pragma once

namespace UnityArUcoMarkerDetectorPlugin
{
    struct DetectedMarker
    {
        int32_t markerId;

        Windows::Foundation::Numerics::float3 point;
        Windows::Foundation::Numerics::float3 rotation;
        Windows::Foundation::Numerics::float4x4 cameraToWorldUnity;

        // Image position
        int x;
        int y;
    };

    class ArUcoMarkerDetector
    {
    public:
        ArUcoMarkerDetector();
        ~ArUcoMarkerDetector();

        void Initialize(
            int* iSpatialCoordinateSystem,
            int arucoDictionaryId,
            float markerSize);
        void Dispose();
        bool DetectMarkers();
        int GetDetectedMarkersCount();
        bool GetDetectedMarkerIds(
            unsigned int* detectedIds,
            int size);
        bool GetDetectedMarkerPose(
            int detectedId,
            float* position,
            float* rotation,
            float* cameraToWorldUnity);

    private:
        int _arucoDictionaryId = cv::aruco::DICT_6X6_250;
        float _markerSize = 0.03f; // meters
        HoloLensForCV::MediaFrameSourceGroupType _selectedHoloLensMediaFrameSourceGroupType;
        HoloLensForCV::SpatialPerception^ _spatialPerception;
        HoloLensForCV::MultiFrameBuffer^ _multiFrameBuffer;
        HoloLensForCV::MediaFrameSourceGroup^ _holoLensMediaFrameSourceGroup;
        Windows::Perception::Spatial::SpatialCoordinateSystem^ _unitySpatialCoordinateSystem;
        bool _holoLensMediaFrameSourceGroupStarted = false;
        std::map<int, DetectedMarker> _observedMarkers;
    };
}


