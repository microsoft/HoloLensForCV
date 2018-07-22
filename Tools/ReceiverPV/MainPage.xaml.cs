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

#define DBG_ENABLE_VERBOSE_LOGGING

using System;
using System.Diagnostics;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI;
using Windows.UI.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;
using Windows.Graphics.Imaging;

namespace ReceiverPV
{
    public sealed class CameraImageContext
    {
        public CameraImageContext(
            BitmapPixelFormat rawImageFormat,
            int imageWidth,
            int imageHeight,
            bool needsConversion)
        {
            RawImage = new SoftwareBitmap(rawImageFormat, imageWidth, imageHeight, BitmapAlphaMode.Ignore);
            ConvertedImage = new SoftwareBitmap(BitmapPixelFormat.Bgra8, imageWidth, imageHeight, BitmapAlphaMode.Ignore);
            ImageDataUpdateInProgress = false;
            ImageSourceNeedsUpdate = true;
        }

        public SoftwareBitmap RawImage;
        public SoftwareBitmap ConvertedImage;
        public bool ImageDataUpdateInProgress;
        public bool ImageSourceNeedsUpdate;
    };

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        private CameraImageContext _pvCameraImageContext;

        public MainPage()
        {
            this.InitializeComponent();

            _pvCameraImageContext = new CameraImageContext(BitmapPixelFormat.Bgra8, 1280, 720, false /* needsConversion */);

            UpdateImages();
        }

        private void UpdateImages()
        {
            if (_pvCameraImageContext.ImageSourceNeedsUpdate)
            {
                var setPvImageTask = this._pvImage.Dispatcher.RunAsync(
                    CoreDispatcherPriority.Normal,
                    async () =>
                    {
                        if (_pvCameraImageContext.ImageDataUpdateInProgress)
                        {
                            return;
                        }

                        _pvCameraImageContext.ImageDataUpdateInProgress = true;

                        _pvCameraImageContext.ConvertedImage = _pvCameraImageContext.RawImage;

                        _pvCameraImageContext.ImageSourceNeedsUpdate = false;

                        var imageSource = new SoftwareBitmapSource();

                        await imageSource.SetBitmapAsync(
                            _pvCameraImageContext.ConvertedImage);

                        this._pvImage.Source = imageSource;

                        _pvCameraImageContext.ImageDataUpdateInProgress = false;
                    });
            }
        }

        /// <summary>
        /// This is the click handler for the 'ConnectSocket' button.
        /// </summary>
        /// <param name="sender">Object for which the event was generated.</param>
        /// <param name="e">Event's parameters.</param>
        private async void ConnectSocket_Click(object sender, RoutedEventArgs e)
        {
            // By default 'HostNameForConnect' is disabled and host name validation is not required. When enabling the
            // text box validating the host name is required since it was received from an untrusted source
            // (user input). The host name is validated by catching ArgumentExceptions thrown by the HostName
            // constructor for invalid input.
            Windows.Networking.HostName hostName;

            try
            {
                hostName = new Windows.Networking.HostName(HostNameForConnect.Text);
            }
            catch (ArgumentException)
            {
                return;
            }

            var pvCameraSocket = new Windows.Networking.Sockets.StreamSocket();
            pvCameraSocket.Control.KeepAlive = true;

            // Save the socket, so subsequent steps can use it.
            try
            {
                // Connect to the server (by default, the listener we created in the previous step).
                await pvCameraSocket.ConnectAsync(hostName, _pvServiceName.Text);
            }
            catch (Exception exception)
            {
                // If this is an unknown status it means that the error is fatal and retry will likely fail.
                if (Windows.Networking.Sockets.SocketError.GetStatus(exception.HResult) ==
                    Windows.Networking.Sockets.SocketErrorStatus.Unknown)
                {
                    throw;
                }
            }

            OnServerConnectionEstablished(
                pvCameraSocket);
        }

