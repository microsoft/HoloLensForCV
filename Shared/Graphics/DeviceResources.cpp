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

using namespace D2D1;
using namespace Microsoft::WRL;
using namespace Windows::Graphics::DirectX::Direct3D11;
using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::Holographic;

// Constructor for DeviceResources.
Graphics::DeviceResources::DeviceResources()
{
    CreateDeviceIndependentResources();
}

// Configures resources that don't depend on the Direct3D device.
void Graphics::DeviceResources::CreateDeviceIndependentResources()
{
    // Initialize Direct2D resources.
    D2D1_FACTORY_OPTIONS options {};

#if defined(_DEBUG)
    // If the project is in a debug build, enable Direct2D debugging via SDK Layers.
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

    // Initialize the Direct2D Factory.
    ASSERT_SUCCEEDED(
        D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED,
            __uuidof(ID2D1Factory2),
            &options,
            &_d2dFactory
            )
        );

    // Initialize the DirectWrite Factory.
    ASSERT_SUCCEEDED(
        DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory2),
            &_dwriteFactory
            )
        );

    // Initialize the Windows Imaging Component (WIC) Factory.
    ASSERT_SUCCEEDED(
        CoCreateInstance(
            CLSID_WICImagingFactory2,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&_wicFactory)
            )
        );
}

void Graphics::DeviceResources::SetHolographicSpace(HolographicSpace^ holographicSpace)
{
    // Cache the holographic space. Used to re-initalize during device-lost scenarios.
    _holographicSpace = holographicSpace;

    InitializeUsingHolographicSpace();
}

