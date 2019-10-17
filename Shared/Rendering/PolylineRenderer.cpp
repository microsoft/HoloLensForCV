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

#include "pch.h"

namespace Rendering
{
    // Loads vertex and pixel shaders from files and instantiates the cube geometry.
    PolylineRenderer::PolylineRenderer(
        const std::shared_ptr<Graphics::DeviceResources>& deviceResources)
        : _deviceResources(deviceResources)
    {
        CreateDeviceDependentResources();
    }

    // Called once per frame. Rotates the cube, and calculates and sets the model matrix
    // relative to the position transform indicated by hologramPositionTransform.
    void PolylineRenderer::Update(
        _In_ const Graphics::StepTimer& /* timer */)
    {
        // Position the marker.
        const auto modelTranslation =
            DirectX::XMMatrixTranslationFromVector(
                DirectX::XMLoadFloat3(&_position));

        // The view and projection matrices are provided by the system; they are associated
        // with holographic cameras, and updated on a per-camera basis.
        // Here, we provide the model transform for the sample hologram. The model transform
        // matrix is transposed to prepare it for the shader.
        XMStoreFloat4x4(
            &_modelConstantBufferData.model,
            DirectX::XMMatrixTranspose(
                modelTranslation));

        // Loading is asynchronous. Resources must be created before they can be updated.
        if (!_loadingComplete)
        {
            return;
        }

        // Use the D3D device context to update Direct3D device-based resources.
        const auto context = _deviceResources->GetD3DDeviceContext();

        // Update the model transform buffer for the hologram.
        context->UpdateSubresource(
            _modelConstantBuffer.Get(),
            0,
            nullptr,
            &_modelConstantBufferData,
            0,
            0
        );
    }

    // Renders one frame using the vertex and pixel shaders.
    // On devices that do not support the D3D11_FEATURE_D3D11_OPTIONS3::
    // VPAndRTArrayIndexFromAnyShaderFeedingRasterizer optional feature,
    // a pass-through geometry shader is also used to set the render 
    // target array index.
    void PolylineRenderer::Render()
    {
        // Loading is asynchronous. Resources must be created before drawing can occur.
        if (!_loadingComplete)
        {
            return;
        }

        const auto context = _deviceResources->GetD3DDeviceContext();

        _slateMaterial->Bind();

        // Each vertex is one instance of the VertexPositionColorTexture struct.
        const UINT stride = sizeof(VertexPositionColorTexture);
        const UINT offset = 0;
        context->IASetVertexBuffers(
            0,
            1,
            _vertexBuffer.GetAddressOf(),
            &stride,
            &offset
        );

        context->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

        // Apply the model constant buffer to the vertex shader.
        context->VSSetConstantBuffers(
            0,
            1,
            _modelConstantBuffer.GetAddressOf()
        );

        // Set pixel shader resources
        {
            ID3D11ShaderResourceView* shaderResourceViews[1] =
            {
                nullptr
            };

            context->PSSetShaderResources(
                0 /* StartSlot */,
                1 /* NumViews */,
                shaderResourceViews);
        }

        // Draw the objects.
        context->DrawInstanced(
            static_cast<uint32_t>(_vertices.size()),
            2 /* InstanceCount */,
            0 /* StartVertexLocation */,
            0 /* StartInstanceLocation */);
    }

    void PolylineRenderer::CreateDeviceDependentResources()
    {
        if (nullptr == _slateMaterial)
        {
            _slateMaterial =
                std::make_unique<SlateMaterial>(
                    _deviceResources);
        }

        _slateMaterial->CreateDeviceDependentResources();

        {
            const CD3D11_BUFFER_DESC constantBufferDesc(sizeof(SlateModelConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
            ASSERT_SUCCEEDED(
                _deviceResources->GetD3DDevice()->CreateBuffer(
                    &constantBufferDesc,
                    nullptr,
                    &_modelConstantBuffer
                )
            );
        }

        // Once all shaders are loaded, create the mesh.
        {
            // Load mesh vertices. Each vertex has a position and a color.
            // Note that the cube size has changed from the default DirectX app
            // template. Windows Holographic is scaled in meters, so to draw the
            // cube at a comfortable size we made the cube width 0.2 m (20 cm).
            const float sx = 1.0f, sy = 1.0f, sz = 1.0f;

            {
                static const std::array<VertexPositionColorTexture, 8> cubeVertices =
                { {
                    { { -sx, -sy, -sz },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 1.0f } },
                    { { -sx, -sy,  sz },{ 0.0f, 1.0f, 0.0f },{ 0.0f, 1.0f } },
                    { { -sx,  sy, -sz },{ 0.0f, 0.0f, 1.0f },{ 0.0f, 0.0f } },
                    { { -sx,  sy,  sz },{ 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f } },
                    { { sx, -sy, -sz },{ 1.0f, 0.0f, 1.0f },{ 1.0f, 1.0f } },
                    { { sx, -sy,  sz },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 1.0f } },
                    { { sx,  sy, -sz },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f } },
                    { { sx,  sy,  sz },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f } },
                    } };

                for (size_t i = 0; i < cubeVertices.size(); ++i)
                {
                    _vertices.emplace_back(
                        cubeVertices[i]);

                    _vertices.emplace_back(
                        cubeVertices[(i + 1) % cubeVertices.size()]);
                }
            }

            D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };

            vertexBufferData.pSysMem = &_vertices[0];
            vertexBufferData.SysMemPitch = 0;
            vertexBufferData.SysMemSlicePitch = 0;

            const CD3D11_BUFFER_DESC vertexBufferDesc(
                static_cast<uint32_t>(sizeof(VertexPositionColorTexture) * _vertices.size()),
                D3D11_BIND_VERTEX_BUFFER);

            ASSERT_SUCCEEDED(
                _deviceResources->GetD3DDevice()->CreateBuffer(
                    &vertexBufferDesc,
                    &vertexBufferData,
                    &_vertexBuffer
                )
            );
        }

        _loadingComplete = true;
    }

    void PolylineRenderer::ReleaseDeviceDependentResources()
    {
        _loadingComplete = false;

        _modelConstantBuffer.Reset();
        _vertexBuffer.Reset();

        if (nullptr != _slateMaterial)
        {
            _slateMaterial->ReleaseDeviceDependentResources();
        }
    }
}
