#include "Basic.hlsli"

// ¶¥µã×ÅÉ«Æ÷
VertexPosHWNormalTex VS(VertexPosNormalTex vIn)
{
	VertexPosHWNormalTex vOut;
	matrix worldViewProj = mul(gWorld, mul(gView, gProj));
	vOut.PosH = mul(float4(vIn.PosL, 1.0f), worldViewProj);
	float4 posW = mul(float4(vIn.PosL, 1.0f), gWorld);
	vOut.PosW = posW.xyz;
	vOut.NormalW = mul(vIn.NormalL, (float3x3) gWorldInvTranspose);
	vOut.Tex = vIn.Tex;
	return vOut;
}