void Graphics::DeviceResources::InitializeUsingHolographicSpace()
{
    // The holographic space might need to determine which adapter supports
    // holograms, in which case it will specify a non-zero PrimaryAdapterId.
    LUID id =
    {
        _holographicSpace->PrimaryAdapterId.LowPart,
        _holographicSpace->PrimaryAdapterId.HighPart
    };

    // When a primary adapter ID is given to the app, the app should find
    // the corresponding DXGI adapter and use it to create Direct3D devices
    // and device contexts. Otherwise, there is no restriction on the DXGI
    // adapter the app can use.
    if ((id.HighPart != 0) && (id.LowPart != 0))
    {
        UINT createFlags = 0;
#ifdef DEBUG
        if (Graphics::SdkLayersAvailable())
        {
            createFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
#endif
        // Create the DXGI factory.
        ComPtr<IDXGIFactory1> dxgiFactory;
        ASSERT_SUCCEEDED(
            CreateDXGIFactory2(
                createFlags,
                IID_PPV_ARGS(&dxgiFactory)
                )
            );
        ComPtr<IDXGIFactory4> dxgiFactory4;
        ASSERT_SUCCEEDED(dxgiFactory.As(&dxgiFactory4));

        // Retrieve the adapter specified by the holographic space.
        ASSERT_SUCCEEDED(
            dxgiFactory4->EnumAdapterByLuid(
                id,
                IID_PPV_ARGS(&_dxgiAdapter)
                )
            );
    }
    else
    {
        _dxgiAdapter.Reset();
    }

    CreateDeviceResources();

    _holographicSpace->SetDirect3D11Device(_d3dInteropDevice);
}

// Configures the Direct3D device, and stores handles to it and the device context.
void Graphics::DeviceResources::CreateDeviceResources()
{
    // This flag adds support for surfaces with a different color channel ordering
    // than the API default. It is required for compatibility with Direct2D.
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
    if (Graphics::SdkLayersAvailable())
    {
        // If the project is in a debug build, enable debugging via SDK Layers with this flag.
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }
#endif

    // This array defines the set of DirectX hardware feature levels this app will support.
    // Note the ordering should be preserved.
    // Note that HoloLens supports feature level 11.1. The HoloLens emulator is also capable
    // of running on graphics cards starting with feature level 10.0.
    D3D_FEATURE_LEVEL featureLevels [] =
    {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    // Create the Direct3D 11 API device object and a corresponding context.
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;

    const HRESULT hr = D3D11CreateDevice(
        _dxgiAdapter.Get(),        // Either nullptr, or the primary adapter determined by Windows Holographic.
        D3D_DRIVER_TYPE_HARDWARE,   // Create a device using the hardware graphics driver.
        0,                          // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
        creationFlags,              // Set debug and Direct2D compatibility flags.
        featureLevels,              // List of feature levels this app can support.
        ARRAYSIZE(featureLevels),   // Size of the list above.
        D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
        &device,                    // Returns the Direct3D device created.
        &_d3dFeatureLevel,         // Returns feature level of device created.
        &context                    // Returns the device immediate context.
        );

    if (FAILED(hr))
    {
        // If the initialization fails, fall back to the WARP device.
        // For more information on WARP, see:
        // http://go.microsoft.com/fwlink/?LinkId=286690
        ASSERT_SUCCEEDED(
            D3D11CreateDevice(
                nullptr,              // Use the default DXGI adapter for WARP.
                D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
                0,
                creationFlags,
                featureLevels,
                ARRAYSIZE(featureLevels),
                D3D11_SDK_VERSION,
                &device,
                &_d3dFeatureLevel,
                &context
                )
            );
    }

    // Store pointers to the Direct3D device and immediate context.
    ASSERT_SUCCEEDED(
        device.As(&_d3dDevice)
        );

    ASSERT_SUCCEEDED(
        context.As(&_d3dContext)
        );

    // Acquire the DXGI interface for the Direct3D device.
    ComPtr<IDXGIDevice3> dxgiDevice;
    ASSERT_SUCCEEDED(
        _d3dDevice.As(&dxgiDevice)
        );

    // Wrap the native device using a WinRT interop object.
    _d3dInteropDevice = CreateDirect3DDevice(dxgiDevice.Get());

    // Cache the DXGI adapter.
    // This is for the case of no preferred DXGI adapter, or fallback to WARP.
    ComPtr<IDXGIAdapter> dxgiAdapter;
    ASSERT_SUCCEEDED(
        dxgiDevice->GetAdapter(&dxgiAdapter)
        );
    ASSERT_SUCCEEDED(
        dxgiAdapter.As(&_dxgiAdapter)
        );

    // Check for device support for the optional feature that allows setting the render target array index from the vertex shader stage.
    D3D11_FEATURE_DATA_D3D11_OPTIONS3 options;
    _d3dDevice->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS3, &options, sizeof(options));
    if (options.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer)
    {
        _supportsVprt = true;
    }
}

// Validates the back buffer for each HolographicCamera and recreates
// resources for back buffers that have changed.
// Locks the set of holographic camera resources until the function exits.
void Graphics::DeviceResources::EnsureCameraResources(
    HolographicFrame^ frame,
    HolographicFramePrediction^ prediction)
{
    UseHolographicCameraResources<void>([this, frame, prediction](std::map<UINT32, std::unique_ptr<CameraResources>>& cameraResourceMap)
    {
        for (HolographicCameraPose^ pose : prediction->CameraPoses)
        {
            HolographicCameraRenderingParameters^ renderingParameters = frame->GetRenderingParameters(pose);
            CameraResources* pCameraResources = cameraResourceMap[pose->HolographicCamera->Id].get();

            pCameraResources->CreateResourcesForBackBuffer(this, renderingParameters);
        }
    });
}

// Prepares to allocate resources and adds resource views for a camera.
// Locks the set of holographic camera resources until the function exits.
void Graphics::DeviceResources::AddHolographicCamera(HolographicCamera^ camera)
{
    UseHolographicCameraResources<void>([this, camera](std::map<UINT32, std::unique_ptr<CameraResources>>& cameraResourceMap)
    {
        cameraResourceMap[camera->Id] = std::make_unique<CameraResources>(camera);
    });
}

// Deallocates resources for a camera and removes the camera from the set.
// Locks the set of holographic camera resources until the function exits.
void Graphics::DeviceResources::RemoveHolographicCamera(HolographicCamera^ camera)
{
    UseHolographicCameraResources<void>([this, camera](std::map<UINT32, std::unique_ptr<CameraResources>>& cameraResourceMap)
    {
        CameraResources* pCameraResources = cameraResourceMap[camera->Id].get();

        if (pCameraResources != nullptr)
        {
            pCameraResources->ReleaseResourcesForBackBuffer(this);
            cameraResourceMap.erase(camera->Id);
        }
    });
}

// Recreate all device resources and set them back to the current state.
// Locks the set of holographic camera resources until the function exits.
void Graphics::DeviceResources::HandleDeviceLost()
{
    if (_deviceNotify != nullptr)
    {
        _deviceNotify->OnDeviceLost();
    }

    UseHolographicCameraResources<void>([this](std::map<UINT32, std::unique_ptr<CameraResources>>& cameraResourceMap)
    {
        for (auto& pair : cameraResourceMap)
        {
            CameraResources* pCameraResources = pair.second.get();
            pCameraResources->ReleaseResourcesForBackBuffer(this);
        }
    });

    InitializeUsingHolographicSpace();

    if (_deviceNotify != nullptr)
    {
        _deviceNotify->OnDeviceRestored();
    }
}

// Register our DeviceNotify to be informed on device lost and creation.
void Graphics::DeviceResources::RegisterDeviceNotify(Graphics::IDeviceNotify* deviceNotify)
{
    _deviceNotify = deviceNotify;
}

// Call this method when the app suspends. It provides a hint to the driver that the app
// is entering an idle state and that temporary buffers can be reclaimed for use by other apps.
void Graphics::DeviceResources::Trim()
{
    _d3dContext->ClearState();

    ComPtr<IDXGIDevice3> dxgiDevice;
    ASSERT_SUCCEEDED(_d3dDevice.As(&dxgiDevice));
    dxgiDevice->Trim();
}

// Present the contents of the swap chain to the screen.
// Locks the set of holographic camera resources until the function exits.
void Graphics::DeviceResources::Present(HolographicFrame^ frame)
{
    // By default, this API waits for the frame to finish before it returns.
    // Holographic apps should wait for the previous frame to finish before
    // starting work on a new frame. This allows for better results from
    // holographic frame predictions.
    HolographicFramePresentResult presentResult = frame->PresentUsingCurrentPrediction();

    HolographicFramePrediction^ prediction = frame->CurrentPrediction;
    UseHolographicCameraResources<void>([this, prediction](std::map<UINT32, std::unique_ptr<CameraResources>>& cameraResourceMap)
    {
        for (auto cameraPose : prediction->CameraPoses)
        {
            // This represents the device-based resources for a HolographicCamera.
            Graphics::CameraResources* pCameraResources = cameraResourceMap[cameraPose->HolographicCamera->Id].get();

            // Discard the contents of the render target.
            // This is a valid operation only when the existing contents will be
            // entirely overwritten. If dirty or scroll rects are used, this call
            // should be removed.
            _d3dContext->DiscardView(pCameraResources->GetBackBufferRenderTargetView());

            // Discard the contents of the depth stencil.
            _d3dContext->DiscardView(pCameraResources->GetDepthStencilView());
        }
    });

    // The PresentUsingCurrentPrediction API will detect when the graphics device
    // changes or becomes invalid. When this happens, it is considered a Direct3D
    // device lost scenario.
    if (presentResult == HolographicFramePresentResult::DeviceRemoved)
    {
        // The Direct3D device, context, and resources should be recreated.
        HandleDeviceLost();
    }
}
