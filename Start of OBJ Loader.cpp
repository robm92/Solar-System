//**************************************************************************//
// Start of an OBJ loader.  By no means an end.  This just creates			//
// triangles.																//
//																			//
// Look for the Nigel style comments, like these, for the bits you need to  //
// look at.																	//
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
//	3:	You should correct at least 10% of the typing abd spekking errirs.   //
//**************************************************************************//

//--------------------------------------------------------------------------------------
// File: Tutorial07 - Textures and Constant buffers.cpp
//
// This application demonstrates texturing
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
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
#include "Object.h"
#include "Gamepad.h"



//**************************************************************************//
// Nothing is easy in DirectX.  Before we can even create a single vertex,	//
// we need to define what it looks like.									//
//																			//
// The data types seems to be inhereted from XNA.							//
// An XMFLOAT3 is a float containing 3 numbers, an x, y, x position here.	//
// An XMFLOAT4 is a float containing 4 values, an RGBA colour.	Not that	//
// alpha effects work without additional effort.							//
//**************************************************************************//
struct SimpleVertex
{
	XMFLOAT3 Pos;	//Why not a float4?  See the shader strucrure.  Any thoughts?  Nigel
	XMFLOAT3 VecNormal;
	XMFLOAT2 TexUV;
};

//**************************************************************************//
// A sort of mesh subset, basically an array of vertices and indexes.		//
//**************************************************************************//
struct SortOfMeshSubset
{
	SimpleVertex *vertices;
	USHORT       *indexes;
	USHORT       numVertices;
	USHORT       numUVs;
	USHORT       numNormals;
	USHORT       numIndices;
};


//**************************************************************************//
// Light vector never moves; and colour never changes.  I have done it .	//
// this way to show how constant buffers can be used so that you don't		//
// upsate stuff you don't need to.											//
// Beware of constant buffers that aren't in multiples of 16 bytes..		//
//**************************************************************************//
struct CBNeverChanges
{
	XMFLOAT4 materialColour;
	XMVECTOR vecLight;			// Must be 4, we only use the first 3.
};

struct CBChangeOnResize
{
	XMMATRIX matProjection;
};

//**************************************************************************//
// Note we do it properly here and pass the WVP matrix, rather than world,	//
// view and projection matrices separately.									//
//																			//
// We still need the world matrix to transform the normal vectors.			//
//**************************************************************************//
struct CBChangesEveryFrame
{
	XMMATRIX matWorld;
	XMMATRIX matWorldViewProjection;
};




//**************************************************************************//
// Global Variables.  There are many global variables here (we aren't OO	//
// yet.  I doubt  Roy Tucker (Comp Sci students will know him) will			//
// approve pf this either.  Sorry, Roy.										//
//**************************************************************************//
HINSTANCE                           g_hInst = NULL;
HWND                                g_hWnd = NULL;
D3D_DRIVER_TYPE                     g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL                   g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*                       g_pd3dDevice = NULL;
ID3D11DeviceContext*                g_pImmediateContext = NULL;
IDXGISwapChain*                     g_pSwapChain = NULL;
ID3D11RenderTargetView*             g_pRenderTargetView = NULL;
ID3D11Texture2D*                    g_pDepthStencil = NULL;
ID3D11DepthStencilView*             g_pDepthStencilView = NULL;
ID3D11VertexShader*                 g_pVertexShader = NULL;
ID3D11PixelShader*                  g_pPixelShader = NULL;
ID3D11InputLayout*                  g_pVertexLayout = NULL;
HRESULT hr;
Object Earth, Mercury, sun, pluto, venus, mercury, mars, jupiter, saturn, neptune, uranus, moon,saturnRings, skyBox;
Gamepad *gamepad;

float eyeX = 35, eyeY = 10, eyeZ = 4;
float atX = 0, atY = 0, atZ = 0;





//**************************************************************************//
// Forward declarations.													//
//																			//
// If you are not used to "C" you will find that functions (or methods in	//
// "C++" must have templates defined in advance.  It is usual to define the //
// prototypes in a header file, but we'll put them here for now to keep		//
// things simple.															//
//**************************************************************************//
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();
void charStrToWideChar(WCHAR *dest, char *source);
void XMFLOAT3normalise(XMFLOAT3 *toNormalise);
SortOfMeshSubset *LoadMesh(LPSTR filename);
void keyboardPressed();
void GamepadCamera(bool xboxEnabled, Gamepad* gamepad);



