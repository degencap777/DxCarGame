#include "Basic.hlsli"

// ¶¥µã×ÅÉ«Æ÷
VertexPosHWNormalTex VS(InstancePosNormalTex vIn)
{
	VertexPosHWNormalTex vOut;
	matrix worldViewProj = mul(vIn.World, mul(gView, gProj));
	vOut.PosH = mul(float4(vIn.PosL, 1.0f), worldViewProj);
	float4 posW = mul(float4(vIn.PosL, 1.0f), vIn.World);
	vOut.PosW = posW.xyz;
	vOut.NormalW = mul(vIn.NormalL, (float3x3) vIn.WorldInvTranspose);
	vOut.Tex = vIn.Tex;
	return vOut;
}