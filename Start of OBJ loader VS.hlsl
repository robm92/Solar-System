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



//**************************************************************************//
// Constant Buffer Variables.												//
//**************************************************************************//

//**************************************************************************//
// No longer used; it is here to prove a point which is that you can have	//
// many constant buffers.													//
//**************************************************************************//
cbuffer cbChangeOnResize : register( b0 )
{
    matrix Projection;
};

//**************************************************************************//
// Matrices constant buffer.												//
//**************************************************************************//
cbuffer cbChangesEveryFrame : register( b1 )
{
    matrix MatWorld;
	matrix MatWorldViewProjection;
};




//**************************************************************************//
// Vertex shader input structure.	The semantics (the things after the		//
// colon) look a little weird.  The semantics are used (so Microsoft tell	//
// us) used by the compiler to link shader inputs and outputs. 				//
//																			//
// For this to work, you must ensure that the vertex structure you use in	//
// any program that uses this shader is the same as below, vertex position,	//
// normal vector and texture U, V, in that order!							//
//**************************************************************************//
struct VS_INPUT
{
    float4 Pos       : POSITION;
	float3 VecNormal : NORMAL;
    float2 Tex       : TEXCOORD0;
};



//**************************************************************************//
// Vertex shader output strucrute, which also becomes the pixel shader input//
// structure.	The two things must be identical.							//
// Just a position and a texture UV.										//
//																			//
// NOTE: Pos has a different samentic to the structure above; get it wrong	//
// and nothing works.  That's because the pixel shader is in a different	//
// stage in the rendering pipeline.											//
//**************************************************************************//
struct VS_OPTPUT
{
    float4 Pos		 : SV_POSITION;
	float3 VecNormal : NORMAL;
	float2 Tex		 : TEXCOORD0;
	float4 worldPos  : POSITION;
};




//**************************************************************************//
// Vertex Shader.															//
//**************************************************************************//
VS_OPTPUT VS_obj( VS_INPUT input )
{
    VS_OPTPUT output;
   	//**********************************************************************//
	// Multiply every vertex vy the WVP matrix (we do it "properly here		//
	// unlike the cubes sample.												//
	//**********************************************************************//
	output.Pos = mul( input.Pos, MatWorldViewProjection );

	//position of planet... I think?
	output.worldPos = mul(input.Pos, MatWorld);
   
	//**********************************************************************//
	// Whatever we do to the tiger, we must also do to its normal vector.	//
	//**********************************************************************//
	output.VecNormal = mul( input.VecNormal, (float3x3)MatWorld );

	//**********************************************************************//
	// And finally, just copu the texture Us and Vs to the output structure	//
	//**********************************************************************//
	output.Tex = input.Tex;
    
    return output;
}

