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
    class MarkerRenderer
    {
    public:
        MarkerRenderer(
            _In_ const Graphics::DeviceResourcesPtr& deviceResources);

        void CreateDeviceDependentResources();

        void ReleaseDeviceDependentResources();

        void Update(
            _In_ const Graphics::StepTimer& timer);

        void Render();

        // Property accessors.
        void SetPosition(
            Windows::Foundation::Numerics::float3 pos)
        {
            _position = pos;
        }

        Windows::Foundation::Numerics::float3 GetPosition()
        {
            return _position;
        }

    private:
        // Cached pointer to device resources.
        Graphics::DeviceResourcesPtr _deviceResources;

        // The material we'll use to render this slate.
        std::unique_ptr<SlateMaterial> _slateMaterial;

        // Direct3D resources for the slate geometry.
        Microsoft::WRL::ComPtr<ID3D11Buffer> _vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> _indexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> _modelConstantBuffer;

        // System resources for the slate geometry.
        SlateModelConstantBuffer _modelConstantBufferData;
        uint32 _indexCount = 0;

        // Variables used with the rendering loop.
        bool _loadingComplete = false;

        Windows::Foundation::Numerics::float3 _position = { 0.f, 0.f, 0.f };
    };
}
