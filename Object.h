#ifndef OBJECT_H
#define OBJECT_H

#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>
#include <fstream>		// Files.  The ones without ".h" are the new (not
#include <string>		// so new now) standard library headers.
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <vector>
#include <stdio.h>
#include "resource.h"
#include "Gamepad.h"

using std::vector;
using std::string;
using std::stringstream;
using std::ifstream;


class Object
{
public:

	float g_f_PlanetX;
	float g_f_PlanetY;
	float g_f_PlanetZ;
	int              g_numIndices;



	ID3D11Buffer*                       g_pVertexBuffer;
	ID3D11Buffer*                       g_pIndexBuffer;
	ID3D11SamplerState*                 g_pSamplerLinear;
	XMMATRIX                            g_MatProjection;
	HRESULT hResult;

	float g_f_PlanetSpeed;
	float newElapseTime;
	float g_f_PlanetRY;// = XMConvertToRadians(45);  //45º default
	float planetOrbit;

	//**************************************************************************//
	// The texture; just one here.												//
	//**************************************************************************//
	ID3D11ShaderResourceView           *g_pTextureResourceView;
	//Textures and material variables, used for all mesh's loaded
	std::vector<ID3D11ShaderResourceView*> meshSRV;
	std::vector<std::wstring> textureNameArray;



	//**************************************************************************//
	// Now a global instance of each constant buffer.							//
	//**************************************************************************//
	ID3D11Buffer                       *g_pCBNeverChanges;
	ID3D11Buffer                       *g_pCBChangeOnResize;
	ID3D11Buffer                       *g_pCBChangesEveryFrame;

	LPCWSTR textureName;
	LPSTR filename;
	ID3D11ShaderResourceView *textureResource;

	float planetScale;
	XMMATRIX matPlanetWorld;
	XMMATRIX earthOrbitMatrix;

	D3D11_BUFFER_DESC bd;
	D3D11_SUBRESOURCE_DATA InitData;

	std::vector <USHORT> vectorIndices;

	Object() : _vBuff(NULL), _iBuff(NULL) {}

	void LoadFromFile(UINT width, UINT height, ID3D11DeviceContext* g_pImmediateContext,
		ID3D11Device* g_pd3dDevice);

	std::wstring TrimStart(std::wstring s);

	void Render(ID3D11DeviceContext* g_pImmediateContext, ID3D11RenderTargetView* g_pRenderTargetView,
		ID3D11DepthStencilView* g_pDepthStencilView, ID3D11VertexShader* g_pVertexShader,
		ID3D11PixelShader* g_pPixelShader, float eyeX, float eyeY, float eyeZ, float atX,
		float atY, float atZ,bool aPressed);

private:
	ID3D11Buffer * _vBuff;
	ID3D11Buffer * _iBuff;

};

#endif