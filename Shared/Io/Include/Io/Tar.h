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

#include <fstream>

namespace Io
{
    void CreateTarball(
        _In_ Windows::Storage::StorageFolder^ sourceFolder,
        _In_ const std::vector<std::wstring>& sourceFileNames,
        _In_ Windows::Storage::StorageFolder^ tarballFolder,
        _In_ const std::wstring& tarballFileName);

	// Class to create tarball, which allows for incremental
	// streaming of files into the archive.
	class Tarball
	{
	public:
		Tarball(_In_ const std::wstring& tarballFileName);
		~Tarball();

		// Close the tarball.
		void Close();

		// Add a file to the tarball.
		void AddFile(
			_In_ const std::wstring& fileName,
			_In_ const uint8_t* fileData,
			_In_ const size_t fileSize);

	private:
		// The file handler to the tarball.
		std::ofstream _tarballFile;
	};
}
