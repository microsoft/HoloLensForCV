To consume UnityArUcoMarkerDetectorPlugin.dll in unity:

1) Create a Plugins\WSA\x86 folder in your unity Assets folder
2) Copy all output files from compiling HoloLensForCV and UnityArUcoMarkerDetectorPlugin into this new plugin directory (Likely in HoloLensForCV\Release\HoloLensForCV and HoloLensForCV\Release\UnityArUcoMarkerDetectorPlugin)
3) Add the UnityArUcoMarkerDetectorPluginAPI.cs file to your unity Assets directory

Note: It's suggested to build the 'Release' flavor for these dll's. OpenCV debug dll's contain various assertions/checks that detract from performance.