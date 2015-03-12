//**************************************************************************//
// Class to implement a Thing3d.     It is a child of the abstract class	//
// "Thing3DAbstract.h.														//
//**************************************************************************//

//**************************************************************************//
// This code is copyright of Dr Nigel Barlow, lecturer in computing,		//
// University of Plymouth, UK.  email: nigel@soc.plymouth.ac.uk.			//
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


#include "Thing3D.h"
#include <exception>	//Std stuff; no ".h"


//**************************************************************************//
// Constructor.  Force the parent constructor to execute, then create the   //
// default shaders.															//
//**************************************************************************//
Thing3D::Thing3D(ID3D11Device        *pRenderingDevice,
	ID3D11DeviceContext *pImmediateContext)
	: Thing3DAbstract(pRenderingDevice, pImmediateContext)
{
}





//**************************************************************************//
// Destructor.   We should release some Dx stuff here.   It is NOT complete!//
//**************************************************************************//

Thing3D::~Thing3D()
{
}


//**************************************************************************//
// Create method.  It uses the VERTEX format that was defined in			//
// AbstractThing3D.	  However we could certainly devine our own custom		//
// format here.																//
//**************************************************************************//

void Thing3D::Create(ID3D11Device        *pRenderingDevice,
	ID3D11DeviceContext *pImmediateContext)
{
	ShowMessage(L"Thing3D; call my CreateMesh() instead of create()");
}




//**************************************************************************//
// Load the mesh from an sdkmesh file.  The texture number dertrmines which //
// texture we use in the shader file.  Multi-textures are handled, if they	//
// are handled at all, by using the same shader variable for all textures,	//
// which is HORRIBLE for speed.												//
//**************************************************************************//
void Thing3D::CreateMesh(ID3D11Device         *pRenderingDevice,
	ID3D11DeviceContext  *pImmediateContext,
	LPCTSTR              SDKMeshFileName,
	int                  textureNumber)
{
	HRESULT hr = 0;	// needed for V macro.

	//**********************************************************************//
	// Load the mesh.  Then put the first texture into video memory.		//
	//The same Texture2D variable	is used for other textures.				//
	//**********************************************************************//
	this->textureNumber = textureNumber;

	V(mesh.Create(pRenderingDevice, SDKMeshFileName, true));

	SDKMESH_SUBSET			  *pSubset = NULL;
	D3D11_PRIMITIVE_TOPOLOGY  PrimType;

	pImmediateContext->PSSetSamplers(0, 1, &pThing3DVS_PS_Pair->pSamLinear);
	pSubset = mesh.GetSubset(0, 0);
	PrimType = CDXUTSDKMesh::GetPrimitiveType11((SDKMESH_PRIMITIVE_TYPE)pSubset->PrimitiveType);
	pImmediateContext->IASetPrimitiveTopology(PrimType);

	pThing3DVS_PS_Pair->pTextureResourceView = mesh.GetMaterial(pSubset->MaterialID)->pDiffuseRV11;
	pImmediateContext->PSSetShaderResources(textureNumber, 1, &pThing3DVS_PS_Pair->pTextureResourceView);

}





