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


#include "Thing3DAbstract.h"
#include "SDKmesh.h"



#ifndef Thing3D_H     //These are termed "guards", can you see what they do?
#define Thing3D_H	 


//**************************************************************************//
// Many of these member variables are public and have to be set by writing  //
// to the variable.   OO programmers would have us write many more          //
// setThisandThat(...) methods.                                             //
//**************************************************************************//
class Thing3D : public Thing3DAbstract
{

	//**********************************************************************//
	// Public instance variables.											//
	//**********************************************************************//
public:
	CDXUTSDKMesh mesh;


	//**********************************************************************//
	// Public methods.														//
	//**********************************************************************//
public:
	//******************************************************************//
	// Constructor.  Force the parent constructor to execute, then		//
	// create the default shaders.										//
	//******************************************************************//
	Thing3D(ID3D11Device        *pRenderingDevice,
		ID3D11DeviceContext *pImmediateContext);
	~Thing3D();									//Destructor.



	//**********************************************************************//
	// Implementation of abstract methods in parent class.					//
	//**********************************************************************//

public:
	virtual void Create(ID3D11Device        *pRenderingDevice,
		ID3D11DeviceContext *pImmediateContext);
	virtual void RenderForMyImplementation(ID3D11Device        *pRenderingDevice,
		ID3D11DeviceContext *pImmediateContext);	//Draw yourself 
	//for whatever you are
	virtual void DoBehaviour();



	//**********************************************************************//
	// Load the mesh from an sdkmesh file.  The texture number dertrmines	//
	// which texture we use in the shader file.  Multi-textures are handled,//
	// if they are handled at all, by using the same shader variable for	//
	// all textures, which is HORRIBLE for speed.							//
	//**********************************************************************//
	virtual void CreateMesh(ID3D11Device         *pRenderingDevice,
		ID3D11DeviceContext  *pImmediateContext,
		LPCTSTR              SDKMeshFileName,
		int                  textureNumber);




};	// End of classy class definition. 
// Must be a ";" here in C++ - weird, eh?   Nigel


#endif	//End of guard.

