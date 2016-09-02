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

#include "DXUT.h"
#include "DXUTCamera.h"
#include "DXUTGui.h"
#include "DXUTSettingsDlg.h"
#include "SDKmisc.h"
#include "SDKMesh.h"
#include "resource.h"
#include "amd_ags.h"


#pragma warning( disable : 4100 ) // disable unreference formal parameter warnings for /W4 builds


//--------------------------------------------------------------------------------------
// Modified D3DSettingsDlg which will move the dialog to the main display instead of
// showing the whole screen dialog.
//--------------------------------------------------------------------------------------
class CEF_D3DSettingsDlg: public CD3DSettingsDlg
{
public:
    HRESULT             OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc );
};
//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager          g_DialogResourceManager;    // manager for shared resources of dialogs
CEF_D3DSettingsDlg                  g_D3DSettingsDlg;           // modified device settings dialog which will always shows on the main display
CDXUTDialog                         g_HUD;                      // manages the 3D UI
CDXUTTextHelper*                    g_pTxtHelper = NULL;
float                               g_AspectRatio = 1.0f;
CDXUTCheckBox*                      g_UseExtensionsCheckBox = nullptr;

ID3D11Texture2D*                    g_Texture = nullptr;
ID3D11ShaderResourceView*           g_TextureSRV = nullptr;
ID3D11SamplerState*                 g_SampleLinear = nullptr;
ID3D11Buffer*                       g_ConstantBuffer = nullptr;

ID3D11InputLayout*                  g_TriangleVertexLayout = nullptr;
ID3D11VertexShader*                 g_TriangleVS = nullptr;
ID3D11PixelShader*                  g_TrianglePS = nullptr;
ID3D11PixelShader*                  g_TrianglePSwithIntrinsics = nullptr;
ID3D11Buffer*                       g_TriangleVB = nullptr;

ID3D11SamplerState*                 g_DummySampler = nullptr;

AGSContext*                         g_AGSContext = nullptr;
AGSGPUInfo                          g_AGSGPUInfo;
bool                                g_ExtensionsSupported = false;
bool                                g_UseExtensions = false;

struct Vertex
{
    float position[ 3 ];
    float uv[ 2 ];
};

void InitExtensions()
{
    // Avoid trying to initialize the shader extensions if we are not a GCN GPU
    if ( g_AGSGPUInfo.architectureVersion == AGSGPUInfo::ArchitectureVersion_GCN )
    {
        unsigned int extensionsSupported = 0;
        if ( agsDriverExtensionsDX11_Init( g_AGSContext, DXUTGetD3D11Device(), 7, &extensionsSupported ) == AGS_SUCCESS )
        {
            if ( extensionsSupported & AGS_DX11_EXTENSION_INTRINSIC_BARYCENTRICS )
            {
                // the Barycentric extension is supported so use this codepath
                g_ExtensionsSupported = true;
                g_UseExtensions = true;
            }
        }
    }
}


