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
    std::vector<HoloLensCameraCalibration> ReadCameraCalibrations(
        Windows::Storage::StorageFolder^ recordingFolder)
    {
        std::vector<HoloLensCameraCalibration> cameraCalibrations;

        std::vector<byte> cameraCalibrationBuffer =
            Io::ReadDataSync(
                recordingFolder,
                L"camera_calibration.csv");

        std::istringstream cameraCalibrationStream(
            std::string(
                reinterpret_cast<char*>(&cameraCalibrationBuffer[0]),
                cameraCalibrationBuffer.size()));

        std::string line;
        std::vector<std::string> tokens;
        std::vector<char> lineBuffer;

        bool csvFileHeaderSeen = false;

        while (std::getline(cameraCalibrationStream, line))
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
                1 /* SensorName */ +
                2 /* FocalLength.[x|y] */ +
                2 /* PrincipalPoint.[x|y] */ +
                3 /* RadialDistortion.[x|y|z] */ +
                2 /* TangentialDistortion.[x|y] */ == tokens.size());

            //
            // Skip the CSV file header
            //
            if (!csvFileHeaderSeen)
            {
                ASSERT(0 == strcmp(tokens[0].c_str(), "SensorName"));
                ASSERT(0 == strcmp(tokens[1].c_str(), "FocalLength.x"));
                ASSERT(0 == strcmp(tokens[9].c_str(), "TangentialDistortion.y"));

                csvFileHeaderSeen = true;

                continue;
            }

            HoloLensCameraCalibration cameraCalibration;

            cameraCalibration.SensorName =
                Utf8ToUtf16(
                    tokens[0]);

            float* intrinsicParameters[9] =
            {
                &cameraCalibration.FocalLengthX,
                &cameraCalibration.FocalLengthY,
                &cameraCalibration.PrincipalPointX,
                &cameraCalibration.PrincipalPointY,
                &cameraCalibration.RadialDistortionX,
                &cameraCalibration.RadialDistortionY,
                &cameraCalibration.RadialDistortionZ,
                &cameraCalibration.TangentialDistortionX,
                &cameraCalibration.TangentialDistortionY
            };

            for (size_t i = 0; i < _countof(intrinsicParameters); ++i)
            {
                *intrinsicParameters[i] =
                    static_cast<float>(
                        std::atof(
                            tokens[i + 1].c_str()));
            }

            cameraCalibrations.emplace_back(
                std::move(
                    cameraCalibration));
        }

        return std::move(
            cameraCalibrations);
    }
}
