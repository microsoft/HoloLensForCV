﻿//*********************************************************
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
    // Provides an interface for an application that owns DeviceResources to be notified of the device being lost or created.
    interface IDeviceNotify
    {
        virtual void OnDeviceLost()     = 0;
        virtual void OnDeviceRestored() = 0;
    };

    // Creates and manages a Direct3D device and immediate context, Direct2D device and context (for debug), and the holographic swap chain.
    class DeviceResources
    {
    public:
        DeviceResources();

        // Public methods related to Direct3D devices.
        void HandleDeviceLost();
        void RegisterDeviceNotify(IDeviceNotify* deviceNotify);
        void Trim();
        void Present(Windows::Graphics::Holographic::HolographicFrame^ frame);

        // Public methods related to holographic devices.
        void SetHolographicSpace(Windows::Graphics::Holographic::HolographicSpace^ space);
        void EnsureCameraResources(
            Windows::Graphics::Holographic::HolographicFrame^ frame,
            Windows::Graphics::Holographic::HolographicFramePrediction^ prediction);

        void AddHolographicCamera(Windows::Graphics::Holographic::HolographicCamera^ camera);
        void RemoveHolographicCamera(Windows::Graphics::Holographic::HolographicCamera^ camera);

        // Holographic accessors.
        template<typename RetType, typename LCallback>
        RetType                 UseHolographicCameraResources(const LCallback& callback);

        Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice^
                                GetD3DInteropDevice() const             { return _d3dInteropDevice;    }

        // D3D accessors.
        ID3D11Device4*          GetD3DDevice() const                    { return _d3dDevice.Get();     }
        ID3D11DeviceContext3*   GetD3DDeviceContext() const             { return _d3dContext.Get();    }
        D3D_FEATURE_LEVEL       GetDeviceFeatureLevel() const           { return _d3dFeatureLevel;     }
        bool                    GetDeviceSupportsVprt() const           { return _supportsVprt;        }

        // DXGI acessors.
        IDXGIAdapter3*          GetDXGIAdapter() const                  { return _dxgiAdapter.Get();   }

        // D2D accessors.
        ID2D1Factory2*          GetD2DFactory() const                   { return _d2dFactory.Get();    }
        IDWriteFactory2*        GetDWriteFactory() const                { return _dwriteFactory.Get(); }
        IWICImagingFactory2*    GetWicImagingFactory() const            { return _wicFactory.Get();    }

    private:
        // Private methods related to the Direct3D device, and resources based on that device.
        void CreateDeviceIndependentResources();
        void InitializeUsingHolographicSpace();
        void CreateDeviceResources();

        // Direct3D objects.
        Microsoft::WRL::ComPtr<ID3D11Device4>                   _d3dDevice;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext3>            _d3dContext;
        Microsoft::WRL::ComPtr<IDXGIAdapter3>                   _dxgiAdapter;

        // Direct3D interop objects.
        Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice^ _d3dInteropDevice;

        // Direct2D factories.
        Microsoft::WRL::ComPtr<ID2D1Factory2>                   _d2dFactory;
        Microsoft::WRL::ComPtr<IDWriteFactory2>                 _dwriteFactory;
        Microsoft::WRL::ComPtr<IWICImagingFactory2>             _wicFactory;

        // The holographic space provides a preferred DXGI adapter ID.
        Windows::Graphics::Holographic::HolographicSpace^       _holographicSpace = nullptr;

        // Properties of the Direct3D device currently in use.
        D3D_FEATURE_LEVEL                                       _d3dFeatureLevel = D3D_FEATURE_LEVEL_10_0;

        // The IDeviceNotify can be held directly as it owns the DeviceResources.
        IDeviceNotify*                                          _deviceNotify = nullptr;

        // Whether or not the current Direct3D device supports the optional feature 
        // for setting the render target array index from the vertex shader stage.
        bool                                                    _supportsVprt = false;

        // Back buffer resources, etc. for attached holographic cameras.
        std::map<UINT32, std::unique_ptr<CameraResources>>      _cameraResources;
        std::mutex                                              _cameraResourcesLock;
    };

    typedef std::shared_ptr<Graphics::DeviceResources> DeviceResourcesPtr;
}

// Device-based resources for holographic cameras are stored in a std::map. Access this list by providing a
// callback to this function, and the std::map will be guarded from add and remove
// events until the callback returns. The callback is processed immediately and must
// not contain any nested calls to UseHolographicCameraResources.
// The callback takes a parameter of type std::map<UINT32, std::unique_ptr<Graphics::CameraResources>>&
// through which the list of cameras will be accessed.
template<typename RetType, typename LCallback>
RetType Graphics::DeviceResources::UseHolographicCameraResources(const LCallback& callback)
{
    std::lock_guard<std::mutex> guard(_cameraResourcesLock);
    return callback(_cameraResources);
}

