#define HLSL
#include "P:\Quake-2\ref_dx11rg\DX11RenderEngine\DX11RenderEngine\CoreShaderInclude.h"
#include "P:\Quake-2\ref_dx11rg\DX11RenderEngine\DX11RenderEngine\UIConstBuffers.h"


struct VSIn {
	float2 pos : Position;
	float2 uv  : TEXCOORD;
};

struct VSOut {
	float4 pos : SV_Position;
	float2 uv  : TEXCOORD;
};


//cbuffer externalData {
//	matrix transform;
//	float2 uvShift;
//	float2 uvScale;
//	float4 color;
//}


VSOut vsIn(VSIn input) {
	VSOut vso;
	vso.pos = mul(float4(input.pos.x, input.pos.y, 0.0f, 1.0f), UITransform.transform);
#ifdef SCALED
	vso.uv = input.uv * UITransform.uvScale + UITransform.uvShift;
#else
	vso.uv = input.uv;
#endif
	return vso;
}

Texture2D tex : register(t0);
SamplerState basicSampler : register(s0);

float4 psIn(VSOut input) : SV_Target
{
#ifdef COLORED
	return UITransform.color;
#endif

#ifdef RED
	return float4(1,0, 0.0f ,1.0f);
#endif
	//return float4(input.uv.x, input.uv.y, 0.0f ,1.0f);
	return tex.Sample(basicSampler, input.uv);
	//return float4(input.uv.x, input.uv.y, 0.0f ,1.0f);
}




//VertexShaderOutput main(float2 pos : Position, float3 color : Color) {
//	VertexShaderOutput vso;
//	//matrix worldViewProj = mul(mul(world, view), projection);
//	//vso.pos = mul(float4(pos.x, pos.y, 0.0f, 1.0f), worldViewProj);
//	vso.pos = float4(pos.x, pos.y, 0.0f, 1.0f);
//	vso.color = float4(color, 1.0);
//	return vso;
//}