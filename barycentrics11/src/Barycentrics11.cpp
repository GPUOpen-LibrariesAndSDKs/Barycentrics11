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
#include "Barycentrics11.h"

#include <dxgi1_2.h>
#include <iostream>
#include <d3dcompiler.h>
#include <algorithm>
#include <iostream>
#include "Window.h"

#pragma warning( disable : 4100 ) // disable unreference formal parameter warnings for /W4 builds

#ifdef max 
#undef max
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p) = nullptr; } }
#endif

#ifdef _DEBUG
#pragma comment (lib, "../../../ags_lib/lib/amd_ags_x64_2019_MDd.lib" )
#else
#pragma comment (lib, "../../../ags_lib/lib/amd_ags_x64_2019_MD.lib" )
#endif

HRESULT WINAPI CompileFromFile( LPCWSTR pFileName,
                                    const D3D_SHADER_MACRO* pDefines,
                                    LPCSTR pEntrypoint, LPCSTR pTarget,
                                    UINT Flags1, UINT Flags2,
                                    ID3DBlob** ppCode )
{
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    Flags1 |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob = nullptr;

    HRESULT hr = D3DCompileFromFile( pFileName, pDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                             pEntrypoint, pTarget, Flags1, Flags2,
                             ppCode, &pErrorBlob );

#pragma warning( suppress : 6102 )
    if ( pErrorBlob )
    {
        OutputDebugStringA( reinterpret_cast<const char*>( pErrorBlob->GetBufferPointer() ) );
        pErrorBlob->Release();
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
void Barycentrics11Sample::Render()
{
    static int counter = 0;
    counter++;

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    m_deviceContext->Map( m_ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
    float* f = static_cast<float*>(MappedResource.pData);
    f[ 0 ] = 0.2f + std::abs( sinf( static_cast<float> (counter) / 64.0f ) );
    f[ 1 ] = m_AspectRatio;
    m_deviceContext->Unmap( m_ConstantBuffer, 0 );

    float ClearColor[4] = { 0.042f, 0.042f, 0.042f, 1.0f };
    m_deviceContext->ClearRenderTargetView( m_renderTargetView, ClearColor );
    m_deviceContext->OMSetRenderTargets ( 1, &m_renderTargetView, nullptr );

    m_deviceContext->IASetVertexBuffers( 0, 0, nullptr, 0, nullptr );
    m_deviceContext->VSSetConstantBuffers( 0, 1, &m_ConstantBuffer );
    m_deviceContext->PSSetSamplers( 0, 1, &m_SampleLinear );
    m_deviceContext->PSSetShaderResources( 0, 1, &m_TextureSRV );

    m_deviceContext->VSSetShader( m_TriangleVS, nullptr, 0 );
    m_deviceContext->PSSetShader( m_UseExtensions ? m_TrianglePSwithIntrinsics : m_TrianglePS, nullptr, 0 );
    m_deviceContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    m_deviceContext->Draw( 3, 0 );
}


///////////////////////////////////////////////////////////////////////////////
void Barycentrics11Sample::Run()
{
    Initialize ();

    while (1)
    {
        if (m_window->MessagePump() == false)
        {
            break;
        }

        Render ();
        Present ();
    }

    Shutdown ();
}

///////////////////////////////////////////////////////////////////////////////
/**
Present the current frame by swapping the back buffer, then move to the
next back buffer and also signal the fence for the current queue slot entry.
*/
void Barycentrics11Sample::Present ()
{
    m_swapChain->Present (1, 0);
}


///////////////////////////////////////////////////////////////////////////////
void Barycentrics11Sample::Initialize ()
{
    m_window.reset (new Window ("Barycentrics11 v2.0", 1920, 1080));

    // Call this before device creation
    agsInitialize( AGS_MAKE_VERSION( AMD_AGS_VERSION_MAJOR, AMD_AGS_VERSION_MINOR, AMD_AGS_VERSION_PATCH ), nullptr, &m_AGSContext, &m_AGSGPUInfo );

    CreateDeviceAndSwapChain();

    // Setup constant buffer
    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;
    Desc.ByteWidth = 4 * sizeof( float );
    m_device->CreateBuffer( &Desc, NULL, &m_ConstantBuffer );

    // Create sampler state
    D3D11_SAMPLER_DESC SamDesc;
    SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.MipLODBias = 0.0f;
    SamDesc.MaxAnisotropy = 1;
    SamDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    SamDesc.BorderColor[0] = SamDesc.BorderColor[1] = SamDesc.BorderColor[2] = SamDesc.BorderColor[3] = 0;
    SamDesc.MinLOD = 0;
    SamDesc.MaxLOD = D3D11_FLOAT32_MAX;
    m_device->CreateSamplerState( &SamDesc, &m_SampleLinear );

    SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    m_device->CreateSamplerState( &SamDesc, &m_DummySampler );

    CD3D11_TEXTURE2D_DESC texDesc( DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 256, 256, 1, 1, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE );

    DWORD* imageData = new DWORD[ texDesc.Width * texDesc.Height ];
    for ( unsigned int y = 0; y < texDesc.Height; y++ )
    {
        for ( unsigned int x = 0; x < texDesc.Width; x++ )
        {
            int xx = x / 32;
            int yy = y / 32;

            imageData[ x + y*texDesc.Width ] = (xx % 2 == yy % 2) ? 0xffffffff : 0xff000000;
        }
    }

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = imageData;
    data.SysMemPitch = texDesc.Width * sizeof( DWORD );
    m_device->CreateTexture2D( &texDesc, &data, &m_Texture );
    m_device->CreateShaderResourceView( m_Texture, nullptr, &m_TextureSRV );

    delete [] imageData;

    ID3DBlob* pBlob = nullptr;

    // VS
    CompileFromFile( L"..\\src\\shaders.hlsl", nullptr, "VS_main", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlob );
    m_device->CreateVertexShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &m_TriangleVS );
    SAFE_RELEASE( pBlob );

    // Default PS
    CompileFromFile( L"..\\src\\shaders.hlsl", nullptr, "PS_main", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlob );
    m_device->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &m_TrianglePS );
    SAFE_RELEASE( pBlob );

    // PS with extension support
    const D3D_SHADER_MACRO macros[] = { { "AMD_USE_SHADER_INTRINSICS", "1" }, { nullptr, nullptr } };
    CompileFromFile( L"..\\src\\shaders.hlsl", macros, "PS_main", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlob );
    m_device->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &m_TrianglePSwithIntrinsics );
    SAFE_RELEASE( pBlob );
}

