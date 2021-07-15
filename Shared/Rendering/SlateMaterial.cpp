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
    // Loads vertex and pixel shaders from files.
    SlateMaterial::SlateMaterial(
        const std::shared_ptr<Graphics::DeviceResources>& deviceResources)
        : _deviceResources(deviceResources)
    {
        CreateDeviceDependentResources();
    }

    void SlateMaterial::Update(
        _In_ const Graphics::StepTimer& /* timer */)
    {
    }

    // Binds the material for rendering.
    // On devices that do not support the D3D11_FEATURE_D3D11_OPTIONS3::
    // VPAndRTArrayIndexFromAnyShaderFeedingRasterizer optional feature,
    // a pass-through geometry shader is also used to set the render 
    // target array index.
    void SlateMaterial::Bind()
    {
        // Loading is asynchronous. Resources must be created before drawing can occur.
        if (!_loadingComplete)
        {
            return;
        }

        const auto context = _deviceResources->GetD3DDeviceContext();

        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->IASetInputLayout(_inputLayout.Get());

        // Attach the vertex shader.
        context->VSSetShader(
            _vertexShader.Get(),
            nullptr,
            0
        );

        if (!_usingVprtShaders)
        {
            // On devices that do not support the D3D11_FEATURE_D3D11_OPTIONS3::
            // VPAndRTArrayIndexFromAnyShaderFeedingRasterizer optional feature,
            // a pass-through geometry shader is used to set the render target 
            // array index.
            context->GSSetShader(
                _geometryShader.Get(),
                nullptr,
                0
            );
        }

        // Attach the pixel shader.
        context->PSSetShader(
            _pixelShader.Get(),
            nullptr,
            0
        );
    }

    void SlateMaterial::CreateDeviceDependentResources()
    {
        _usingVprtShaders = _deviceResources->GetDeviceSupportsVprt();

        // On devices that do support the D3D11_FEATURE_D3D11_OPTIONS3::
        // VPAndRTArrayIndexFromAnyShaderFeedingRasterizer optional feature
        // we can avoid using a pass-through geometry shader to set the render
        // target array index, thus avoiding any overhead that would be 
        // incurred by setting the geometry shader stage.
        const std::wstring vertexShaderFileName =
            _usingVprtShaders
            ? L"ms-appx:///Rendering/SlateMaterial.VPRT.vs.cso"
            : L"ms-appx:///Rendering/SlateMaterial.Default.vs.cso";

        // Load shaders asynchronously.
        Concurrency::task<std::vector<byte>> loadVSTask =
            Io::ReadDataAsync(
                vertexShaderFileName);

        Concurrency::task<std::vector<byte>> loadPSTask =
            Io::ReadDataAsync(
                L"ms-appx:///Rendering/SlateMaterial.Default.ps.cso");

        Concurrency::task<std::vector<byte>> loadGSTask;
        if (!_usingVprtShaders)
        {
            // Load the pass-through geometry shader.
            loadGSTask =
                Io::ReadDataAsync(
                    L"ms-appx:///Rendering/SlateMaterial.Default.gs.cso");
        }

        // After the vertex shader file is loaded, create the shader and input layout.
        Concurrency::task<void> createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData)
        {
            ASSERT_SUCCEEDED(
                _deviceResources->GetD3DDevice()->CreateVertexShader(
                    fileData.data(),
                    fileData.size(),
                    nullptr,
                    &_vertexShader
                )
            );

            constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 3> vertexDesc =
            { {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            } };

            ASSERT_SUCCEEDED(
                _deviceResources->GetD3DDevice()->CreateInputLayout(
                    vertexDesc.data(),
                    static_cast<uint32_t>(vertexDesc.size()),
                    fileData.data(),
                    fileData.size(),
                    &_inputLayout
                )
            );
        });

        Concurrency::task<void> createGSTask;
        if (!_usingVprtShaders)
        {
            // After the pass-through geometry shader file is loaded, create the shader.
            createGSTask = loadGSTask.then([this](const std::vector<byte>& fileData)
            {
                ASSERT_SUCCEEDED(
                    _deviceResources->GetD3DDevice()->CreateGeometryShader(
                        fileData.data(),
                        fileData.size(),
                        nullptr,
                        &_geometryShader
                    )
                );
            });
        }

        // After the pixel shader file is loaded, create the shader and constant buffer.
        Concurrency::task<void> createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData)
        {
            ASSERT_SUCCEEDED(
                _deviceResources->GetD3DDevice()->CreatePixelShader(
                    fileData.data(),
                    fileData.size(),
                    nullptr,
                    &_pixelShader
                )
            );
        });

        // Once all shaders are loaded, create the mesh.
        Concurrency::task<void> shaderTaskGroup =
            _usingVprtShaders
            ? (createPSTask && createVSTask)
            : (createPSTask && createGSTask && createVSTask);

        // Once the cube is loaded, the object is ready to be rendered.
        shaderTaskGroup.then([this]()
        {
            _loadingComplete = true;
        });
    }

    void SlateMaterial::ReleaseDeviceDependentResources()
    {
        _loadingComplete = false;
        _usingVprtShaders = false;
        _vertexShader.Reset();
        _inputLayout.Reset();
        _pixelShader.Reset();
        _geometryShader.Reset();
    }
}
