[![pipeline status](http://isvgit.amd.com/gpuopen/Barycentrics11/badges/master/pipeline.svg)](http://isvgit.amd.com/gpuopen/Barycentrics11/commits/master)

# AMD Barycentric Shader Extension Sample

This sample shows how to use the GCN shader extensions for D3D11 to access the barycentric intrinsic instruction in an HLSL pixel shader.

### Prerequisites
* AMD Radeon&trade; GCN-based GPU (HD 7000 series or newer)
* 64-bit Windows&reg; 10
* Visual Studio&reg; 2019
* Radeon Software Crimson Edition 16.9.1 (driver version 16.40) or later

### Sample Overview
This sample renders a triangle zooming in and out. The triangle uses a checker board texture modulated by the barycentric coordinates as RGB colors.

### Points of Interest
* This sample uses a driver extension to enable the use of instrinsic instructions.
  * The driver extension is accessed through the AMD GPU Services (AGS) library.
  * For more information on AGS, including samples, visit the AGS SDK repository: https://github.com/GPUOpen-LibrariesAndSDKs/AGS_SDK
* The shader compiler should not use the D3DCOMPILE_SKIP_OPTIMIZATION option, otherwise it will not work.
* Other instrinsics are also available.

### Attribution
* AMD, the AMD Arrow logo, Radeon, and combinations thereof are either registered trademarks or trademarks of Advanced Micro Devices, Inc. in the United States and/or other countries.
* Microsoft, Direct3D, DirectX, Visual Studio, and Windows are either registered trademarks or trademarks of Microsoft Corporation in the United States and/or other countries.
