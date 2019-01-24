#include "pch.h"
#include "ArUcoMarkerDetector.h"

using namespace HoloLensForCV;
using namespace Platform;
using namespace UnityArUcoMarkerDetectorPlugin;
using namespace Windows::Foundation::Numerics;
using namespace Windows::Perception::Spatial;

void DebugOutputMatrix(std::wstring prompt, float4x4 transform)
{
    prompt += std::to_wstring(transform.m11) + L", ";
    prompt += std::to_wstring(transform.m12) + L", ";
    prompt += std::to_wstring(transform.m13) + L", ";
    prompt += std::to_wstring(transform.m14) + L", ";
    prompt += std::to_wstring(transform.m21) + L", ";
    prompt += std::to_wstring(transform.m22) + L", ";
    prompt += std::to_wstring(transform.m23) + L", ";
    prompt += std::to_wstring(transform.m24) + L", ";
    prompt += std::to_wstring(transform.m31) + L", ";
    prompt += std::to_wstring(transform.m32) + L", ";
    prompt += std::to_wstring(transform.m33) + L", ";
    prompt += std::to_wstring(transform.m34) + L", ";
    prompt += std::to_wstring(transform.m41) + L", ";
    prompt += std::to_wstring(transform.m42) + L", ";
    prompt += std::to_wstring(transform.m43) + L", ";
    prompt += std::to_wstring(transform.m44);
    dbg::trace(prompt.data());
}

std::map<int32_t, DetectedMarker> DetectArUcoMarkers(
    SpatialCoordinateSystem^ unityCoordinateSystem,
    HoloLensForCV::SensorFrame^ frame,
    int arucoDictionaryId,
    float markerSize)
{
    std::map<int32_t, DetectedMarker> detectedMarkers;

    cv::Ptr<cv::aruco::DetectorParameters> arucoDetectorParameters =
        cv::aruco::DetectorParameters::create();

    cv::Ptr<cv::aruco::Dictionary> arucoDictionary =
        cv::aruco::getPredefinedDictionary(arucoDictionaryId);

    std::vector<std::vector<cv::Point2f>> arucoMarkers, arucoRejectedCandidates;
    std::vector<int32_t> arucoMarkerIds;

    cv::Mat wrappedImage;

    // Note: Using wrapper for photo video, it appears there is a different wrapper for research mode cameras
    rmcv::WrapHoloLensSensorFrameWithCvMat(
        frame,
        wrappedImage);

    // Aruco detection does not support an image with alpha values, so convert to grayscale
    cv::Mat grayMat;
    cv::cvtColor(wrappedImage, grayMat, CV_BGRA2GRAY);

    dbg::trace(L"Starting detection algorithm");

    cv::aruco::detectMarkers(
        grayMat,
        arucoDictionary,
        arucoMarkers,
        arucoMarkerIds,
        arucoDetectorParameters,
        arucoRejectedCandidates);

    dbg::trace(
        L"Completed marker detection: %i ids founds",
        arucoMarkerIds.size());

    if (!arucoMarkerIds.empty())
    {
        IBox<float4x4>^ cameraToUnityReference =
            frame->FrameCoordinateSystem->TryGetTransformTo(unityCoordinateSystem);
        if (!cameraToUnityReference)
        {
            dbg::trace(L"Failed to obtain transform to unity coordinate space");
            throw ref new FailureException();
        }

        auto cameraToUnity = cameraToUnityReference->Value;

        float4x4 viewToCamera;
        invert(frame->CameraViewTransform, &viewToCamera);
        auto viewToCameraMat = DirectX::XMLoadFloat4x4(&viewToCamera);

        // Winrt uses right-handed row-vector representation for transforms
        // Unity uses left-handed column vector representation for transforms
        // Therefore, we convert our original winrt 'transform' to the 'transformUnity' by transposing and flipping z values
        float4x4 transform = viewToCamera * cameraToUnity;
        float4x4 transformUnity = transpose(transform);
        transformUnity.m31 *= -1.0f;
        transformUnity.m32 *= -1.0f;
        transformUnity.m33 *= -1.0f;
        transformUnity.m34 *= -1.0f;

        cv::Mat cameraMatrix(3, 3, CV_64F, cv::Scalar(0));
        cameraMatrix.at<double>(0, 0) = frame->CoreCameraIntrinsics->FocalLength.x;
        cameraMatrix.at<double>(0, 2) = frame->CoreCameraIntrinsics->PrincipalPoint.x;
        cameraMatrix.at<double>(1, 1) = frame->CoreCameraIntrinsics->FocalLength.y;
        cameraMatrix.at<double>(1, 2) = frame->CoreCameraIntrinsics->PrincipalPoint.y;
        cameraMatrix.at<double>(2, 2) = 1.0;

        std::wstring camMatStr = L"Camera Matrix: ";
        for (int m = 0; m < cameraMatrix.rows; m++)
        {
            for (int n = 0; n < cameraMatrix.cols; n++)
            {
                auto value = cameraMatrix.at<double>(m, n);
                camMatStr += std::to_wstring(value) + L", ";
            }
        }
        dbg::trace(camMatStr.data());

        cv::Mat distortionCoefficientsMatrix(1, 5, CV_64F);
        distortionCoefficientsMatrix.at<double>(0, 0) = frame->CoreCameraIntrinsics->RadialDistortion.x;
        distortionCoefficientsMatrix.at<double>(0, 1) = frame->CoreCameraIntrinsics->RadialDistortion.y;
        distortionCoefficientsMatrix.at<double>(0, 2) = frame->CoreCameraIntrinsics->TangentialDistortion.x;
        distortionCoefficientsMatrix.at<double>(0, 3) = frame->CoreCameraIntrinsics->TangentialDistortion.y;
        distortionCoefficientsMatrix.at<double>(0, 4) = frame->CoreCameraIntrinsics->RadialDistortion.z;

        std::wstring distMatStr = L"Distortion Vector: ";
        for (int n = 0; n < 5; n++)
        {
            auto value = distortionCoefficientsMatrix.at<double>(0, n);
            distMatStr += std::to_wstring(value) + L", ";
        }
        dbg::trace(distMatStr.data());

        std::vector<cv::Vec3d> rotationVecs;
        std::vector<cv::Vec3d> translationVecs;
        cv::aruco::estimatePoseSingleMarkers(arucoMarkers, markerSize, cameraMatrix, distortionCoefficientsMatrix, rotationVecs, translationVecs);

        for (size_t i = 0; i < arucoMarkerIds.size(); i++)
        {
            auto id = arucoMarkerIds[i];

            auto posText = L"OpenCV Marker Position: " + std::to_wstring(translationVecs[i][0]) + L", " + std::to_wstring(translationVecs[i][1]) + L", " + std::to_wstring(translationVecs[i][2]);
            dbg::trace(posText.data());

            auto rotText = L"OpenCV Marker Rotation: " + std::to_wstring(rotationVecs[i][0]) + L", " + std::to_wstring(rotationVecs[i][1]) + L", " + std::to_wstring(rotationVecs[i][2]);
            dbg::trace(rotText.data());

            DetectedMarker marker;
            marker.markerId = id;
            marker.point = float3(translationVecs[i][0], translationVecs[i][1], translationVecs[i][2]);
            marker.rotation = float3(rotationVecs[i][0], rotationVecs[i][1], rotationVecs[i][2]);
            marker.cameraToWorldUnity = transformUnity;
            detectedMarkers[id] = marker;
        }
    }

    return detectedMarkers;
}