//**************************************************************************//
// Implementation of an abstract method in the parent class.  Render ourself//
// for whatever we are in this particular implementation, which is a mesh.	//
//**************************************************************************//
void Thing3D::RenderForMyImplementation(ID3D11Device        *pRenderingDevice,
	ID3D11DeviceContext *pImmediateContext)
{

	//**********************************************************************//
	// There is some issue with the overloaded multiply of an XMMatrix that //
	// the good ond D3DXMATRIX never used to have.   If you use the			//
	// overloaded multiply on an object's instance variables it goes bang!	//
	// Answers to Nigel please!  There may be a prize...					//
	//**********************************************************************//
	XMMATRIX matWorldlocal = matWorld;
	XMMATRIX matViewLocal = matView;
	XMMATRIX matProjectionLocal = matProjection;

	XMMATRIX matWorldViewProjectionLocal = matWorldlocal * matViewLocal * matProjectionLocal;

	matWorld = matWorldlocal;


	//******************************************************************//    
	// Update shader variables.  We must update these for every mesh	//
	// that we draw (well, actually we need only update the position	//
	// for each mesh, think hard about this - Nigel						//
	//																	//
	// We pass the parameters to it in a constant buffer.  The buffer	//
	// we define in this module MUST match the constant buffer in the	//
	// shader.															//
	//																	//
	// It would seem that the constant buffer we pass to the shader must//
	// be global, well defined on the heap anyway.  Not a local variable//
	// it would seem.													//
	//******************************************************************// 
	Thing3DVS_PS_Pair::CBMatrices      cbMatrices;
	cbMatrices.matWorld = XMMatrixTranspose(matWorldlocal);
	cbMatrices.matWorldViewProjection = XMMatrixTranspose(matWorldViewProjectionLocal);
	pThing3DVS_PS_Pair->UpdateCBMatrices(pImmediateContext, &cbMatrices);

	Thing3DVS_PS_Pair::CBLighting cbLighting;
	cbLighting.lightDiffuseColour = lightDiffuseColour;
	cbLighting.lightDiffuseColour = lightDiffuseColour;
	cbLighting.materialDiffuseColor = materialDiffuseColour;
	cbLighting.vecLightDir = XMVector3Normalize(vecLightDirection);
	pThing3DVS_PS_Pair->UpdateCBLighting(pImmediateContext, &cbLighting);

	pImmediateContext->PSSetSamplers(0, 1, &pThing3DVS_PS_Pair->pSamLinear);
	pThing3DVS_PS_Pair->SetupShadersAndConstantbuffers(pImmediateContext);


	//**************************************************************************//
	// Render the mesh.															//
	//**************************************************************************//
	//Get the mesh
	//IA setup

	pImmediateContext->IASetInputLayout(pThing3DVS_PS_Pair->pVertexLayout);
	UINT Strides[1];
	UINT Offsets[1];
	ID3D11Buffer* pVB[1];
	pVB[0] = mesh.GetVB11(0, 0);
	Strides[0] = (UINT)mesh.GetVertexStride(0, 0);
	Offsets[0] = 0;
	pImmediateContext->IASetVertexBuffers(0, 1, pVB, Strides, Offsets);
	pImmediateContext->IASetIndexBuffer(mesh.GetIB11(0), mesh.GetIBFormat11(0), 0);


	UINT                      numSubsets = mesh.GetNumSubsets(0);
	boolean                   mustRestoreOriginalTexture = false;
	ID3D11ShaderResourceView  *pDiffuseRV = NULL;
	SDKMESH_SUBSET			  *pSubset = NULL;
	D3D11_PRIMITIVE_TOPOLOGY  PrimType;


	//pThing3DVS_PS_Pair->SetTexturesAndConstantbuffers();

	//**************************************************************************//
	// Render each subset.														//
	//**************************************************************************//
	for (UINT subset = 0; subset < numSubsets; ++subset)
	{
		pSubset = NULL;

		// Get the subset
		pSubset = mesh.GetSubset(0, subset);

		PrimType = CDXUTSDKMesh::GetPrimitiveType11((SDKMESH_PRIMITIVE_TYPE)pSubset->PrimitiveType);
		pImmediateContext->IASetPrimitiveTopology(PrimType);

		//**************************************************************************//
		// Should there be more than one texture, we put it into each one into the  //
		// same slot in shader memory, which is going to be slow.					//
		//**************************************************************************//
		//if (subset > 0)	// not yet working
		{
			pDiffuseRV = mesh.GetMaterial(pSubset->MaterialID)->pDiffuseRV11;
			pImmediateContext->PSSetShaderResources(textureNumber, 1, &pDiffuseRV);
			mustRestoreOriginalTexture = true;
		}

		pImmediateContext->DrawIndexed((UINT)pSubset->IndexCount, 0, (UINT)pSubset->VertexStart);
	}

	//******************************************************************************//
	// Restore the original texture if we put another in its place.					//
	//******************************************************************************//
	if (mustRestoreOriginalTexture)
	{
		PrimType = CDXUTSDKMesh::GetPrimitiveType11((SDKMESH_PRIMITIVE_TYPE)pSubset->PrimitiveType);
		pSubset = mesh.GetSubset(0, 0);
		pImmediateContext->IASetPrimitiveTopology(PrimType);
		pDiffuseRV = mesh.GetMaterial(pSubset->MaterialID)->pDiffuseRV11;
		pImmediateContext->PSSetShaderResources(textureNumber, 1, &pDiffuseRV);
	}


}







//**************************************************************************//
// Do something, up to you what.	All this does is move the object forward//
// and slow it down some, to simulate slow the object a wee bit of friction.//
//**************************************************************************//
void Thing3D::DoBehaviour()
{
	MoveForward();
	speed *= 0.99;
}