//keyboard

bool		 g_b_LeftArrowDown = false;	//Status of keyboard.  Thess are set
bool		 g_b_RightArrowDown = false;	//in the callback KeyboardProc(), and 
bool		 g_b_UpArrowDown = false;	//are used in onFrameMove().
bool		 g_b_DownArrowDown = false;
bool		 g_b_SpaceBarDown = false;
bool		 g_b_W = false;






//**************************************************************************//
// A Windows program always kicks off in WinMain.							//
// Initializes everything and goes into a message processing				//
// loop. Idle time is used to render the scene.								//
//																			//
// In other words, run the computer flat out.  Is this good?				//
//**************************************************************************//
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;

	if (FAILED(InitDevice()))
	{
		CleanupDevice();
		return 0;
	}



	//**************************************************************************//
	// Main Windoze message loop.												//
	//																			//
	// Gamers will see this as a game loop, though you will find something like //
	// this main loop deep within any Windows application.						//
	//**************************************************************************//
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Render();
		}
	}

	CleanupDevice();

	return (int)msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"TutorialWindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	// Create window
	g_hInst = hInstance;
	RECT rc = { 0, 0, 1600, 900 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindow(L"TutorialWindowClass", L"Solar System Model", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
		NULL);
	if (!g_hWnd)
		return E_FAIL;

	ShowWindow(g_hWnd, nCmdShow);

	return S_OK;
}


//**************************************************************************//
// Compile the shader file.  These files aren't pre-compiled (well, not		//
// here, and are compiled on he fly).										//
//**************************************************************************//
HRESULT CompileShaderFromFile(WCHAR* szFileName,		// File Name
	LPCSTR szEntryPoint,		// Namee of shader
	LPCSTR szShaderModel,		// Shader model
	ID3DBlob** ppBlobOut)	// Blob returned
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

	ID3DBlob* pErrorBlob;
	hr = D3DX11CompileFromFile(szFileName, NULL, NULL, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL);
	if (FAILED(hr))
	{
		WCHAR errorCharsW[200];
		if (pErrorBlob != NULL)
		{
			charStrToWideChar(errorCharsW, (char *)pErrorBlob->GetBufferPointer());
			MessageBox(0, errorCharsW, L"Error", 0);
		}
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}



