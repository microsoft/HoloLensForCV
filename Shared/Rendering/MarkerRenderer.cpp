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
    MarkerRenderer::MarkerRenderer(
        const std::shared_ptr<Graphics::DeviceResources>& deviceResources,
        const float markerSize)
        : _markerSize(markerSize)
        , _deviceResources(deviceResources)
    {
        CreateDeviceDependentResources();
    }

    // Called once per frame. Rotates the cube, and calculates and sets the model matrix
    // relative to the position transform indicated by hologramPositionTransform.
    void MarkerRenderer::Update(
        _In_ const Graphics::StepTimer& timer)
    {
        // Position the marker.
        const auto modelTranslation =
            DirectX::XMMatrixTranslationFromVector(
                DirectX::XMLoadFloat3(&_position));

        const float rotationInRadians =
            static_cast<float>(timer.GetTotalSeconds()) * DirectX::XM_PI;

        const auto modelRotation =
            DirectX::XMMatrixRotationX(rotationInRadians) *
            DirectX::XMMatrixRotationY(rotationInRadians) *
            DirectX::XMMatrixRotationZ(rotationInRadians);

        // The view and projection matrices are provided by the system; they are associated
        // with holographic cameras, and updated on a per-camera basis.
        // Here, we provide the model transform for the sample hologram. The model transform
        // matrix is transposed to prepare it for the shader.
        XMStoreFloat4x4(
            &_modelConstantBufferData.model,
            DirectX::XMMatrixTranspose(
                modelRotation * modelTranslation));

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
    void MarkerRenderer::Render()
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
        context->IASetIndexBuffer(
            _indexBuffer.Get(),
            DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
            0
        );

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
        context->DrawIndexedInstanced(
            _indexCount,   // Index count per instance.
            2,              // Instance count.
            0,              // Start index location.
            0,              // Base vertex location.
            0               // Start instance location.
        );
    }

    void MarkerRenderer::CreateDeviceDependentResources()
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
            // template. Windows Holographic is scaled in meters.
            const float sx = _markerSize, sy = _markerSize, sz = _markerSize;
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

            D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };

            vertexBufferData.pSysMem = cubeVertices.data();
            vertexBufferData.SysMemPitch = 0;
            vertexBufferData.SysMemSlicePitch = 0;

            const CD3D11_BUFFER_DESC vertexBufferDesc(
                static_cast<uint32_t>(sizeof(VertexPositionColorTexture) * cubeVertices.size()),
                D3D11_BIND_VERTEX_BUFFER);

            ASSERT_SUCCEEDED(
                _deviceResources->GetD3DDevice()->CreateBuffer(
                    &vertexBufferDesc,
                    &vertexBufferData,
                    &_vertexBuffer
                )
            );

            // Load mesh indices. Each trio of indices represents
            // a triangle to be rendered on the screen.
            // For example: 2,1,0 means that the vertices with indexes
            // 2, 1, and 0 from the vertex buffer compose the
            // first triangle of this mesh.
            // Note that the winding order is clockwise by default.
            constexpr std::array<unsigned short, 36> cubeIndices =
            { {
                    2,1,0, // -x
                    2,3,1,

                    6,4,5, // +x
                    6,5,7,

                    0,1,5, // -y
                    0,5,4,

                    2,6,7, // +y
                    2,7,3,

                    0,4,6, // -z
                    0,6,2,

                    1,3,7, // +z
                    1,7,5,
                } };

            _indexCount = static_cast<uint32_t>(cubeIndices.size());

            D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };

            indexBufferData.pSysMem = cubeIndices.data();
            indexBufferData.SysMemPitch = 0;
            indexBufferData.SysMemSlicePitch = 0;

            CD3D11_BUFFER_DESC indexBufferDesc(
                static_cast<uint32_t>(sizeof(unsigned short) * cubeIndices.size()),
                D3D11_BIND_INDEX_BUFFER);

            ASSERT_SUCCEEDED(
                _deviceResources->GetD3DDevice()->CreateBuffer(
                    &indexBufferDesc,
                    &indexBufferData,
                    &_indexBuffer
                )
            );
        }

        _loadingComplete = true;
    }

    void MarkerRenderer::ReleaseDeviceDependentResources()
    {
        _loadingComplete = false;

        _modelConstantBuffer.Reset();
        _vertexBuffer.Reset();
        _indexBuffer.Reset();

        if (nullptr != _slateMaterial)
        {
            _slateMaterial->ReleaseDeviceDependentResources();
        }
    }
}
