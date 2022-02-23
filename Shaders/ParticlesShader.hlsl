#define HLSL
#include "P:\Quake-2\ref_dx11rg\DX11RenderEngine\DX11RenderEngine\CoreShaderInclude.h"
#include "P:\Quake-2\ref_dx11rg\DX11RenderEngine\DX11RenderEngine\ParticlesConstBuffer.h"

struct VSIn {
	float3 pos     : Position;
	float4 color   : Color;
};

struct VSOut {
	float4 pos     : Position;
	float4 color   : Color;
};

struct GSOut {
	float4 pos     : SV_Position;
	float4 color   : Color;
};


VSOut vsIn(VSIn input) {
	VSOut vso;

	float4 worldPosition = float4(input.pos, 1);

	vso.pos = mul(worldPosition, ParticlesCB.view);

	vso.color = input.color;
	return vso;
}

GSOut CreateQuadVertex(VSOut input, float2 offset) {
	GSOut gso = (GSOut)0;
	input.pos.xy += offset;
	gso.pos = mul(input.pos, ParticlesCB.projection);
	gso.color = input.color;
	return gso;
}


[maxvertexcount(4)]
void gsIn(point VSOut input[1], inout TriangleStream<GSOut> tristream) {
	tristream.Append(CreateQuadVertex(input[0], float2(1, -1)));
	tristream.Append(CreateQuadVertex(input[0], float2(1, 1)));
	tristream.Append(CreateQuadVertex(input[0], float2(-1, -1)));
	tristream.Append(CreateQuadVertex(input[0], float2(-1, 1)));
	tristream.RestartStrip();

}



float4 psIn(GSOut input) : SV_Target
{
#ifdef RED
	return float4(1.0, 0,0, 1.0f);
#endif

	return input.color;
}

