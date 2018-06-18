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

using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;
using namespace Windows::Networking;
using namespace Windows::Networking::Connectivity;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;
using namespace Windows::Graphics::Holographic;
using namespace Windows::Perception::Spatial;
using namespace Windows::UI::Input::Spatial;

namespace Holographic
{
    // Loads and initializes application assets when the application is loaded.
    AppMainBase::AppMainBase(
        const std::shared_ptr<Graphics::DeviceResources>& deviceResources)
        : _deviceResources(deviceResources)
    {
        // Register to be notified if the device is lost or recreated.
        _deviceResources->RegisterDeviceNotify(
            this);
    }

    AppMainBase::~AppMainBase()
    {
        // Deregister device notification.
        _deviceResources->RegisterDeviceNotify(nullptr);

        UnregisterHolographicEventHandlers();
    }

    void AppMainBase::SetHolographicSpace(
        HolographicSpace^ holographicSpace)
    {
        UnregisterHolographicEventHandlers();

        _holographicSpace = holographicSpace;

        //
        // In this sample, we will defer to the HoloLensForCV component for spatial perception.
        //
        _spatialPerception =
            ref new HoloLensForCV::SpatialPerception();

        _spatialInputHandler =
            std::make_shared<SpatialInputHandler>();

        // Respond to camera added events by creating any resources that are specific
        // to that camera, such as the back buffer render target view.
        // When we add an event handler for CameraAdded, the API layer will avoid putting
        // the new camera in new HolographicFrames until we complete the deferral we created
        // for that handler, or return from the handler without creating a deferral. This
        // allows the app to take more than one frame to finish creating resources and
        // loading assets for the new holographic camera.
        // This function should be registered before the app creates any HolographicFrames.
        _cameraAddedToken =
            _holographicSpace->CameraAdded +=
                ref new Windows::Foundation::TypedEventHandler<HolographicSpace^, HolographicSpaceCameraAddedEventArgs^>(
                    std::bind(
                        &AppMainBase::OnCameraAdded,
                        this,
                        std::placeholders::_1,
                        std::placeholders::_2));

        // Respond to camera removed events by releasing resources that were created for that
        // camera.
        // When the app receives a CameraRemoved event, it releases all references to the back
        // buffer right away. This includes render target views, Direct2D target bitmaps, and so on.
        // The app must also ensure that the back buffer is not attached as a render target, as
        // shown in DeviceResources::ReleaseResourcesForBackBuffer.
        _cameraRemovedToken =
            _holographicSpace->CameraRemoved +=
                ref new Windows::Foundation::TypedEventHandler<HolographicSpace^, HolographicSpaceCameraRemovedEventArgs^>(
                    std::bind(
                        &AppMainBase::OnCameraRemoved,
                        this,
                        std::placeholders::_1,
                        std::placeholders::_2));

        OnHolographicSpaceChanged(
            holographicSpace);
    }

    void AppMainBase::UnregisterHolographicEventHandlers()
    {
        if (_holographicSpace != nullptr)
        {
            // Clear previous event registrations.

            if (_cameraAddedToken.Value != 0)
            {
                _holographicSpace->CameraAdded -= _cameraAddedToken;
                _cameraAddedToken.Value = 0;
            }

            if (_cameraRemovedToken.Value != 0)
            {
                _holographicSpace->CameraRemoved -= _cameraRemovedToken;
                _cameraRemovedToken.Value = 0;
            }
        }

        if (nullptr != _spatialPerception)
        {
            _spatialPerception->UnregisterHolographicEventHandlers();
        }
    }

    // Updates the application state once per frame.
    HolographicFrame^ AppMainBase::Update()
    {
        // Before doing the timer update, there is some work to do per-frame
        // to maintain holographic rendering. First, we will get information
        // about the current frame.

        // The HolographicFrame has information that the app needs in order
        // to update and render the current frame. The app begins each new
        // frame by calling CreateNextFrame.
        HolographicFrame^ holographicFrame = _holographicSpace->CreateNextFrame();

        // Get a prediction of where holographic cameras will be when this frame
        // is presented.
        HolographicFramePrediction^ prediction = holographicFrame->CurrentPrediction;

        // Back buffers can change from frame to frame. Validate each buffer, and recreate
        // resource views and depth buffers as needed.
        _deviceResources->EnsureCameraResources(holographicFrame, prediction);

        //
        // Check for new input state since the last frame.
        //
        SpatialInteractionSourceState^ pointerState =
            _spatialInputHandler->CheckForInput();

        if (pointerState != nullptr)
        {
            OnSpatialInput(
                pointerState);
        }

        _timer.Tick([&]()
        {
            //
            // Put time-based updates here. By default this code will run once per frame,
            // but if you change the StepTimer to use a fixed time step this code will
            // run as many times as needed to get to the current step.
            //
            OnUpdate(
                holographicFrame,
                _timer);
        });

        // The holographic frame will be used to get up-to-date view and projection matrices and
        // to present the swap chain.
        return holographicFrame;
    }

