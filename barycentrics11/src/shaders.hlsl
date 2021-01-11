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

#ifdef AMD_USE_SHADER_INTRINSICS
#include "../../ags_lib/hlsl/ags_shader_intrinsics_dx11.hlsl"
#endif


Texture2D<float4> Texture       : register(t0);
SamplerState texureSampler      : register(s0);

cbuffer PerFrameConstants : register (b0)
{
    float g_Scale;
    float g_Aspect;
    float pad0;
    float pad1;
}

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};


VertexShaderOutput VS_main( uint id : SV_VertexId )
{
    VertexShaderOutput output;

    const float3 positions[] =
    {
        {  0.0f,   0.25f, 0.0f },
        {  0.25f, -0.25f, 0.0f },
        { -0.25f, -0.25f, 0.0f }
    };

    const float2 uvs[] =
    {
        { 0.5f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f }
    };

    output.position = float4( positions[ id ], 1 );
    output.position.xy *= float2( 1.0, g_Aspect ) * g_Scale;
    output.uv = uvs[ id ];

    return output;
}


float4 PS_main( float4 position : SV_POSITION, float2 uv : TEXCOORD ) : SV_TARGET
{
    float4 col = float4(Texture.Sample( texureSampler, uv ).rgb, 1.0);
#ifdef AMD_USE_SHADER_INTRINSICS

    //fading a bit the checker board texture
    col.rgb *= 0.2;

    // actually using the barycentric intrinsic instrunction
    float2 barycentric = AmdDxExtShaderIntrinsics_IjBarycentricCoords( AmdDxExtShaderIntrinsicsBarycentric_LinearCenter );
    col.rgb += float3(barycentric.x, barycentric.y, 1.0 - (barycentric.x + barycentric.y));

#endif
    return col;
}
