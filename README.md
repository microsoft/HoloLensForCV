
# Purpose

We want to help people use the HoloLens as a Computer Vision and Robotics research device.
The project was launched at CVPR 2017, and we intend to extend it as new capabilities become
available on the HoloLens.

# Contents

This repository contains reusable components, samples and tools aimed to make it easier to use
HoloLens as a tool for Computer Vision and Robotics research.

   Take a quick look at the [HoloLensForCV UWP component](Shared/HoloLensForCV). This component is
   used by most of our samples and tools to access, stream and record HoloLens sensor data.

   Learn how to [build](https://github.com/Microsoft/HoloLensForCV/wiki/Building) the project and [deploy](https://github.com/Microsoft/HoloLensForCV/wiki/Running-the-samples) the samples.

   Learn how to [use OpenCV on HoloLens](Samples/ComputeOnDevice).

   Learn how to [stream](Tools/Streamer) sensor data and how to [process it online](Samples/ComputeOnDesktop) on a companion PC.

   Learn how to [record](Tools/Recorder) sensor data and how to [process it offline](Samples/BatchProcessing) on a companion PC.

## Universal Windows Platform development

All of the samples require Visual Studio 2017 Update 3 and the Windows Software Development Kit
(SDK) for Windows 10 to build, test, and deploy your Universal Windows Platform apps. In addition,
samples specific to Windows 10 Holographic require a Windows Holographic device to execute.
Windows Holographic devices include the Microsoft HoloLens and the Microsoft HoloLens Emulator.

   [Get a free copy of Visual Studio 2017 Community Edition with support for building Universal
   Windows Platform apps](http://go.microsoft.com/fwlink/p/?LinkID=280676)

   [Install the Windows Holographic tools](https://developer.microsoft.com/windows/mixed-reality/install_the_tools).

   [Learn how to build great apps for Windows by experimenting with our sample apps](https://developer.microsoft.com/en-us/windows/samples).

   [Find more information on Microsoft Developer site](http://go.microsoft.com/fwlink/?LinkID=532421).

Additionally, to stay on top of the latest updates to Windows and the development tools, become
a Windows Insider by joining the Windows Insider Program.

   [Become a Windows Insider](https://insider.windows.com/)

## Using the samples

The easiest way to use these samples without using Git is to download the zip file containing the
current version (using the following link or by clicking the "Download ZIP" button on the repo page).
You can then unzip the entire archive and use the samples in Visual Studio 2017.

   [Download the samples ZIP](../../archive/master.zip)

   **Notes:** 
   * Before you unzip the archive, right-click it, select **Properties**, and then select **Unblock**.
   * Be sure to unzip the entire archive, and not just individual samples. The samples all depend on the [Shared](Shared) folder in the archive.   
   * In Visual Studio 2017, the platform target defaults to ARM, so be sure to change that to x64 or x86 if you want to test on a non-ARM device. 

# Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
