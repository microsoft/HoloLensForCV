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

namespace Graphics
{
    class DeviceResources;

    // Constant buffer used to send the view-projection matrices to the shader pipeline.
    struct ViewProjectionConstantBuffer
    {
        DirectX::XMFLOAT4X4 viewProjection[2];
    };

    // Assert that the constant buffer remains 16-byte aligned (best practice).
    static_assert(
        (sizeof(ViewProjectionConstantBuffer) % (sizeof(float) * 4)) == 0,
        "ViewProjection constant buffer size must be 16-byte aligned (16 bytes is the length of four floats).");

    // Manages DirectX device resources that are specific to a holographic camera, such as the
    // back buffer, ViewProjection constant buffer, and viewport.
    class CameraResources
    {
    public:
        CameraResources(Windows::Graphics::Holographic::HolographicCamera^ holographicCamera);

        void CreateResourcesForBackBuffer(
            Graphics::DeviceResources* pDeviceResources,
            Windows::Graphics::Holographic::HolographicCameraRenderingParameters^ cameraParameters
            );
        void ReleaseResourcesForBackBuffer(
            Graphics::DeviceResources* pDeviceResources
            );

        void UpdateViewProjectionBuffer(
            std::shared_ptr<Graphics::DeviceResources> deviceResources,
            Windows::Graphics::Holographic::HolographicCameraPose^ cameraPose,
            Windows::Perception::Spatial::SpatialCoordinateSystem^ coordinateSystem);

        bool AttachViewProjectionBuffer(
            std::shared_ptr<Graphics::DeviceResources> deviceResources);

        // Direct3D device resources.
        ID3D11RenderTargetView* GetBackBufferRenderTargetView()     const { return _d3dRenderTargetView.Get();     }
        ID3D11DepthStencilView* GetDepthStencilView()               const { return _d3dDepthStencilView.Get();     }
        ID3D11Texture2D*        GetBackBufferTexture2D()            const { return _d3dBackBuffer.Get();           }
        D3D11_VIEWPORT          GetViewport()                       const { return _d3dViewport;                   }
        DXGI_FORMAT             GetBackBufferDXGIFormat()           const { return _dxgiFormat;                    }

        // Render target properties.
        Windows::Foundation::Size GetRenderTargetSize()             const { return _d3dRenderTargetSize;           }
        bool                    IsRenderingStereoscopic()           const { return _isStereo;                      }

        // The holographic camera these resources are for.
        Windows::Graphics::Holographic::HolographicCamera^ GetHolographicCamera() const { return _holographicCamera; }

    private:
        // Direct3D rendering objects. Required for 3D.
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView>      _d3dRenderTargetView;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView>      _d3dDepthStencilView;
        Microsoft::WRL::ComPtr<ID3D11Texture2D>             _d3dBackBuffer;

        // Device resource to store view and projection matrices.
        Microsoft::WRL::ComPtr<ID3D11Buffer>                _viewProjectionConstantBuffer;

        // Direct3D rendering properties.
        DXGI_FORMAT                                         _dxgiFormat;
        Windows::Foundation::Size                           _d3dRenderTargetSize;
        D3D11_VIEWPORT                                      _d3dViewport;

        // Indicates whether the camera supports stereoscopic rendering.
        bool                                                _isStereo = false;

        // Indicates whether this camera has a pending frame.
        bool                                                _framePending = false;

        // Pointer to the holographic camera these resources are for.
        Windows::Graphics::Holographic::HolographicCamera^  _holographicCamera = nullptr;
    };
}