//--------------------------------------------------------------------------------------
// Retrieve the main display info and place the dialog on main display
// whenever the resolution is changed.
//--------------------------------------------------------------------------------------
HRESULT CEF_D3DSettingsDlg::OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc )
{
    m_Dialog.SetLocation( 0, 0 );
    m_Dialog.SetSize( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    m_Dialog.SetBackgroundColors( D3DCOLOR_ARGB( 255, 98, 138, 206 ),
                                  D3DCOLOR_ARGB( 255, 54, 105, 192 ),
                                  D3DCOLOR_ARGB( 255, 54, 105, 192 ),
                                  D3DCOLOR_ARGB( 255, 10,  73, 179 ) );

    m_RevertModeDialog.SetLocation( 0, 0 );
    m_RevertModeDialog.SetSize( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    m_RevertModeDialog.SetBackgroundColors( D3DCOLOR_ARGB( 255, 98, 138, 206 ),
                                            D3DCOLOR_ARGB( 255, 54, 105, 192 ),
                                            D3DCOLOR_ARGB( 255, 54, 105, 192 ),
                                            D3DCOLOR_ARGB( 255, 10,  73, 179 ) );

    return S_OK;
}
//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
enum BARYCENTRICS11_SAMPLE_IDC
{
    IDC_TOGGLEFULLSCREEN    = 1,
    IDC_TOGGLEREF,
    IDC_CHANGEDEVICE,
    IDC_USE_EXTENSIONS
};

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK KeyboardProc( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );

// Direct3D 11 callbacks
bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
HRESULT CALLBACK OnD3D11SwapChainResized( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );

void RenderText();


//--------------------------------------------------------------------------------------
// Initialize the app
//--------------------------------------------------------------------------------------
void InitApp()
{
    g_D3DSettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_HUD.SetCallback( OnGUIEvent );
}


void InitSampleUI()
{
    int iY = 40;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 5, iY, 150, 22 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 5, iY += 24, 150, 22, VK_F2 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 5, iY += 24, 150, 22, VK_F3 );
    g_HUD.AddCheckBox( IDC_USE_EXTENSIONS, L"Use Extensions (E)", 5, iY += 24, 150, 22, g_UseExtensions, 'E', false, &g_UseExtensionsCheckBox );
    g_UseExtensionsCheckBox->SetEnabled( g_ExtensionsSupported );
}


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // Initialise AGS lib
    agsInit( &g_AGSContext, nullptr, &g_AGSGPUInfo );

    if ( g_AGSGPUInfo.architectureVersion != AGSGPUInfo::ArchitectureVersion_GCN )
    {
        MessageBox( 0, L"This GPU is not GCN so is not capable of running these shader extensions", L"Information", MB_OK );
    }
    else
    {
        int major = 0;
        int minor = 0;
        int patch = 0;
        sscanf_s( g_AGSGPUInfo.radeonSoftwareVersion, "%d.%d.%d", &major, &minor, &patch );
        if ( major < 16 || ( major == 16 && minor < 5 ) || ( major == 16 && minor == 5 && patch < 2 ) )
        {
            MessageBox( 0, L"You will need to update your driver to 16.5.2 or later in order to run these shader extensions", L"Information", MB_OK );
        }
    }

    // DXUT will create and use the best device
    // that is available on the system depending on which D3D callbacks are set below
    DXUTSetIsInGammaCorrectMode( false );

    // Set DXUT callbacks
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( KeyboardProc );

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11SwapChainResized );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );

    InitApp();
    DXUTInit( true, true ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"Barycentrics11 v1.0" );

    int width = 1920;
    int height = 1080;
    bool windowed = true;

    DXUTCreateDevice(D3D_FEATURE_LEVEL_11_0, windowed, width, height );

    InitSampleUI();

    DXUTMainLoop(); // Enter into the DXUT render loop

    DXUTShutdown();

    // Clean up AGS lib
    agsDeInit( g_AGSContext );

    return DXUTGetExitCode();
}

//--------------------------------------------------------------------------------------
// Called right before creating a D3D11 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    pDeviceSettings->d3d11.SyncInterval = 1;
    g_D3DSettingsDlg.GetDialogControl()->GetComboBox( DXUTSETTINGSDLG_D3D11_PRESENT_INTERVAL )->SetEnabled( false );

    return true;
}

