#define HLSL
#include "P:\Quake-2\ref_dx11rg\DX11RenderEngine\DX11RenderEngine\CoreShaderInclude.h"
#include "P:\Quake-2\ref_dx11rg\DX11RenderEngine\DX11RenderEngine\ModelConstBuffers.h"



#ifdef LERP
struct VSIn {
	float2 uv  :     TEXCOORD;
	float3 pos1 :    Position1;
	float3 normal1 : NORMAL1;
	float3 pos2 :    Position2;
	float3 normal2 : NORMAL2;
};
#else
struct VSIn {
	float3 pos : Position;
	float3 normal : NORMAL;
	float2 uv  : TEXCOORD;
};
#endif


struct VSOut {
	float4 pos : SV_Position;
	float2 uv  : TEXCOORD;
	float3 normal : NORMAL;
};


//cbuffer externalData {
//	matrix world;
//	matrix view;
//	matrix projection;
//
//	float4 color;
//	float alpha;
//	float width;
//	float height;
//}


//cbuffer LerpExternalData{
//	//float4 color;	
//	//float alpha;
//	//float width;
//	//float height;
//}



VSOut vsIn(VSIn input) {
	VSOut vso;
	float3 worldPosition;
#ifdef LERP
	worldPosition = input.pos1 * ModelsExtraData.alpha + (1 - ModelsExtraData.alpha) * input.pos2;
#else
	worldPosition = input.pos;
#endif

	vso.pos = mul(mul(mul(float4(worldPosition, 1.0f), ModelsTransform.world), ModelsTransform.view), ModelsTransform.projection);


#ifdef LERP
	if (ModelsExtraData.alpha < 0.5) {
		vso.normal = input.normal1;
	}
	else {
		vso.normal = input.normal2;
	}
#else
	vso.normal = input.normal;
#endif

#ifdef BAD_UV
	vso.uv = input.uv / ModelsExtraData.wh;
	vso.uv.y = vso.uv.y;
#else
	vso.uv = input.uv;
#endif


	return vso;
}


Texture2D tex : register(t0);

SamplerState basicSampler : register(s0);


float4 psIn(VSOut input) : SV_Target
{

	float4 result;
#ifdef RED
	return float4(1.0, input.uv.x, input.uv.y, 1.0f);
#endif

#ifdef COLORED
	return color;
#endif

	result = tex.Sample(basicSampler, input.uv);

#ifdef LIGHTED
	float4 light = ModelsExtraData.color;
	light += float4(1, 1, 1, 1) * 0.3;
	light.w = 1;
	result *= light;
#endif

	return result;
}




//VertexShaderOutput main(float2 pos : Position, float3 color : Color) {
//	VertexShaderOutput vso;
//	//matrix worldViewProj = mul(mul(world, view), projection);
//	//vso.pos = mul(float4(pos.x, pos.y, 0.0f, 1.0f), worldViewProj);
//	vso.pos = float4(pos.x, pos.y, 0.0f, 1.0f);
//	vso.color = float4(color, 1.0);
//	return vso;
//}