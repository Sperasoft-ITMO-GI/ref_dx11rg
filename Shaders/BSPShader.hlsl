#define HLSL
#include "P:\Quake-2\ref_dx11rg\DX11RenderEngine\DX11RenderEngine\CoreShaderInclude.h"
#include "P:\Quake-2\ref_dx11rg\DX11RenderEngine\DX11RenderEngine\UPConstBuffers.h"

struct VSIn {
	float3 pos     : Position;
	float2 uv      : TEXCOORD;
	float2 luv     : LIGHTTEXCOORD;
};


struct VSOut {
	float4 pos  : SV_Position;
	float2 uv   : TEXCOORD;
	float2 luv  : LIGHTTEXCOORD;
};


//cbuffer externalData {
//	matrix world;
//	matrix view;
//	matrix projection;
//	float4 color;
//	float2 texOffset;
//}



VSOut vsIn(VSIn input) {
	VSOut vso;

	float3 worldPosition = input.pos;

	vso.pos = mul(mul(mul(float4(worldPosition, 1.0f), UPTransform.world), UPTransform.view), UPTransform.projection);
	vso.uv = input.uv;
	vso.luv = input.luv;// -UPExtraData.texOffset;

	return vso;
}


Texture2D tex : register(t0); 
Texture2D lightMapText : register(t1);

SamplerState basicSampler : register(s0);
SamplerState lightSampler : register(s1);


float4 psIn(VSOut input) : SV_Target
{
#ifdef RED
	return float4(1.0, 0,0, 1.0f);
#endif

#ifdef COLORED
	return color;
#endif
//return float4(input.uv.x, input.uv.y, 0.0f ,1.0f);
	float4 texColor = tex.Sample(basicSampler, input.uv) ;
	float4 light = float4(1,1,1,1);
	//result = float4(1.0,1.0,1.0,1.0);

	float luma = dot(texColor.rgb, float3(0.3f,0.5f,0.2f));
	float mask = saturate(luma*2-1);
	float3 glow = texColor.rgb * mask;
//	float4 	
	
#ifdef LIGHTMAPPED
	light = lightMapText.Sample(lightSampler, input.luv);
	light.rgb += float3(1, 1, 1)*0.3;
	light.w = 1;
#endif

	return texColor * light * UPExtraData.color + float4(glow, 0.0);
}




//VertexShaderOutput main(float2 pos : Position, float3 color : Color) {
//	VertexShaderOutput vso;
//	//matrix worldViewProj = mul(mul(world, view), projection);
//	//vso.pos = mul(float4(pos.x, pos.y, 0.0f, 1.0f), worldViewProj);
//	vso.pos = float4(pos.x, pos.y, 0.0f, 1.0f);
//	vso.color = float4(color, 1.0);
//	return vso;
//}