//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if (*pbNoFurtherProcessing)
    {
        return 0;
    }

    // Pass messages to settings dialog if its active
    if (g_D3DSettingsDlg.IsActive())
    {
        g_D3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if (*pbNoFurtherProcessing)
    {
        return 0;
    }

    return 0;
}
//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK KeyboardProc( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
}
//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch (nControlID)
    {
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen();
            break;
        case IDC_TOGGLEREF:
            DXUTToggleREF();
            break;
        case IDC_CHANGEDEVICE:
            g_D3DSettingsDlg.SetActive( !g_D3DSettingsDlg.IsActive() );
            break;
        case IDC_USE_EXTENSIONS:
            if ( g_ExtensionsSupported )
            {
                g_UseExtensions = g_UseExtensionsCheckBox->GetChecked();
            }
            break;
    }
}
//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}
//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
    HRESULT hr;

    // It is important that the extension mechanism is initialized before we create the shaders.
    InitExtensions();

     // Get device context
    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();

    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );

     g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

    const Vertex vertices[ 4 ] =
    {
        { {  0.0f,   0.25f, 0.0f }, { 0.5f, 0.0f } },
        { {  0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f } },
        { { -0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f } }
    };

    // Create Vertex Buffer
    CD3D11_BUFFER_DESC vbDesc( sizeof( vertices ), D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_IMMUTABLE );

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices;
    V_RETURN( pd3dDevice->CreateBuffer( &vbDesc, &vbData, &g_TriangleVB ) );

    // Setup constant buffer
    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;
    Desc.ByteWidth = 4 * sizeof( float );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &g_ConstantBuffer ) );

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
    V_RETURN( pd3dDevice->CreateSamplerState( &SamDesc, &g_SampleLinear ) )

    SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    V_RETURN( pd3dDevice->CreateSamplerState( &SamDesc, &g_DummySampler ) )

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
    V_RETURN( pd3dDevice->CreateTexture2D( &texDesc, &data, &g_Texture ) );
    V_RETURN( pd3dDevice->CreateShaderResourceView( g_Texture, nullptr, &g_TextureSRV ) );

    delete [] imageData;

    ID3DBlob* pBlob = nullptr;

    // VS
    V_RETURN( DXUTCompileFromFile( L"..\\src\\Shaders\\Barycentrics11.hlsl", nullptr, "VS_main", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlob ) );
    V_RETURN( pd3dDevice->CreateVertexShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_TriangleVS ) );
    // Define our scene vertex data layout
    const D3D11_INPUT_ELEMENT_DESC VertexLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    V_RETURN( pd3dDevice->CreateInputLayout( VertexLayout, ARRAYSIZE( VertexLayout ), pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &g_TriangleVertexLayout ) );
    SAFE_RELEASE( pBlob );

    // Default PS
    V_RETURN( DXUTCompileFromFile( L"..\\src\\Shaders\\Barycentrics11.hlsl", nullptr, "PS_main", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlob ) );
    V_RETURN( pd3dDevice->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_TrianglePS ) );
    SAFE_RELEASE( pBlob );

    // PS with extension support
    const D3D_SHADER_MACRO macros[] = { { "AMD_USE_SHADER_INTRINSICS", "1" }, { nullptr, nullptr } };
    V_RETURN( DXUTCompileFromFile( L"..\\src\\Shaders\\Barycentrics11.hlsl", macros, "PS_main", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlob ) );
    V_RETURN( pd3dDevice->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_TrianglePSwithIntrinsics ) );
    SAFE_RELEASE( pBlob );

    return S_OK;
}
//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11SwapChainResized( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr = S_OK;

    g_AspectRatio = (float)pBackBufferSurfaceDesc->Width / (float)pBackBufferSurfaceDesc->Height;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    // Locate the HUD and UI based on the area of main display.
    g_HUD.SetLocation( 0, 0 );
    g_HUD.SetSize( 170, 170 );

    return hr;
}
//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void RenderScene( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext )
{
    static int counter = 0;
    counter++;

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    pd3dImmediateContext->Map( g_ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
    float* f = static_cast<float*>(MappedResource.pData);
    f[ 0 ] = 0.2f + std::abs( std::sin( static_cast<float> (counter) / 64.0f ) );
    f[ 1 ] = g_AspectRatio;
    pd3dImmediateContext->Unmap( g_ConstantBuffer, 0 );

    const UINT stride[] = { sizeof( Vertex ) };
    const UINT offset[] = { 0 };
    pd3dImmediateContext->IASetVertexBuffers( 0, 1, &g_TriangleVB, stride, offset );
    pd3dImmediateContext->VSSetConstantBuffers( 0, 1, &g_ConstantBuffer );
    pd3dImmediateContext->PSSetSamplers( 0, 1, &g_SampleLinear );
    pd3dImmediateContext->PSSetShaderResources( 0, 1, &g_TextureSRV );

    pd3dImmediateContext->IASetInputLayout( g_TriangleVertexLayout );
    pd3dImmediateContext->VSSetShader( g_TriangleVS, nullptr, 0 );
    pd3dImmediateContext->PSSetShader( g_UseExtensions ? g_TrianglePSwithIntrinsics : g_TrianglePS, nullptr, 0 );
    pd3dImmediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    pd3dImmediateContext->Draw( 3, 0 );
}
//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime, void* pUserContext )
{
    // Clear the render target
    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    float ClearColor[4] = { 0.042f, 0.042f, 0.042f, 0.0f };
    pd3dImmediateContext->ClearRenderTargetView( pRTV, ClearColor );
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if (g_D3DSettingsDlg.IsActive())
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }

    RenderScene(pd3dDevice, pd3dImmediateContext);

    DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
    RenderText();
    g_HUD.OnRender( fElapsedTime );
    DXUT_EndPerfEvent();
}
//--------------------------------------------------------------------------------------
// Render the help and statistics text
//--------------------------------------------------------------------------------------
void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 0, 0 );
    g_pTxtHelper->SetForegroundColor( DirectX::XMVectorSet( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );
    g_pTxtHelper->End();
}
//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}
//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_D3DSettingsDlg.OnD3D11DestroyDevice();
    CDXUTDirectionWidget::StaticOnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();

    SAFE_DELETE( g_pTxtHelper );

    SAFE_RELEASE( g_TextureSRV );
    SAFE_RELEASE( g_Texture );
    SAFE_RELEASE( g_SampleLinear );
    SAFE_RELEASE( g_DummySampler );
    SAFE_RELEASE( g_ConstantBuffer );
    SAFE_RELEASE( g_TriangleVertexLayout );
    SAFE_RELEASE( g_TriangleVS );
    SAFE_RELEASE( g_TrianglePS );
    SAFE_RELEASE( g_TrianglePSwithIntrinsics );
    SAFE_RELEASE( g_TriangleVB );

    // It is safe to call the deinit even if the init was not called.
    agsDriverExtensionsDX11_DeInit( g_AGSContext );
}