ArUcoMarkerDetector::ArUcoMarkerDetector()
{
}


ArUcoMarkerDetector::~ArUcoMarkerDetector()
{
}

void ArUcoMarkerDetector::Initialize(
    int* spatialCoordinateSystem,
    int arucoDictionaryId,
    float markerSize)
{
    _arucoDictionaryId = arucoDictionaryId;
    _markerSize = markerSize;

    _selectedHoloLensMediaFrameSourceGroupType = MediaFrameSourceGroupType::PhotoVideoCamera;

    if (spatialCoordinateSystem)
    {
        _unitySpatialCoordinateSystem = reinterpret_cast<SpatialCoordinateSystem^>(spatialCoordinateSystem);
    }
    else
    {
        dbg::trace(L"Creator failed to provide valid ISpatialCoordinateSystem");
        throw ref new InvalidArgumentException();
    }

    dbg::trace(L"Setting up SpatialPerception");
    _spatialPerception =
        ref new HoloLensForCV::SpatialPerception();

    dbg::trace(L"Creating a MultiFrameBuffer");
    _multiFrameBuffer =
        ref new HoloLensForCV::MultiFrameBuffer();

    dbg::trace(L"Creating a MediaFrameSourceGroup");
    _holoLensMediaFrameSourceGroup =
        ref new HoloLensForCV::MediaFrameSourceGroup(
            _selectedHoloLensMediaFrameSourceGroupType,
            _spatialPerception,
            _multiFrameBuffer);

    dbg::trace(L"Enabling the photo video camera for the MediaFrameSourceGroup");
    _holoLensMediaFrameSourceGroup->Enable(SensorType::PhotoVideo);

    dbg::trace(L"Starting the MediaFrameSourceGroup");
    concurrency::create_task(_holoLensMediaFrameSourceGroup->StartAsync()).then(
        [&]()
    {
        dbg::trace(L"MediaFrameSourceGroup has started");
        _holoLensMediaFrameSourceGroupStarted = true;
    });
}