        private async void OnServerConnectionEstablished(
            Windows.Networking.Sockets.StreamSocket socket)
        {
#if DBG_ENABLE_VERBOSE_LOGGING
            System.Diagnostics.Debug.WriteLine(
                "OnServerConnectionEstablished: server connection established");
#endif

            var receiver = new HoloLensForCV.SensorFrameReceiver(
                socket);

            try
            {
                while (true)
                {
                    HoloLensForCV.SensorFrame sensorFrame =
                        await receiver.ReceiveAsync();

#if DBG_ENABLE_VERBOSE_LOGGING
                    System.Diagnostics.Debug.WriteLine(
                        "OnServerConnectionEstablished: seeing a {0}x{1} image of type {2} with timestamp {3}",
                        sensorFrame.SoftwareBitmap.PixelWidth,
                        sensorFrame.SoftwareBitmap.PixelHeight,
                        sensorFrame.FrameType,
                        sensorFrame.Timestamp);
#endif

                    switch (sensorFrame.FrameType)
                    {
                        case HoloLensForCV.SensorType.PhotoVideo:
                            _pvCameraImageContext.RawImage =
                                sensorFrame.SoftwareBitmap;

                            _pvCameraImageContext.ImageSourceNeedsUpdate = true;
                            break;

                        default:
                            throw new Exception("invalid sensor frame type");
                    }

                    UpdateImages();
                }
            }
            catch (Exception exception)
            {
                // If this is an unknown status it means that the error is fatal and retry will likely fail.
                if (Windows.Networking.Sockets.SocketError.GetStatus(exception.HResult) ==
                    Windows.Networking.Sockets.SocketErrorStatus.Unknown)
                {
                    throw;
                }

                System.Diagnostics.Debug.WriteLine(
                    "Read stream failed with error: " + exception.Message);
            }
        }

        /// <summary>
        /// Function delegate that transforms a scanline from an input image to an output image.
        /// </summary>
        /// <param name="pixelWidth"></param>
        /// <param name="inputRowBytes"></param>
        /// <param name="outputRowBytes"></param>
        private unsafe delegate void TransformScanline(int pixelWidth, byte* inputRowBytes, byte* outputRowBytes);

