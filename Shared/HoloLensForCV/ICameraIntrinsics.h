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

#include <inspectable.h>

namespace SensorStreaming
{
    // {5DCC7829-9471-4D78-8C27-5B2B9A0EC5EF}
    // Type : IUnknown (SensorStreaming::ICameraIntrinsics)
    // Stores intrinsics of the captured frame
    EXTERN_GUID(MFSampleExtension_SensorStreaming_CameraIntrinsics, 0x5dcc7829, 0x9471, 0x4d78, 0x8c, 0x27, 0x5b, 0x2b, 0x9a, 0xe, 0xc5, 0xef);

    struct
    __declspec(uuid("9086e81c-0485-434d-918b-25924a877b09"))
    ICameraIntrinsics : IInspectable
    {
        virtual HRESULT __stdcall MapImagePointToCameraUnitPlane(_In_ float (&uv) [2],
                                                                 _Out_ float (&xy) [2]) = 0;
        
        virtual HRESULT __stdcall MapCameraSpaceToImagePoint(_In_ float(&xy)[2],
                                                             _Out_ float(&uv)[2]) = 0;
    };
}