void ArUcoMarkerDetector::Dispose()
{
    dbg::trace(L"Stopping the MediaFrameSourceGroup");
    if (_holoLensMediaFrameSourceGroupStarted &&
        _holoLensMediaFrameSourceGroup)
    {
        _holoLensMediaFrameSourceGroupStarted = false;
        concurrency::create_task(_holoLensMediaFrameSourceGroup->StopAsync()).then(
            [&]()
        {
            dbg::trace(L"MediaFrameSourceGroup has stopped");

            if (_spatialPerception)
            {
                _spatialPerception->UnregisterHolographicEventHandlers();
                _spatialPerception = nullptr;
            }

            _holoLensMediaFrameSourceGroup = nullptr;
            _multiFrameBuffer = nullptr;
            _unitySpatialCoordinateSystem = nullptr;
        });
    }
}

bool ArUcoMarkerDetector::DetectMarkers()
{
    if (!_holoLensMediaFrameSourceGroupStarted)
    {
        dbg::trace(L"MediaFrameSourceGroup not yet started");
        return false;
    }

    dbg::trace(L"Obtaining most recent PhotoVideo frame");
    auto frame = _multiFrameBuffer->GetLatestFrame(HoloLensForCV::SensorType::PhotoVideo);
    if (!frame)
    {
        dbg::trace(L"No frame was obtained");
        return false;
    }

    dbg::trace(L"Detecting markers");
    _observedMarkers = DetectArUcoMarkers(
        _unitySpatialCoordinateSystem,
        frame,
        _arucoDictionaryId,
        _markerSize);

    return true;
}

int ArUcoMarkerDetector::GetDetectedMarkersCount()
{
    return _observedMarkers.size();
}

bool ArUcoMarkerDetector::GetDetectedMarkerIds(unsigned int* detectedIds, int size)
{
    if (size != _observedMarkers.size())
    {
        dbg::trace(L"Provided int array isn't large enough");
        return false;
    }

    int count = 0;
    for (auto marker : _observedMarkers)
    {
        detectedIds[count] = marker.second.markerId;
        count++;
    }

    return true;
}

bool ArUcoMarkerDetector::GetDetectedMarkerPose(
    int detectedId,
    float* position,
    float* rotation,
    float* cameraToWorldUnity)
{
    if (_observedMarkers.find(detectedId) == _observedMarkers.end())
    {
        dbg::trace(L"Marker was not detected");
        return false;
    }

    position[0] = _observedMarkers.at(detectedId).point.x;
    position[1] = _observedMarkers.at(detectedId).point.y;
    position[2] = _observedMarkers.at(detectedId).point.z;

    // Note: This is a rodrigues vector (magnitude is equal to the angle in radians, normalized vector is the axis for rotation)
    // These are NOT euler angles
    rotation[0] = _observedMarkers.at(detectedId).rotation.x;
    rotation[1] = _observedMarkers.at(detectedId).rotation.y;
    rotation[2] = _observedMarkers.at(detectedId).rotation.z;

    // Note: This transform is equivalent to Unity's PhotoCaptureFrame.TryGetCameraToWorldMatrix output
    // During detection, it was reformatted to left-handed column vector format for use in Unity
    cameraToWorldUnity[0] = _observedMarkers.at(detectedId).cameraToWorldUnity.m11;
    cameraToWorldUnity[1] = _observedMarkers.at(detectedId).cameraToWorldUnity.m12;
    cameraToWorldUnity[2] = _observedMarkers.at(detectedId).cameraToWorldUnity.m13;
    cameraToWorldUnity[3] = _observedMarkers.at(detectedId).cameraToWorldUnity.m14;
    cameraToWorldUnity[4] = _observedMarkers.at(detectedId).cameraToWorldUnity.m21;
    cameraToWorldUnity[5] = _observedMarkers.at(detectedId).cameraToWorldUnity.m22;
    cameraToWorldUnity[6] = _observedMarkers.at(detectedId).cameraToWorldUnity.m23;
    cameraToWorldUnity[7] = _observedMarkers.at(detectedId).cameraToWorldUnity.m24;
    cameraToWorldUnity[8] = _observedMarkers.at(detectedId).cameraToWorldUnity.m31;
    cameraToWorldUnity[9] = _observedMarkers.at(detectedId).cameraToWorldUnity.m32;
    cameraToWorldUnity[10] = _observedMarkers.at(detectedId).cameraToWorldUnity.m33;
    cameraToWorldUnity[11] = _observedMarkers.at(detectedId).cameraToWorldUnity.m34;
    cameraToWorldUnity[12] = _observedMarkers.at(detectedId).cameraToWorldUnity.m41;
    cameraToWorldUnity[13] = _observedMarkers.at(detectedId).cameraToWorldUnity.m42;
    cameraToWorldUnity[14] = _observedMarkers.at(detectedId).cameraToWorldUnity.m43;
    cameraToWorldUnity[15] = _observedMarkers.at(detectedId).cameraToWorldUnity.m44;
    return true;
}