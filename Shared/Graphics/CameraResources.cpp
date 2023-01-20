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

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Graphics::DirectX::Direct3D11;
using namespace Windows::Graphics::Holographic;
using namespace Windows::Perception::Spatial;

Graphics::CameraResources::CameraResources(HolographicCamera^ camera) :
    _holographicCamera(camera),
    _isStereo(camera->IsStereo),
    _d3dRenderTargetSize(camera->RenderTargetSize)
{
    _d3dViewport = CD3D11_VIEWPORT(
        0.f, 0.f,
        _d3dRenderTargetSize.Width,
        _d3dRenderTargetSize.Height
        );
};

// Updates resources associated with a holographic camera's swap chain.
// The app does not access the swap chain directly, but it does create
// resource views for the back buffer.
void Graphics::CameraResources::CreateResourcesForBackBuffer(
    Graphics::DeviceResources* pDeviceResources,
    HolographicCameraRenderingParameters^ cameraParameters
    )
{
    const auto device = pDeviceResources->GetD3DDevice();

    // Get the WinRT object representing the holographic camera's back buffer.
    IDirect3DSurface^ surface = cameraParameters->Direct3D11BackBuffer;

    // Get a DXGI interface for the holographic camera's back buffer.
    // Holographic cameras do not provide the DXGI swap chain, which is owned
    // by the system. The Direct3D back buffer resource is provided using WinRT
    // interop APIs.
    ComPtr<ID3D11Resource> resource;
    ASSERT_SUCCEEDED(
        GetDXGIInterfaceFromObject(surface, IID_PPV_ARGS(&resource))
        );

    // Get a Direct3D interface for the holographic camera's back buffer.
    ComPtr<ID3D11Texture2D> cameraBackBuffer;
    ASSERT_SUCCEEDED(
        resource.As(&cameraBackBuffer)
        );

    // Determine if the back buffer has changed. If so, ensure that the render target view
    // is for the current back buffer.
    if (_d3dBackBuffer.Get() != cameraBackBuffer.Get())
    {
        // This can change every frame as the system moves to the next buffer in the
        // swap chain. This mode of operation will occur when certain rendering modes
        // are activated.
        _d3dBackBuffer = cameraBackBuffer;

        // Create a render target view of the back buffer.
        // Creating this resource is inexpensive, and is better than keeping track of
        // the back buffers in order to pre-allocate render target views for each one.
        ASSERT_SUCCEEDED(
            device->CreateRenderTargetView(
                _d3dBackBuffer.Get(),
                nullptr,
                &_d3dRenderTargetView
                )
            );

        // Get the DXGI format for the back buffer.
        // This information can be accessed by the app using CameraResources::GetBackBufferDXGIFormat().
        D3D11_TEXTURE2D_DESC backBufferDesc;
        _d3dBackBuffer->GetDesc(&backBufferDesc);
        _dxgiFormat = backBufferDesc.Format;

        // Check for render target size changes.
        Windows::Foundation::Size currentSize = _holographicCamera->RenderTargetSize;
        if (_d3dRenderTargetSize != currentSize)
        {
            // Set render target size.
            _d3dRenderTargetSize = currentSize;

            // A new depth stencil view is also needed.
            _d3dDepthStencilView.Reset();
        }
    }

    // Refresh depth stencil resources, if needed.
    if (_d3dDepthStencilView == nullptr)
    {
        // Create a depth stencil view for use with 3D rendering if needed.
        CD3D11_TEXTURE2D_DESC depthStencilDesc(
            DXGI_FORMAT_R16_TYPELESS,
            static_cast<UINT>(_d3dRenderTargetSize.Width),
            static_cast<UINT>(_d3dRenderTargetSize.Height),
            _isStereo ? 2 : 1, // Create two textures when rendering in stereo.
            1, // Use a single mipmap level.
            D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE
            );

        ASSERT_SUCCEEDED(
            device->CreateTexture2D(
                &depthStencilDesc,
                nullptr,
                &_d3dDepthStencil
                )
            );

        CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(
            _isStereo ? D3D11_DSV_DIMENSION_TEXTURE2DARRAY : D3D11_DSV_DIMENSION_TEXTURE2D,
            DXGI_FORMAT_D16_UNORM
            );
        ASSERT_SUCCEEDED(
            device->CreateDepthStencilView(
                _d3dDepthStencil.Get(),
                &depthStencilViewDesc,
                &_d3dDepthStencilView
                )
            );
    }

    // Create the constant buffer, if needed.
    if (_viewProjectionConstantBuffer == nullptr)
    {
        // Create a constant buffer to store view and projection matrices for the camera.
        CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
        ASSERT_SUCCEEDED(
            device->CreateBuffer(
                &constantBufferDesc,
                nullptr,
                &_viewProjectionConstantBuffer
                )
            );
    }
}

