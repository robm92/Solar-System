//**************************************************************************//
// Start of an OBJ loader.  By no means an end.  This just creates			//
// triangles.																//
//**************************************************************************//

//**************************************************************************//
// Modifications to the MS sample code is copyright of Dr Nigel Barlow,		//
// lecturer in computing, University of Plymouth, UK.						//
// email: nigel@soc.plymouth.ac.uk.											//
//																			//
// You may use, modify and distribute this (rather cack-handed in places)	//
// code subject to the following conditions:								//
//																			//
//	1:	You may not use it, or sell it, or use it in any adapted form for	//
//		financial gain, without my written premission.						//
//																			//
//	2:	You must not remove the copyright messages.							//
//																			//
//	3:	You should correct at least 10% of the typig abd spekking errirs.   //
//**************************************************************************//
//--------------------------------------------------------------------------------------
// File: Tutorial07 - Textures and Constant buffers.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

struct Light
{
	float3 dir;
	float4 ambient;
	float4 diffuse;
};

//**************************************************************************//
// Constant Buffer Variables.												//
//**************************************************************************//
cbuffer cbNeverChanges : register( b0 )
{
	Light light;
};

//**************************************************************************//
// Textures and Texture Samplers.  These variables can't (it seems) go in	//
// constant buffers; I think the size of a CB is too restricted.			//
//																			//
// We only have one Texture2D variable here, what does that suggest about	//
// the nature or our mesh, i.e. could it be multi-textured?					//
//**************************************************************************//
Texture2D    txDiffuse : register( t0 );
SamplerState samLinear : register( s0 );

//**************************************************************************//
// Pixel shader input structure.	The semantics (the things after the		//
// colon) look a little weird.  The semantics are used (so Microsoft tell	//
// us) used by the compiler to link shader inputs and outputs. 				//
//																			//
// For this to work, you must ensure this structure is identical to the		//
// vertex shader's output structure.										//
//**************************************************************************//
struct PS_INPUT
{
    float4 Pos		 : SV_POSITION;
	float3 vecNormal : NORMAL;
	float2 Tex		 : TEXCOORD0;
};

//**************************************************************************//
// Pixel Shader.	This one has no lighting.  You can have many pixel		//
// shaders in one of these files.											//
//**************************************************************************//
float4 PS_TexturesNoLighting( PS_INPUT input) : SV_Target
{
    return txDiffuse.Sample( samLinear, input.Tex );
}

//**************************************************************************//
// Pixel Shader.	This one has basic lighting, however the really			//
// important part is the rexture sampler.									//
//**************************************************************************//
float4 PS_TexturesWithLighting( PS_INPUT input) : SV_Target
{
	input.vecNormal = normalize(input.vecNormal);

	float4 diffuse = txDiffuse.Sample(samLinear, input.Tex);

	float3 finalColor;

	finalColor = diffuse * light.ambient;
	finalColor += saturate(dot(light.dir, input.vecNormal) * light.diffuse * diffuse);

	return float4(finalColor, diffuse.a);

	/*float4 newLight = saturate( dot( light.dir, input.vecNormal ) );
    return txDiffuse.Sample( samLinear, input.Tex ) * newLight;*/
}
