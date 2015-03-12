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
	float3 pos;
	float  range;
	float3 att;
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
	float4 worldPos  : POSITION;
};

//**************************************************************************//
// Pixel Shader.	This one has no lighting.  You can have many pixel		//
// shaders in one of these files.											//
//**************************************************************************//
float4 PS_TexturesNoLighting( PS_INPUT input) : SV_Target
{
    return txDiffuse.Sample( samLinear, input.Tex );
}

float4 PS_TexturesWithLighting(PS_INPUT input) : SV_Target
{
	input.vecNormal = normalize(input.vecNormal);

	float4 diffuse = txDiffuse.Sample(samLinear, input.Tex);

	float3 finalColor = float3(0.0f, 0.0f, 0.0f);

	//Create the vector between light position and pixels position
	float3 lightToPixelVec = light.pos - input.worldPos;

	//Find the distance between the light pos and pixel pos
	float d = length(lightToPixelVec);

	//Create the ambient light
	float3 finalAmbient = diffuse * light.ambient;

		//If pixel is too far, return pixel color with ambient light
		if (d > light.range)
			return float4(finalAmbient, diffuse.a);

	//Turn lightToPixelVec into a unit length vector describing
	//the pixels direction from the lights position
	lightToPixelVec /= d;

	//Calculate how much light the pixel gets by the angle
	//in which the light strikes the pixels surface
	float howMuchLight = dot(lightToPixelVec, input.vecNormal);

	//If light is striking the front side of the pixel
	if (howMuchLight > 0.0f)
	{
		//Add light to the finalColor of the pixel
		finalColor += howMuchLight * diffuse * light.diffuse;

		//Calculate Light's Falloff factor
		finalColor /= light.att[0] + (light.att[1] * d) + (light.att[2] * (d*d));
	}

	//make sure the values are between 1 and 0, and add the ambient
	finalColor = saturate(finalColor + finalAmbient);

	return float4(finalColor, diffuse.a);
}