///////////////////////////////////////////////////////////////////////////////
void Barycentrics11Sample::Shutdown ()
{
    // Clear things out to avoid false-positive D3D debug runtime ref-count warnings.
    m_swapChain->SetFullscreenState (FALSE, 0);
    m_deviceContext->ClearState ();
    m_deviceContext->Flush ();

    m_renderTargetView->Release();
    m_renderTarget->Release();

    SAFE_RELEASE( m_TextureSRV );
    SAFE_RELEASE( m_Texture );
    SAFE_RELEASE( m_SampleLinear );
    SAFE_RELEASE( m_DummySampler );
    SAFE_RELEASE( m_ConstantBuffer );
    SAFE_RELEASE( m_TriangleVS );
    SAFE_RELEASE( m_TrianglePS );
    SAFE_RELEASE( m_TrianglePSwithIntrinsics );

    m_swapChain->Release();

    if ( m_AGSGPUInfo.devices[ 0 ].vendorId == 0x1002 )
    {
        agsDriverExtensionsDX11_DestroyDevice( m_AGSContext, m_device, nullptr, m_deviceContext, nullptr );
    }
    else
    {
        m_deviceContext->Release();
        m_device->Release();
    }

    agsDeInitialize(m_AGSContext);
}

///////////////////////////////////////////////////////////////////////////////
void Barycentrics11Sample::CreateDeviceAndSwapChain ()
{
    DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.Height = 1080;
    swapChainDesc.BufferDesc.Width = 1920;
    swapChainDesc.OutputWindow = m_window->GetHWND ();
    swapChainDesc.Windowed = true;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.SampleDesc.Count = 1;

    AGSDX11DeviceCreationParams creationParams = 
    {
        nullptr,                    /* default adapter */
        D3D_DRIVER_TYPE_HARDWARE,
        0,                          /* Module for driver, must be null if hardware*/
#ifdef _DEBUG
        D3D11_CREATE_DEVICE_DEBUG,  /* debug device for debug builds */
#else
        0,                          /* No flags */
#endif       
        nullptr,                    /* no feature levels */
        0,                          /* no feature levels */
        D3D11_SDK_VERSION,
        &swapChainDesc
    };

    if ( m_AGSGPUInfo.devices[ 0 ].vendorId == 0x1002 )
    {
        AGSDX11ExtensionParams extensionParams = {};
        AGSDX11ReturnedParams returnedParams = {};
        if ( agsDriverExtensionsDX11_CreateDevice( m_AGSContext, &creationParams, &extensionParams, &returnedParams ) == AGS_SUCCESS )
        {
            m_device = returnedParams.pDevice;
            m_deviceContext = returnedParams.pImmediateContext;
            m_swapChain = returnedParams.pSwapChain;

            m_UseExtensions = returnedParams.extensionsSupported.intrinsics16;
        }
    }
    else
    {
        D3D11CreateDeviceAndSwapChain( 
            creationParams.pAdapter,
            creationParams.DriverType,
            creationParams.Software,
            creationParams.Flags,
            creationParams.pFeatureLevels,
            creationParams.FeatureLevels,
            creationParams.SDKVersion,
            &swapChainDesc,
            &m_swapChain,
            &m_device,
            nullptr,
            &m_deviceContext);
    }

    m_swapChain->GetBuffer (0, IID_PPV_ARGS (&m_renderTarget));
    D3D11_RENDER_TARGET_VIEW_DESC desc = {};
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    m_device->CreateRenderTargetView (m_renderTarget, &desc, &m_renderTargetView);

    m_AspectRatio = (float)m_window->GetWidth() / (float)m_window->GetHeight();

    D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (float)m_window->GetWidth(), (float)m_window->GetHeight(), 0.0f, 1.0f };
    m_deviceContext->RSSetViewports( 1, &viewport );
}

int WinMain (
    _In_ HINSTANCE,
    _In_opt_ HINSTANCE,
    _In_ LPSTR,
    _In_ int
    )
{
    // Enable run-time memory check for debug builds.
    // (When _DEBUG is not defined, calls to _CrtSetDbgFlag are removed during preprocessing.)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    Barycentrics11Sample sample;
    sample.Run();

    return 0;
}