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

namespace BatchProcessing
{
    void HoloLensCameraFrame::Load()
    {
        std::vector<byte> imageBuffer =
            Io::ReadDataSync(
                RecordingFolder,
                FileName);

        Image = cv::Mat(
            Height /* _rows */,
            Width /* _cols */,
            PixelFormat,
            imageBuffer.data(),
            cv::Mat::AUTO_STEP).clone();
    }

    void HoloLensCameraFrame::Unload()
    {
        Image.release();
    }

    std::vector<HoloLensCameraFrame> DiscoverCameraFrames(
        _In_ Windows::Storage::StorageFolder^ recordingFolder,
        _In_ const std::wstring& manifestFileName)
    {
        std::vector<HoloLensCameraFrame> cameraFrames;

        std::vector<byte> cameraFramesBuffer =
            Io::ReadDataSync(
                recordingFolder,
                manifestFileName);

        std::istringstream cameraFramesStream(
            std::string(
                reinterpret_cast<char*>(&cameraFramesBuffer[0]),
                cameraFramesBuffer.size()));

        std::string line;
        std::vector<std::string> tokens;
        std::vector<char> lineBuffer;

        bool csvFileHeaderSeen = false;

        while (std::getline(cameraFramesStream, line))
        {
            //
            // Skip comments and empty lines
            //
            line.erase(
                line.find_last_not_of(
                    " \t\r\n") + 1);

            if (line.empty() || line[0] == '#')
            {
                continue;
            }

            Io::TokenizeString(
                line,
                "," /* delimiter */,
                tokens,
                lineBuffer);

            ASSERT(
                1 /* Timestamp */ +
                1 /* ImageFileName */ +
                16 /* FrameToOrigin.m(1..4)(1..4) */ +
                16 /* CameraViewTransform.m(1..4)(1..4) */ +
                16 /* CameraProjectionTransform.m(1..4)(1..4) */ == tokens.size());

            //
            // Skip the CSV file header
            //
            if (!csvFileHeaderSeen)
            {
                ASSERT(0 == strcmp(tokens[0].c_str(), "Timestamp"));
                ASSERT(0 == strcmp(tokens[1].c_str(), "ImageFileName"));
                ASSERT(0 == strcmp(tokens[17].c_str(), "FrameToOrigin.m44"));
                ASSERT(0 == strcmp(tokens[33].c_str(), "CameraViewTransform.m44"));
                ASSERT(0 == strcmp(tokens[49].c_str(), "CameraProjectionTransform.m44"));

                csvFileHeaderSeen = true;

                continue;
            }

            HoloLensCameraFrame cameraFrame;

            cameraFrame.Timestamp =
                std::atoll(
                    tokens[0].c_str());

            cameraFrame.RecordingFolder =
                recordingFolder;

            cameraFrame.FileName =
                Utf8ToUtf16(
                    tokens[1]);

            cameraFrame.FrameToOrigin =
                cv::Mat::eye(
                    4 /* rows */,
                    4 /* cols */,
                    CV_32F /* type */);

            cameraFrame.CameraViewTransform =
                cv::Mat::eye(
                    4 /* rows */,
                    4 /* cols */,
                    CV_32F /* type */);

            cameraFrame.CameraProjectionTransform =
                cv::Mat::eye(
                    4 /* rows */,
                    4 /* cols */,
                    CV_32F /* type */);

            //
            // FrameToOrigin follows the timestamp and image file name fields:
            //
            for (int32_t j = 0; j < 4; ++j)
            {
                for (int32_t i = 0; i < 4; ++i)
                {
                    cameraFrame.FrameToOrigin.at<float>(j, i) =
                        static_cast<float>(
                            std::atof(
                                tokens[j * 4 + i + 2].c_str()));
                }
            }

            for (int32_t j = 0; j < 4; ++j)
            {
                for (int32_t i = 0; i < 4; ++i)
                {
                    cameraFrame.CameraViewTransform.at<float>(j, i) =
                        static_cast<float>(
                            std::atof(
                                tokens[j * 4 + i + 2].c_str()));
                }
            }

            for (int32_t j = 0; j < 4; ++j)
            {
                for (int32_t i = 0; i < 4; ++i)
                {
                    cameraFrame.CameraProjectionTransform.at<float>(j, i) =
                        static_cast<float>(
                            std::atof(
                                tokens[j * 4 + i + 2].c_str()));
                }
            }

            //TODO: add the width/height/pixel format to the manifest or camera calibration CSV
            cameraFrame.Width = 1280;
            cameraFrame.Height = 720;
            cameraFrame.PixelFormat = CV_8SC4; // BGRA

            cameraFrames.emplace_back(
                std::move(
                    cameraFrame));
        }

        return std::move(
            cameraFrames);
    }
}
