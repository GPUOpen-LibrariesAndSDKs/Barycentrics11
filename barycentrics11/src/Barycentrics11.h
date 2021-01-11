//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <memory>
#include <vector>
#include "amd_ags.h"

class Window;

///////////////////////////////////////////////////////////////////////////////
class Barycentrics11Sample
{
private:
public:

    Barycentrics11Sample() {}
    ~Barycentrics11Sample() {}

    void Run();

private:

    void Initialize();
    void Shutdown();
    void Render();
    void Present();
    void CreateDeviceAndSwapChain();

    std::unique_ptr<Window>             m_window = {};

    IDXGISwapChain*                     m_swapChain = nullptr;
    ID3D11Device*                       m_device = nullptr;
    ID3D11DeviceContext*                m_deviceContext = nullptr;
    ID3D11Texture2D*                    m_renderTarget = nullptr;
    ID3D11RenderTargetView*             m_renderTargetView = nullptr;
    ID3D11Texture2D*                    m_Texture = nullptr;
    ID3D11ShaderResourceView*           m_TextureSRV = nullptr;
    ID3D11SamplerState*                 m_SampleLinear = nullptr;
    ID3D11Buffer*                       m_ConstantBuffer = nullptr;

    ID3D11VertexShader*                 m_TriangleVS = nullptr;
    ID3D11PixelShader*                  m_TrianglePS = nullptr;
    ID3D11PixelShader*                  m_TrianglePSwithIntrinsics = nullptr;

    ID3D11SamplerState*                 m_DummySampler = nullptr;

    AGSContext*                         m_AGSContext = nullptr;
    AGSGPUInfo                          m_AGSGPUInfo = {};
    bool                                m_ExtensionsSupported = false;
    bool                                m_UseExtensions = false;
    float                               m_AspectRatio = 1.0f;
};