//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
		return hr;

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;

	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, NULL, &g_pDepthStencil);
	if (FAILED(hr))
		return hr;

	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr))
		return hr;

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);


	//**********************************************************************//
	// Compile the shader file.  These files aren't pre-compiled (well, not //
	// here, and are compiles on he fly).									//
	//																		//
	// This is DirectX11, but what shader model do you see here?			//
	// Change to shader model 5 in Babbage209 and it should still work.		//
	//**********************************************************************//
	ID3DBlob* pVSBlob = NULL;
	hr = CompileShaderFromFile(L"Start of OBJ loader VS.hlsl", "VS_obj", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	//**********************************************************************//
	// Create the vertex shader.											//
	//**********************************************************************//
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	//**********************************************************************//
	// Create the pixel shader.												//
	//**********************************************************************//
	ID3DBlob* pPSBlob = NULL;
	hr = CompileShaderFromFile(L"Start of OBJ loader PS.hlsl", "PS_TexturesWithLighting", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;


	//**********************************************************************//
	// Define the input layout.  I won't go too much into this except that  //
	// the vertex defined here MUST be consistent with the vertex shader	//
	// input you use in your shader file and the constand buffer structure  //
	// at the top of this module.											//
	//																		//
	// Here a vertex has a position a normal vector (used for lighting) and //
	// a single texture UV coordinate.										//
	//**********************************************************************//
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Set the input layout
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	//create the Gamepad
	gamepad = new Gamepad(1);


	//**************************************************************************//
	// Load the obj mesh										//
	//**************************************************************************//	
	Mercury.filename = "Media\\earth\\earth.obj";
	Mercury.textureName = L"Media\\earth\\mercury.jpg";
	Mercury.planetScale = 0.8;
	Mercury.LoadFromFile(width, height, g_pImmediateContext, g_pd3dDevice);
	Mercury.g_f_PlanetX = 7;

	venus.filename = "Media\\earth\\earth.obj";
	venus.textureName = L"Media\\earth\\venus.jpg";
	venus.planetScale = 1.0;
	venus.LoadFromFile(width, height, g_pImmediateContext, g_pd3dDevice);
	venus.g_f_PlanetX = 9;
	
	Earth.filename = "Media\\earth\\earth.obj";
	Earth.textureName = L"Media\\earth\\NewEarth.jpg";
	Earth.planetScale = 1.7;
	Earth.LoadFromFile(width, height, g_pImmediateContext, g_pd3dDevice);
	Earth.g_f_PlanetX = 13;
	
	moon.filename = "Media\\earth\\earth.obj";
	moon.textureName = L"Media\\earth\\moon.jpg";
	moon.planetScale = 0.2;
	moon.LoadFromFile(width, height, g_pImmediateContext, g_pd3dDevice);
	moon.g_f_PlanetX = 1;

	mars.filename = "Media\\earth\\earth.obj";
	mars.textureName = L"Media\\earth\\mars.jpg";
	mars.planetScale = 1.0;
	mars.LoadFromFile(width, height, g_pImmediateContext, g_pd3dDevice);
	mars.g_f_PlanetX = 17;

	jupiter.filename = "Media\\earth\\earth.obj";
	jupiter.textureName = L"Media\\earth\\jupiter.jpg";
	jupiter.planetScale = 5.7;
	jupiter.LoadFromFile(width, height, g_pImmediateContext, g_pd3dDevice);
	jupiter.g_f_PlanetX = 22;

	saturn.filename = "Media\\earth\\earth.obj";
	saturn.textureName = L"Media\\earth\\saturn.jpg";
	saturn.planetScale = 4;
	saturn.LoadFromFile(width, height, g_pImmediateContext, g_pd3dDevice);
	saturn.g_f_PlanetX = 30;

	saturnRings.filename = "Media\\earth\\earth.obj";
	saturnRings.textureName = L"Media\\earth\\test.png";
	saturnRings.LoadFromFile(width, height, g_pImmediateContext, g_pd3dDevice);
	saturnRings.g_f_PlanetX = 30;

	uranus.filename = "Media\\earth\\earth.obj";
	uranus.textureName = L"Media\\earth\\uranus.jpg";
	uranus.planetScale = 2;
	uranus.LoadFromFile(width, height, g_pImmediateContext, g_pd3dDevice);
	uranus.g_f_PlanetX = 35;

	neptune.filename = "Media\\earth\\earth.obj";
	neptune.textureName = L"Media\\earth\\neptune.jpg";
	neptune.planetScale = 2;
	neptune.LoadFromFile(width, height, g_pImmediateContext, g_pd3dDevice);
	neptune.g_f_PlanetX = 40;

	pluto.filename = "Media\\earth\\earth.obj";
	pluto.textureName = L"Media\\earth\\pluto.jpg";
	pluto.planetScale = 0.8;
	pluto.LoadFromFile(width, height, g_pImmediateContext, g_pd3dDevice);
	pluto.g_f_PlanetX = 45;

	sun.filename = "Media\\earth\\earth.obj";
	sun.textureName = L"Media\\earth\\sun.jpg";
	sun.planetScale = 10;
	sun.LoadFromFile(width, height, g_pImmediateContext, g_pd3dDevice);
	sun.g_f_PlanetX = 0;


	//O.LoadFromFile("Media\\pig\\pig.obj", width, height, g_pImmediateContext, g_pd3dDevice);


	return S_OK;
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState();

	//if (g_pSamplerLinear) g_pSamplerLinear->Release();
	//if (g_pTextureResourceView) g_pTextureResourceView->Release();
	//if (g_pCBNeverChanges) g_pCBNeverChanges->Release();
	//if (g_pCBChangeOnResize) g_pCBChangeOnResize->Release();
	//if (g_pCBChangesEveryFrame) g_pCBChangesEveryFrame->Release();
	//if (g_pVertexBuffer) g_pVertexBuffer->Release();
	//if (g_pIndexBuffer) g_pIndexBuffer->Release();
	if (g_pVertexLayout) g_pVertexLayout->Release();
	if (g_pVertexShader) g_pVertexShader->Release();
	if (g_pPixelShader) g_pPixelShader->Release();
	if (g_pDepthStencil) g_pDepthStencil->Release();
	//if (g_pDepthStencilView) g_pDepthStencilView->Release();
	//if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void Render()
{

	//
	// Clear the back buffer
	//
	float ClearColor[4] = { 0.1f, 0.1f, 0.1f, 0.1f }; // red, green, blue, alpha
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);

	//
	// Clear the depth buffer to 1.0 (max depth)
	//
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	//**************************************************************************//
	// Update our time.  This block is supposed to make the movement frame rate //
	// independent, as the frame rate we get depends of the performance of our	//
	// computer.  We may even be in reference (software emulation) mode, which	//
	// is painfully slow.														//
	//**************************************************************************//
	static float t = 0.0f;
	if (g_driverType == D3D_DRIVER_TYPE_REFERENCE)
	{
		t += (float)XM_PI * 0.0125f;
	}
	else
	{
		static DWORD dwTimeStart = 0;
		DWORD dwTimeCur = GetTickCount();
		if (dwTimeStart == 0)
			dwTimeStart = dwTimeCur;
		t = (dwTimeCur - dwTimeStart) / 1000.0f;
	}
	

	//gamepad->Rumble(1.0f, 1.0f);
	GamepadCamera(true, gamepad);


	//*************************************************//
	//            Render the Planets                   //
	//*************************************************//
	Earth.Render(g_pImmediateContext, g_pRenderTargetView, g_pDepthStencilView,
		g_pVertexShader, g_pPixelShader, eyeX, eyeY, eyeZ, atX, atY, atZ);
	//rotation speed relative to planet size
	Earth.g_f_PlanetRY = XMConvertToRadians(45) / Earth.planetScale *t;
	Earth.planetOrbit = XMConvertToRadians(45) / Earth.g_f_PlanetX *t * 10;

	moon.Render(g_pImmediateContext, g_pRenderTargetView, g_pDepthStencilView,
		g_pVertexShader, g_pPixelShader, eyeX, eyeY, eyeZ, atX, atY, atZ);
	moon.g_f_PlanetRY = XMConvertToRadians(45) / moon.planetScale * t;
	moon.planetOrbit = XMConvertToRadians(45) / Earth.g_f_PlanetX *t* 10;

	mars.Render(g_pImmediateContext, g_pRenderTargetView, g_pDepthStencilView,
		g_pVertexShader, g_pPixelShader, eyeX, eyeY, eyeZ, atX, atY, atZ);
	mars.g_f_PlanetRY = XMConvertToRadians(45) / mars.planetScale * t;
	mars.planetOrbit = XMConvertToRadians(45) / mars.g_f_PlanetX *t* 10;

	jupiter.Render(g_pImmediateContext, g_pRenderTargetView, g_pDepthStencilView,
		g_pVertexShader, g_pPixelShader, eyeX, eyeY, eyeZ, atX, atY, atZ);
	jupiter.g_f_PlanetRY = XMConvertToRadians(45) / jupiter.planetScale * t;
	jupiter.planetOrbit = XMConvertToRadians(45) / jupiter.g_f_PlanetX *t * 10;

	saturn.Render(g_pImmediateContext, g_pRenderTargetView, g_pDepthStencilView,
		g_pVertexShader, g_pPixelShader, eyeX, eyeY, eyeZ, atX, atY, atZ);
	saturn.g_f_PlanetRY = XMConvertToRadians(45) / saturn.planetScale * t;
	saturn.planetOrbit = XMConvertToRadians(45) / saturn.g_f_PlanetX *t* 10;

	saturnRings.Render(g_pImmediateContext, g_pRenderTargetView, g_pDepthStencilView,
		g_pVertexShader, g_pPixelShader, eyeX, eyeY, eyeZ, atX, atY, atZ);
	saturnRings.g_f_PlanetRY = XMConvertToRadians(45) / saturn.planetScale * t;
	saturnRings.planetOrbit = XMConvertToRadians(45) / saturn.g_f_PlanetX *t * 10;

	uranus.Render(g_pImmediateContext, g_pRenderTargetView, g_pDepthStencilView,
		g_pVertexShader, g_pPixelShader, eyeX, eyeY, eyeZ, atX, atY, atZ);
	uranus.g_f_PlanetRY = XMConvertToRadians(45) / uranus.planetScale * t;
	uranus.planetOrbit = XMConvertToRadians(45) / uranus.g_f_PlanetX *t* 10;

	neptune.Render(g_pImmediateContext, g_pRenderTargetView, g_pDepthStencilView,
		g_pVertexShader, g_pPixelShader, eyeX, eyeY, eyeZ, atX, atY, atZ);
	neptune.g_f_PlanetRY = XMConvertToRadians(45) / neptune.planetScale * t;
	neptune.planetOrbit = XMConvertToRadians(45) / neptune.g_f_PlanetX *t* 10;

	pluto.Render(g_pImmediateContext, g_pRenderTargetView, g_pDepthStencilView,
		g_pVertexShader, g_pPixelShader, eyeX, eyeY, eyeZ, atX, atY, atZ);
	pluto.g_f_PlanetRY = XMConvertToRadians(45) / pluto.planetScale * t;
	pluto.planetOrbit = XMConvertToRadians(45) / pluto.g_f_PlanetX *t* 10;

	Mercury.Render(g_pImmediateContext, g_pRenderTargetView, g_pDepthStencilView,
		g_pVertexShader, g_pPixelShader, eyeX, eyeY, eyeZ, atX, atY, atZ);
	Mercury.g_f_PlanetRY = XMConvertToRadians(45) / Mercury.planetScale * t;
	Mercury.planetOrbit = XMConvertToRadians(45) / Mercury.g_f_PlanetX *t* 10;

	venus.Render(g_pImmediateContext, g_pRenderTargetView, g_pDepthStencilView,
		g_pVertexShader, g_pPixelShader, eyeX, eyeY, eyeZ, atX, atY, atZ);
	venus.g_f_PlanetRY = XMConvertToRadians(45) / venus.planetScale * t;
	venus.planetOrbit = XMConvertToRadians(45) / venus.g_f_PlanetX *t* 10;

	sun.Render(g_pImmediateContext, g_pRenderTargetView, g_pDepthStencilView,
		g_pVertexShader, g_pPixelShader, eyeX, eyeY, eyeZ, atX, atY, atZ);
	sun.g_f_PlanetRY = XMConvertToRadians(45) / sun.planetScale *t;


	//
	// Present our back buffer to our front buffer
	//
	g_pSwapChain->Present(0, 0);
}




//**************************************************************************//
// Convert an old chracter (char *) string to a WCHAR * string.  There must//
// be something built into Visual Studio to do this for me, but I can't		//
// find it - Nigel.															//
//**************************************************************************//
void charStrToWideChar(WCHAR *dest, char *source)
{
	int length = strlen(source);
	for (int i = 0; i <= length; i++)
		dest[i] = (WCHAR)source[i];
}



//**************************************************************************//
// Normalise an XMFLOAT3 as if it were a vector, i.e. return a vector of	//
// unit magnitude.  By Nigel.												//
//**************************************************************************//
void XMFLOAT3normalise(XMFLOAT3 *toNormalise)
{
	float magnitude = (toNormalise->x * toNormalise->x
		+ toNormalise->y * toNormalise->y
		+ toNormalise->z * toNormalise->z);

	magnitude = sqrt(magnitude);
	toNormalise->x /= magnitude;
	toNormalise->y /= magnitude;
	toNormalise->z /= magnitude;
}


void GamepadCamera(bool xboxEnabled, Gamepad* gamepad)
{
	if (xboxEnabled)
	{
		gamepad->Update();


		float gamepadLTrig = gamepad->LeftTrigger();
		float gamepadRTrig = gamepad->RightTrigger();
		/*eyeX -= gamepadLTrig/100;
		eyeX += gamepadRTrig/100;*/

		if (!gamepad->LStick_InDeadzone())
		{

			float gamepadLX = gamepad->LeftStick_X();
			float gamepadLY = gamepad->LeftStick_Y();

			eyeZ  -= gamepadLX/100;
			atZ += gamepadLX/50;

			eyeX  -= gamepadLY/100;
			//atY += gamepadLY/50;
			

		}

		if (!gamepad->RStick_InDeadzone())
		{
			XMFLOAT3 lookAtPt;
			XMFLOAT3 eyePt;

			float gamepadRX = gamepad->RightStick_X();
			float gamepadRY = gamepad->RightStick_Y();

			atZ += gamepadRX/50;
			atY += gamepadRY/50;

		}
		//rumble if close to sun

		if (eyeX < 30)
		{
			gamepad->Rumble(0.4, 0.4);
		}
		else if (eyeX > 30)
		{
			gamepad->Rumble(0, 0);
		}

		gamepad->RefreshState();
	}
}




