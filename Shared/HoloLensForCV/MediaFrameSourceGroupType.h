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

namespace HoloLensForCV
{
    public enum class MediaFrameSourceGroupType
    {
        PhotoVideoCamera,
#if ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS
        HoloLensResearchModeSensors,
#endif /* ENABLE_HOLOLENS_RESEARCH_MODE_SENSORS */
    };
}
