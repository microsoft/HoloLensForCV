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

namespace Rendering
{
    // Constant buffer used to send hologram position transform to the shader pipeline.
    struct SlateModelConstantBuffer
    {
        DirectX::XMFLOAT4X4 model;
    };

    // Assert that the constant buffer remains 16-byte aligned (best practice).
    static_assert(
        0 == (sizeof(SlateModelConstantBuffer) % (sizeof(float) * 4)),
        "Model constant buffer size must be 16-byte aligned (16 bytes is the length of four floats).");

    // Used to send per-vertex data to the vertex shader.
    struct VertexPositionColorTexture
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT3 color;
        DirectX::XMFLOAT2 tex;
    };
}
