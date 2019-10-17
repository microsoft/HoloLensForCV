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
    class SlateMaterial
    {
    public:
        SlateMaterial(
            _In_ const Graphics::DeviceResourcesPtr& deviceResources);

        void CreateDeviceDependentResources();

        void ReleaseDeviceDependentResources();

        void Update(
            _In_ const Graphics::StepTimer& timer);

        void Bind();

    private:
        // Cached pointer to device resources.
        Graphics::DeviceResourcesPtr _deviceResources;

        // Direct3D resources for the slate geometry.
        Microsoft::WRL::ComPtr<ID3D11InputLayout> _inputLayout;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> _vertexShader;
        Microsoft::WRL::ComPtr<ID3D11GeometryShader> _geometryShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> _pixelShader;

        // Variables used with the rendering loop.
        bool _loadingComplete = false;

        // If the current D3D Device supports VPRT, we can avoid using a geometry
        // shader just to set the render target array index.
        bool _usingVprtShaders = false;
    };
}