// Releases resources associated with a back buffer.
void Graphics::CameraResources::ReleaseResourcesForBackBuffer(Graphics::DeviceResources* pDeviceResources)
{
    const auto context = pDeviceResources->GetD3DDeviceContext();

    // Release camera-specific resources.
    _d3dBackBuffer.Reset();
    _d3dRenderTargetView.Reset();
    _d3dDepthStencilView.Reset();
    _viewProjectionConstantBuffer.Reset();

    // Ensure system references to the back buffer are released by clearing the render
    // target from the graphics pipeline state, and then flushing the Direct3D context.
    ID3D11RenderTargetView* nullViews[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { nullptr };
    context->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
    context->Flush();
}

// Updates the view/projection constant buffer for a holographic camera.
void Graphics::CameraResources::UpdateViewProjectionBuffer(
    std::shared_ptr<Graphics::DeviceResources> deviceResources,
    HolographicCameraPose^ cameraPose,
    SpatialCoordinateSystem^ coordinateSystem
    )
{
    // The system changes the viewport on a per-frame basis for system optimizations.
    _d3dViewport = CD3D11_VIEWPORT(
        cameraPose->Viewport.Left,
        cameraPose->Viewport.Top,
        cameraPose->Viewport.Width,
        cameraPose->Viewport.Height
        );

    // The projection transform for each frame is provided by the HolographicCameraPose.
    HolographicStereoTransform cameraProjectionTransform = cameraPose->ProjectionTransform;

    // Get a container object with the view and projection matrices for the given
    // pose in the given coordinate system.
    Platform::IBox<HolographicStereoTransform>^ viewTransformContainer = cameraPose->TryGetViewTransform(coordinateSystem);

    // If TryGetViewTransform returns a null pointer, that means the pose and coordinate
    // system cannot be understood relative to one another; content cannot be rendered
    // in this coordinate system for the duration of the current frame.
    // This usually means that positional tracking is not active for the current frame, in
    // which case it is possible to use a SpatialLocatorAttachedFrameOfReference to render
    // content that is not world-locked instead.
    Graphics::ViewProjectionConstantBuffer viewProjectionConstantBufferData;
    bool viewTransformAcquired = viewTransformContainer != nullptr;
    if (viewTransformAcquired)
    {
        // Otherwise, the set of view transforms can be retrieved.
        HolographicStereoTransform viewCoordinateSystemTransform = viewTransformContainer->Value;

        // Update the view matrices. Holographic cameras (such as Microsoft HoloLens) are
        // constantly moving relative to the world. The view matrices need to be updated
        // every frame.
        XMStoreFloat4x4(
            &viewProjectionConstantBufferData.viewProjection[0],
            XMMatrixTranspose(XMLoadFloat4x4(&viewCoordinateSystemTransform.Left) * XMLoadFloat4x4(&cameraProjectionTransform.Left))
            );
        XMStoreFloat4x4(
            &viewProjectionConstantBufferData.viewProjection[1],
            XMMatrixTranspose(XMLoadFloat4x4(&viewCoordinateSystemTransform.Right) * XMLoadFloat4x4(&cameraProjectionTransform.Right))
            );
    }

    // Use the D3D device context to update Direct3D device-based resources.
    const auto context = deviceResources->GetD3DDeviceContext();

    // Loading is asynchronous. Resources must be created before they can be updated.
    if (context == nullptr || _viewProjectionConstantBuffer == nullptr || !viewTransformAcquired)
    {
        _framePending = false;
    }
    else
    {
        // Update the view and projection matrices.
        context->UpdateSubresource(
            _viewProjectionConstantBuffer.Get(),
            0,
            nullptr,
            &viewProjectionConstantBufferData,
            0,
            0
            );

        _framePending = true;
    }
}

// Gets the view-projection constant buffer for the HolographicCamera and attaches it
// to the shader pipeline.
bool Graphics::CameraResources::AttachViewProjectionBuffer(
    std::shared_ptr<Graphics::DeviceResources> deviceResources
    )
{
    // This method uses Direct3D device-based resources.
    const auto context = deviceResources->GetD3DDeviceContext();

    // Loading is asynchronous. Resources must be created before they can be updated.
    // Cameras can also be added asynchronously, in which case they must be initialized
    // before they can be used.
    if (context == nullptr || _viewProjectionConstantBuffer == nullptr || _framePending == false)
    {
        return false;
    }

    // Set the viewport for this camera.
    context->RSSetViewports(1, &_d3dViewport);

    // Send the constant buffer to the vertex shader.
    context->VSSetConstantBuffers(
        1,
        1,
        _viewProjectionConstantBuffer.GetAddressOf()
        );

    // The template includes a pass-through geometry shader that is used by
    // default on systems that don't support the D3D11_FEATURE_D3D11_OPTIONS3::
    // VPAndRTArrayIndexFromAnyShaderFeedingRasterizer extension. The shader 
    // will be enabled at run-time on systems that require it.
    // If your app will also use the geometry shader for other tasks and those
    // tasks require the view/projection matrix, uncomment the following line 
    // of code to send the constant buffer to the geometry shader as well.
    /*context->GSSetConstantBuffers(
        1,
        1,
        _viewProjectionConstantBuffer.GetAddressOf()
        );*/

    _framePending = false;

    return true;
}