        [ComImport]
        [Guid("5B0D3235-4DBA-4D44-865E-8F1D0E4FD04D")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        unsafe interface IMemoryBufferByteAccess
        {
            void GetBuffer(out byte* buffer, out uint capacity);
        }

        /// <summary>
        /// Transform image into Bgra8 image using given transform method.
        /// </summary>
        /// <param name="softwareBitmap">Input image to transform.</param>
        /// <param name="transformScanline">Method to map pixels in a scanline.</param>
        private static unsafe void TransformBitmap(
            SoftwareBitmap inputBitmap,
            SoftwareBitmap outputBitmap,
            TransformScanline transformScanline)
        {
            using (var input = inputBitmap.LockBuffer(BitmapBufferAccessMode.Read))
            using (var output = outputBitmap.LockBuffer(BitmapBufferAccessMode.Write))
            {
                // Get stride values to calculate buffer position for a given pixel x and y position.
                int inputStride = input.GetPlaneDescription(0).Stride;
                int outputStride = output.GetPlaneDescription(0).Stride;
                int pixelWidth = inputBitmap.PixelWidth;
                int pixelHeight = inputBitmap.PixelHeight;

                using (var outputReference = output.CreateReference())
                using (var inputReference = input.CreateReference())
                {
                    // Get input and output byte access buffers.
                    byte* inputBytes;
                    uint inputCapacity;
                    ((IMemoryBufferByteAccess)inputReference).GetBuffer(out inputBytes, out inputCapacity);
                    byte* outputBytes;
                    uint outputCapacity;
                    ((IMemoryBufferByteAccess)outputReference).GetBuffer(out outputBytes, out outputCapacity);

                    // Iterate over all pixels and store converted value.
                    for (int y = 0; y < pixelHeight; y++)
                    {
                        byte* inputRowBytes = inputBytes + y * inputStride;
                        byte* outputRowBytes = outputBytes + y * outputStride;

                        transformScanline(pixelWidth, inputRowBytes, outputRowBytes);
                    }
                }
            }
        }

        /// <summary>
        /// A helper class to manage look-up-table for pseudo-colors.
        /// </summary>
        private static class PseudoColorHelper
        {
#region Constructor, private members and methods

            private const int TableSize = 1024;   // Look up table size
            private static readonly uint[] PseudoColorTable;
            private static readonly uint[] InfraredRampTable;

            // Color palette mapping value from 0 to 1 to blue to red colors.
            private static readonly Color[] ColorRamp =
            {
                Color.FromArgb(a:0xFF, r:0x7F, g:0x00, b:0x00),
                Color.FromArgb(a:0xFF, r:0xFF, g:0x00, b:0x00),
                Color.FromArgb(a:0xFF, r:0xFF, g:0x7F, b:0x00),
                Color.FromArgb(a:0xFF, r:0xFF, g:0xFF, b:0x00),
                Color.FromArgb(a:0xFF, r:0x7F, g:0xFF, b:0x7F),
                Color.FromArgb(a:0xFF, r:0x00, g:0xFF, b:0xFF),
                Color.FromArgb(a:0xFF, r:0x00, g:0x7F, b:0xFF),
                Color.FromArgb(a:0xFF, r:0x00, g:0x00, b:0xFF),
                Color.FromArgb(a:0xFF, r:0x00, g:0x00, b:0x7F),
            };

            static PseudoColorHelper()
            {
                PseudoColorTable = InitializePseudoColorLut();
                InfraredRampTable = InitializeInfraredRampLut();
            }

            /// <summary>
            /// Maps an input infrared value between [0, 1] to corrected value between [0, 1].
            /// </summary>
            /// <param name="value">Input value between [0, 1].</param>
            [MethodImpl(MethodImplOptions.AggressiveInlining)]  // Tell the compiler to inline this method to improve performance
            private static uint InfraredColor(float value)
            {
                int index = (int)(value * TableSize);
                index = index < 0 ? 0 : index > TableSize - 1 ? TableSize - 1 : index;
                return InfraredRampTable[index];
            }

            /// <summary>
            /// Initializes the pseudo-color look up table for infrared pixels
            /// </summary>
            private static uint[] InitializeInfraredRampLut()
            {
                uint[] lut = new uint[TableSize];
                for (int i = 0; i < TableSize; i++)
                {
                    var value = (float)i / TableSize;
                    // Adjust to increase color change between lower values in infrared images
                    var alpha = (float)Math.Pow(1 - value, 12);
                    lut[i] = ColorRampInterpolation(alpha);
                }
                return lut;
            }

            /// <summary>
            /// Initializes pseudo-color look up table for depth pixels
            /// </summary>
            private static uint[] InitializePseudoColorLut()
            {
                uint[] lut = new uint[TableSize];
                for (int i = 0; i < TableSize; i++)
                {
                    lut[i] = ColorRampInterpolation((float)i / TableSize);
                }
                return lut;
            }

            /// <summary>
            /// Maps a float value to a pseudo-color pixel
            /// </summary>
            private static uint ColorRampInterpolation(float value)
            {
                // Map value to surrounding indexes on the color ramp
                int rampSteps = ColorRamp.Length - 1;
                float scaled = value * rampSteps;
                int integer = (int)scaled;
                int index =
                    integer < 0 ? 0 :
                    integer >= rampSteps - 1 ? rampSteps - 1 :
                    integer;
                Color prev = ColorRamp[index];
                Color next = ColorRamp[index + 1];

                // Set color based on ratio of closeness between the surrounding colors
                uint alpha = (uint)((scaled - integer) * 255);
                uint beta = 255 - alpha;
                return
                    ((prev.A * beta + next.A * alpha) / 255) << 24 | // Alpha
                    ((prev.R * beta + next.R * alpha) / 255) << 16 | // Red
                    ((prev.G * beta + next.G * alpha) / 255) << 8 |  // Green
                    ((prev.B * beta + next.B * alpha) / 255);        // Blue
            }

            /// <summary>
            /// Maps a value in [0, 1] to a pseudo RGBA color.
            /// </summary>
            /// <param name="value">Input value between [0, 1].</param>
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            private static uint PseudoColor(float value)
            {
                int index = (int)(value * TableSize);
                index = index < 0 ? 0 : index > TableSize - 1 ? TableSize - 1 : index;
                return PseudoColorTable[index];
            }

            /// <summary>
            /// Maps a value in [0, 1] to a shade of gray.
            /// </summary>
            /// <param name="value">Input value between [0, 1].</param>
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            private static uint Grayscale(float value)
            {
                if (value < 0.0f || value > 1.0f)
                {
                    return 0xFF000000;
                }

                uint intensity = (uint)(value * 255.0f + 0.5f);

                return 0xFF000000 | (intensity << 16) | (intensity << 8) | intensity;
            }

            #endregion

            /// <summary>
            /// Maps each pixel in a scanline from a 16 bit depth value to a pseudo-color pixel.
            /// </summary>
            /// <param name="pixelWidth">Width of the input scanline, in pixels.</param>
            /// <param name="inputRowBytes">Pointer to the start of the input scanline.</param>
            /// <param name="outputRowBytes">Pointer to the start of the output scanline.</param>
            /// <param name="depthScale">Physical distance that corresponds to one unit in the input scanline.</param>
            /// <param name="minReliableDepth">Shortest distance at which the sensor can provide reliable measurements.</param>
            /// <param name="maxReliableDepth">Furthest distance at which the sensor can provide reliable measurements.</param>
            public static unsafe void PseudoColorForDepth(int pixelWidth, byte* inputRowBytes, byte* outputRowBytes, float depthScale, float minReliableDepth, float maxReliableDepth)
            {
                // Visualize space in front of your desktop.
                float minInMeters = minReliableDepth * depthScale;
                float maxInMeters = maxReliableDepth * depthScale;
                float rangeReciprocal = 1.0f / maxInMeters;

                ushort* inputRow = (ushort*)inputRowBytes;
                uint* outputRow = (uint*)outputRowBytes;
                for (int x = 0; x < pixelWidth; x++)
                {
                    var depth = inputRow[x] * depthScale;

                    if (depth < minInMeters || depth > maxInMeters)
                    {
                        // Map invalid depth values to transparent pixels.
                        // This happens when depth information cannot be calculated, e.g. when objects are too close.
                        outputRow[x] = 0;
                    }
                    else
                    {
                        var alpha = depth * rangeReciprocal;

#if false
                        outputRow[x] = Grayscale(alpha);
#else
                        outputRow[x] = PseudoColor(alpha);
#endif
                    }
                }
            }

            /// <summary>
            /// Maps each pixel in a scanline from a 8 bit infrared value to a pseudo-color pixel.
            /// </summary>
            /// /// <param name="pixelWidth">Width of the input scanline, in pixels.</param>
            /// <param name="inputRowBytes">Pointer to the start of the input scanline.</param>
            /// <param name="outputRowBytes">Pointer to the start of the output scanline.</param>
            public static unsafe void PseudoColorFor8BitInfrared(
                int pixelWidth, byte* inputRowBytes, byte* outputRowBytes)
            {
                byte* inputRow = inputRowBytes;
                uint* outputRow = (uint*)outputRowBytes;
                for (int x = 0; x < pixelWidth; x++)
                {
                    outputRow[x] = InfraredColor(inputRow[x] / (float)Byte.MaxValue);
                }
            }

            /// <summary>
            /// Maps each pixel in a scanline from a 16 bit infrared value to a pseudo-color pixel.
            /// </summary>
            /// <param name="pixelWidth">Width of the input scanline.</param>
            /// <param name="inputRowBytes">Pointer to the start of the input scanline.</param>
            /// <param name="outputRowBytes">Pointer to the start of the output scanline.</param>
            public static unsafe void PseudoColorFor16BitInfrared(int pixelWidth, byte* inputRowBytes, byte* outputRowBytes)
            {
                ushort* inputRow = (ushort*)inputRowBytes;
                uint* outputRow = (uint*)outputRowBytes;
                for (int x = 0; x < pixelWidth; x++)
                {
                    outputRow[x] = InfraredColor(inputRow[x] / (float)UInt16.MaxValue);
                }
            }
        }
    }
}