    // Renders the current frame to each holographic camera, according to the
    // current application and spatial positioning state. Returns true if the
    // frame was rendered to at least one camera.
    bool AppMainBase::Render(
        Windows::Graphics::Holographic::HolographicFrame^ holographicFrame)
    {
        // Don't try to render anything before the first Update.
        if (_timer.GetFrameCount() == 0)
        {
            return false;
        }

        //
        // Take care of any tasks that are not specific to an individual holographic
        // camera. This includes anything that doesn't need the final view or projection
        // matrix, such as lighting maps.
        //
        OnPreRender();

        // Lock the set of holographic camera resources, then draw to each camera
        // in this frame.
        return _deviceResources->UseHolographicCameraResources<bool>(
            [this, holographicFrame](
                std::map<UINT32, std::unique_ptr<Graphics::CameraResources>>& cameraResourceMap)
        {
            // Up-to-date frame predictions enhance the effectiveness of image stablization and
            // allow more accurate positioning of holograms.
            holographicFrame->UpdateCurrentPrediction();

            HolographicFramePrediction^ prediction =
                holographicFrame->CurrentPrediction;

            bool atLeastOneCameraRendered = false;

            for (auto cameraPose : prediction->CameraPoses)
            {
                // This represents the device-based resources for a HolographicCamera.
                Graphics::CameraResources* pCameraResources =
                    cameraResourceMap[
                        cameraPose->HolographicCamera->Id].get();

                // Get the device context.
                const auto context =
                    _deviceResources->GetD3DDeviceContext();

                const auto depthStencilView =
                    pCameraResources->GetDepthStencilView();

                // Set render targets to the current holographic camera.
                {
                    ID3D11RenderTargetView *const targets[1] =
                    {
                        pCameraResources->GetBackBufferRenderTargetView()
                    };

                    context->OMSetRenderTargets(
                        1 /* NumViews */,
                        targets,
                        depthStencilView);

                    // Clear the back buffer and depth stencil view.
                    context->ClearRenderTargetView(
                        targets[0],
                        DirectX::Colors::Transparent);

                    context->ClearDepthStencilView(
                        depthStencilView,
                        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                        1.0f /* Depth */,
                        0 /* Stencil */);
                }

                //
                // Notes regarding holographic content:
                //    * For drawing, remember that you have the potential to fill twice as many pixels
                //      in a stereoscopic render target as compared to a non-stereoscopic render target
                //      of the same resolution. Avoid unnecessary or repeated writes to the same pixel,
                //      and only draw holograms that the user can see.
                //    * To help occlude hologram geometry, you can create a depth map using geometry
                //      data obtained via the surface mapping APIs. You can use this depth map to avoid
                //      rendering holograms that are intended to be hidden behind tables, walls,
                //      monitors, and so on.
                //    * Black pixels will appear transparent to the user wearing the device, but you
                //      should still use alpha blending to draw semitransparent holograms. You should
                //      also clear the screen to Transparent as shown above.
                //

                // The view and projection matrices for each holographic camera will change
                // every frame. This function refreshes the data in the constant buffer for
                // the holographic camera indicated by cameraPose.
                pCameraResources->UpdateViewProjectionBuffer(
                    _deviceResources,
                    cameraPose,
                    _spatialPerception->GetOriginFrameOfReference()->CoordinateSystem);

                // Attach the view/projection constant buffer for this camera to the graphics pipeline.
                const bool cameraActive =
                    pCameraResources->AttachViewProjectionBuffer(
                        _deviceResources);

                //
                // Only render world-locked content when positional tracking is active.
                //
                if (cameraActive)
                {
                    OnRender();
                }

                //
                // Unbind the render target and depth-stencil views from the pipeline
                //
                {
                    ID3D11RenderTargetView* nullViews[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { nullptr };
                    context->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
                }

                //
                // When positional tracking is active, 
                //
                if (cameraActive)
                {
                    auto spRenderingParameters = holographicFrame->GetRenderingParameters(
                        cameraPose);
              
                    if (_hasFocusPoint)
                    {
                        //
                        // SetFocusPoint informs the system about a specific point in your scene to
                        // prioritize for image stabilization. The focus point is set independently
                        // for each holographic camera.
                        // You should set the focus point near the content that the user is looking at.
                        // In this example, we put the focus point at the center of the sample hologram,
                        // since that is the only hologram available for the user to focus on.
                        // You can also set the relative velocity and facing of that content; the sample
                        // hologram is at a fixed point so we only need to indicate its position.
                        //
                        spRenderingParameters->SetFocusPoint(
                            _spatialPerception->GetOriginFrameOfReference()->CoordinateSystem,
                            _optionalFocusPoint);
                    }
#if 0
                    else
                    {
                        //
                        // Make use of the depth buffer to optimize image stabilization.
                        //
                        Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencil(
                            pCameraResources->GetDepthStencil());

                        Microsoft::WRL::ComPtr<IDXGIResource1> depthStencilResource;
                        ASSERT_SUCCEEDED(depthStencil.As(&depthStencilResource));

                        Microsoft::WRL::ComPtr<IDXGISurface2> depthDxgiSurface;
                        ASSERT_SUCCEEDED(depthStencilResource->CreateSubresourceSurface(0, &depthDxgiSurface));

                        auto d3dSurface = Windows::Graphics::DirectX::Direct3D11::CreateDirect3DSurface(
                            depthDxgiSurface.Get());

                        spRenderingParameters->CommitDirect3D11DepthBuffer(
                            d3dSurface);
                    }
#endif
                }

                atLeastOneCameraRendered = true;
            }

            return atLeastOneCameraRendered;
        });
    }

    void AppMainBase::SaveAppState()
    {
        //
        // TODO: Insert code here to save your app state.
        //       This method is called when the app is about to suspend.
        //
        //       For example, store information in the SpatialAnchorStore.
        //
    }

    void AppMainBase::LoadAppState()
    {
        //
        // TODO: Insert code here to load your app state.
        //       This method is called when the app resumes.
        //
        //       For example, load information from the SpatialAnchorStore.
        //
    }

    // Notifies classes that use Direct3D device resources that the device resources
    // need to be released before this method returns.
    void AppMainBase::OnDeviceLost()
    {
    }

    // Notifies classes that use Direct3D device resources that the device resources
    // may now be recreated.
    void AppMainBase::OnDeviceRestored()
    {
    }

    void AppMainBase::OnCameraAdded(
        HolographicSpace^ sender,
        HolographicSpaceCameraAddedEventArgs^ args)
    {
        Deferral^ deferral =
            args->GetDeferral();

        HolographicCamera^ holographicCamera =
            args->Camera;

        concurrency::create_task([this, deferral, holographicCamera]()
        {
            //
            // TODO: Allocate resources for the new camera and load any content specific to
            //       that camera. Note that the render target size (in pixels) is a property
            //       of the HolographicCamera object, and can be used to create off-screen
            //       render targets that match the resolution of the HolographicCamera.
            //

            // Create device-based resources for the holographic camera and add it to the list of
            // cameras used for updates and rendering. Notes:
            //   * Since this function may be called at any time, the AddHolographicCamera function
            //     waits until it can get a lock on the set of holographic camera resources before
            //     adding the new camera. At 60 frames per second this wait should not take long.
            //   * A subsequent Update will take the back buffer from the RenderingParameters of this
            //     camera's CameraPose and use it to create the ID3D11RenderTargetView for this camera.
            //     Content can then be rendered for the HolographicCamera.
            _deviceResources->AddHolographicCamera(
                holographicCamera);

            // Holographic frame predictions will not include any information about this camera until
            // the deferral is completed.
            deferral->Complete();
        });
    }

    void AppMainBase::OnCameraRemoved(
        HolographicSpace^ sender,
        HolographicSpaceCameraRemovedEventArgs^ args)
    {
        concurrency::create_task([this]()
        {
            //
            // TODO: Asynchronously unload or deactivate content resources (not back buffer 
            //       resources) that are specific only to the camera that was removed.
            //
        });

        // Before letting this callback return, ensure that all references to the back buffer 
        // are released.
        // Since this function may be called at any time, the RemoveHolographicCamera function
        // waits until it can get a lock on the set of holographic camera resources before
        // deallocating resources for this camera. At 60 frames per second this wait should
        // not take long.
        _deviceResources->RemoveHolographicCamera(
            args->Camera);
    }
}
