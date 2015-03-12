#include "Object.h"

struct SimpleVertex
{
	XMFLOAT3 Pos;
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
// A simple structure to hold a single vertex.								//
//**************************************************************************//
struct VertexXYZ
{
	float x, y, z;
};

struct VertexUV
{
	float u, v;
};

struct VertexNorm
{
	float x, y, z;
};

//Light structure
struct Light
{
	Light()
	{
		ZeroMemory(this, sizeof(Light));
	}
	XMFLOAT3 dir;
	float pad;
	XMFLOAT3 pos;
	float range;
	XMFLOAT3 att;
	float pad2;
	XMFLOAT4 ambient;
	XMFLOAT4 diffuse;
};

Light light;

//**************************************************************************//
// Light vector never moves; and colour never changes.  I have done it .	//
// this way to show how constant buffers can be used so that you don't		//
// upsate stuff you don't need to.											//
// Beware of constant buffers that aren't in multiples of 16 bytes..		//
//**************************************************************************//
struct CBNeverChanges
{
	Light  light;
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

//std::vector <USHORT>    vectorIndices(0);
//std::vector<SimpleVertex> faceCount(0);
XMMATRIX earthMatrix;

std::wstring Object::TrimStart(std::wstring s)
{
	s.erase(s.begin(), std::find_if(s.begin(),
		s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

std::wstring TrimEnd(std::wstring s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(),
		std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

void Object::LoadFromFile(UINT width, UINT height, ID3D11DeviceContext* g_pImmediateContext,
	ID3D11Device* g_pd3dDevice)
{
	std::vector<SimpleVertex> faceCount(0);
	std::wifstream          fileStream;
	std::wstring            line;
	std::vector <VertexXYZ> vectorVertices(0);
	std::vector<VertexNorm> vertNorm(0);
	std::vector<VertexUV> textureUV(0);

	g_f_PlanetX = 0;
	g_f_PlanetY = 0;
	g_f_PlanetZ = 0;

	USHORT indexCount = 0;

	fileStream.open(filename);
	bool isOpen = fileStream.is_open();		//debugging only.


	while (std::getline(fileStream, line))
	{
		line = TrimStart(line);


		//******************************************************************//
		// If true, we have found a vertex.  Read it in as a 2 character	//
		// string, followed by 3 decimal numbers.	Suprisingly the C++		//
		// string has no split() method.   I am using really old stuff,		//
		// fscanf.  There  must be a better way, use regular expressions?	//
		//******************************************************************//
		if (line.compare(0, 2, L"v ") == 0)  //"v space"
		{
			WCHAR first[5];
			float x, y, z;

			WCHAR oldStyleStr[200];
			wcscpy(oldStyleStr, line.c_str());
			swscanf(oldStyleStr, L"%2s%f%f%f", first, &x, &y, &z);

			VertexXYZ v;
			v.x = x; v.y = y; v.z = z;
			vectorVertices.push_back(v);
		}

		if (line.compare(0, 2, L"vt") == 0)  //"vertex UVs
		{
			WCHAR first[5];
			float u, v;
			WCHAR oldStyleStr[200];
			wcscpy(oldStyleStr, line.c_str());
			swscanf(oldStyleStr, L"%2s%f%f", first, &u, &v);

			VertexUV vt;
			vt.u = u; vt.v = v;
			textureUV.push_back(vt);
		}

		if (line.compare(0, 2, L"vn") == 0)  //"Vertex Normals
		{
			WCHAR first[5];
			float x, y, z;

			WCHAR oldStyleStr[200];
			wcscpy(oldStyleStr, line.c_str());
			swscanf(oldStyleStr, L"%2s%f%f%f", first, &x, &y, &z);

			VertexNorm v;
			v.x = x; v.y = y; v.z = z;
			vertNorm.push_back(v);
		}




		//******************************************************************//
		// If true, we have found a face.   Read it in as a 2 character		//
		// string, followed by 3 decimal numbers.	Suprisingly the C++		//
		// string has no split() method.   I am using really old stuff,		//
		// fscanf.  There must be a better way, use regular expressions?	//
		//																	//
		// It assumes the line is in the format								//
		// f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 ...							//
		//******************************************************************// 
		if (line.compare(0, 2, L"f ") == 0)  //"f space"
		{
			WCHAR first[5];
			WCHAR slash1[5];
			WCHAR slash2[5];
			WCHAR slash3[5];
			WCHAR slash4[5];
			WCHAR slash5[5];
			WCHAR slash6[5];

			UINT v1, vt1, vn1, v2, vt2, vn2, v3, vt3, vn3;

			WCHAR oldStyleStr[200];
			wcscpy(oldStyleStr, line.c_str());
			swscanf(oldStyleStr, L"%2s%d%1s%d%1s%d%d%1s%d%1s%d%d%1s%d%1s%d", first,
				&v1, slash1, &vt1, slash2, &vn1,
				&v2, slash3, &vt2, slash4, &vn2,
				&v3, slash5, &vt3, slash6, &vn3);

			//vectorIndices.push_back(v1 - 1);	// Check this carefully; see below	
			//vectorIndices.push_back(v2 - 1);
			//vectorIndices.push_back(v3 - 1);

			//create 3 vertices whenever a face is found
			//faces indicate the index of the loaded v/vt/vn in the array
			//count number of vertices... every time a vertex is pushed to the array of simple vertex, add one to count.

			SimpleVertex faceVert;
			SimpleVertex faceVert2;
			SimpleVertex faceVert3;

			//first vertex
			faceVert.Pos.x = vectorVertices[v1 - 1].x;
			faceVert.Pos.y = vectorVertices[v1 - 1].y;
			faceVert.Pos.z = vectorVertices[v1 - 1].z;
			//first Vertex UVs
			faceVert.TexUV.x = textureUV[vt1 - 1].u;
			faceVert.TexUV.y = textureUV[vt1 - 1].v;

			//first Normals
			faceVert.VecNormal.x = vertNorm[vn1 - 1].x;
			faceVert.VecNormal.y = vertNorm[vn1 - 1].y;
			faceVert.VecNormal.z = vertNorm[vn1 - 1].z;

			//Second vertex
			faceVert2.Pos.x = vectorVertices[v2 - 1].x;
			faceVert2.Pos.y = vectorVertices[v2 - 1].y;
			faceVert2.Pos.z = vectorVertices[v2 - 1].z;
			//Second Vertex UVs
			faceVert2.TexUV.x = textureUV[vt2 - 1].u;
			faceVert2.TexUV.y = textureUV[vt2 - 1].v;

			//Second Normals
			faceVert2.VecNormal.x = vertNorm[vn2 - 1].x;
			faceVert2.VecNormal.y = vertNorm[vn2 - 1].y;
			faceVert2.VecNormal.z = vertNorm[vn2 - 1].z;

			//Third vertex
			faceVert3.Pos.x = vectorVertices[v3 - 1].x;
			faceVert3.Pos.y = vectorVertices[v3 - 1].y;
			faceVert3.Pos.z = vectorVertices[v3 - 1].z;
			//Third Vertex UVs
			faceVert3.TexUV.x = textureUV[vt3 - 1].u;
			faceVert3.TexUV.y = textureUV[vt3 - 1].v;

			//Third Normals
			faceVert3.VecNormal.x = vertNorm[vn3 - 1].x;
			faceVert3.VecNormal.y = vertNorm[vn3 - 1].y;
			faceVert3.VecNormal.z = vertNorm[vn3 - 1].z;
			//dont forget to destroy the faceverts because they are pointer types
			vectorIndices.push_back(indexCount++);
			vectorIndices.push_back(indexCount++);
			vectorIndices.push_back(indexCount++);

			//push back each faceVert to the array of simple vertexes
			faceCount.push_back(faceVert);
			faceCount.push_back(faceVert2);
			faceCount.push_back(faceVert3);


			//Push back the count of each facevert to the indexcount.



		}
	}


	//******************************************************************//
	// Now build up the arrays.											//
	//																	// 
	// Nigel to address this bit; it is WRONG.  OBJ meshes assume index //
	// numbers start from 1; C arrays have indexes that start at 0.		//
	//																	//
	// See abobe wih the -1s.  Sorted?									//
	//******************************************************************//
	SortOfMeshSubset *mesh = new SortOfMeshSubset;

	mesh->numVertices = (USHORT)faceCount.size();
	mesh->numUVs = (USHORT)textureUV.size();
	mesh->numNormals = (USHORT)vertNorm.size();
	mesh->vertices = new SimpleVertex[mesh->numVertices];

	//start populating array of simplevertex
	for (int i = 0; i < mesh->numVertices; i++)
	{
		mesh->vertices[i] = faceCount[i];
	}

	mesh->numIndices = (USHORT)vectorIndices.size();
	mesh->indexes = new USHORT[mesh->numIndices];
	for (int i = 0; i < mesh->numIndices; i++)
	{
		mesh->indexes[i] = vectorIndices[i];
	}

	//**************************************************************************//
	// Create the vertex buffer.												//
	//**************************************************************************//

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * mesh->numVertices;	//From sortOfMesh
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = mesh->vertices;						//From sortOfMesh

	hResult = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);




	//**************************************************************************//																		
	// Index buffer 															//
	//**************************************************************************//

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(USHORT) * mesh->numIndices;   //From sortOfMesh

	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = mesh->indexes;					//From sortOfMesh

	hResult = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBuffer);


	g_numIndices = mesh->numIndices;

	// Set index buffer
	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	// Set primitive topology
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	//**************************************************************************//
	// once vertex and index buffers are created, this structure is no longer needed.//
	//**************************************************************************//
	delete mesh->indexes;		// Delete the two arrays.
	delete mesh->vertices;
	faceCount.clear();
	//std::fill(vectorIndices.begin(), vectorIndices.end(), 0);
	delete mesh;				// Then delete  sortOfMesh

	//**************************************************************************//
	// Create the 3 constant buffers. for Planet											//
	//**************************************************************************//
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(CBNeverChanges);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hResult = g_pd3dDevice->CreateBuffer(&bd, NULL, &g_pCBNeverChanges);
	//if (FAILED(hResult))
	//	return hResult;

	bd.ByteWidth = sizeof(CBChangeOnResize);
	hResult = g_pd3dDevice->CreateBuffer(&bd, NULL, &g_pCBChangeOnResize);
	//if (FAILED(hResult))
	//	return hResult;

	bd.ByteWidth = sizeof(CBChangesEveryFrame);
	hResult = g_pd3dDevice->CreateBuffer(&bd, NULL, &g_pCBChangesEveryFrame);
	//if (FAILED(hResult))
	//	return hResult;



	//**************************************************************************//
	// Load the texture into "ordinary" RAM.									//
	//**************************************************************************//
	hResult = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice,
		textureName,
		NULL, NULL,
		&g_pTextureResourceView,		// This is returned.
		NULL);
	//if (FAILED(hResult))
	//	return hResult;

	textureResource = g_pTextureResourceView;
	//**************************************************************************//
	// Put the texture in shader (video) memory.  As there is only one texture,	//
	// we can do this only once.  Try to minimise the number of times you put	//
	// textures into video memory, there is quite an overhead in doing so.		//
	//**************************************************************************//
	//g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureResourceView);


	// Create the sample state
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hResult = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear);
	//if (FAILED(hResult))
	//	return hResult;



	//**************************************************************************//
	// Update the constant buffer for stuff (the light vector and material		//
	// colour in this case) that never change.  This is faster; don't update	//
	// stuff if you don't have to.												//
	//**************************************************************************//
	if (textureName == L"Media\\earth\\sun.jpg")
	{
		light.pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
		light.range = 100.0f;
		light.att = XMFLOAT3(0.0f, 0.05f, 0.0f);
		light.ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		light.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	else
	{
		light.pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
		light.range = 100.0f;
		light.att = XMFLOAT3(0.0f, 0.05f, 0.0f);
		light.ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.1f);
		light.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	CBNeverChanges cbNeverChanges;
	cbNeverChanges.light = light;
	g_pImmediateContext->UpdateSubresource(g_pCBNeverChanges,
		0, NULL,
		&cbNeverChanges,
		0, 0);


	//**************************************************************************//
	// Create the projection matrix.  Generally you will only want to create	//
	// this matrix once and then forget it.										//
	//**************************************************************************//
	g_MatProjection = XMMatrixPerspectiveFovLH(XM_PIDIV4,			// Field of view (pi / 4 radians, or 45 degrees
		width / (FLOAT)height, // Aspect ratio.
		0.01f,					// Near clipping plane.
		200.0f);				// Far clipping plane.

	CBChangeOnResize cbChangesOnResize;
	cbChangesOnResize.matProjection = XMMatrixTranspose(g_MatProjection);
	g_pImmediateContext->UpdateSubresource(g_pCBChangeOnResize, 0, NULL, &cbChangesOnResize, 0, 0);


}



void Object::Render(ID3D11DeviceContext* g_pImmediateContext, ID3D11RenderTargetView* g_pRenderTargetView,
	ID3D11DepthStencilView* g_pDepthStencilView, ID3D11VertexShader* g_pVertexShader,
	ID3D11PixelShader* g_pPixelShader, float eyeX, float eyeY, float eyeZ, float atX, float atY, float atZ)
{

	g_pImmediateContext->PSSetShaderResources(0, 1, &textureResource);

	//**************************************************************************//
	// Initialize the view matrix.  What you do to the viewer matrix moves the  //
	// viewer, or course.														//
	//																			//
	// The viewer matrix is created every frame here, which looks silly as the	//
	// viewer never moves.  However in general your viewer does move.			//
	//**************************************************************************//


	XMVECTOR Eye = XMVectorSet(eyeX, eyeY, eyeZ, 2.0f);
	XMVECTOR At = XMVectorSet(atX, atY, atZ, 0.0f);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX matView = XMMatrixLookAtLH(Eye,	// The eye, or viewer's position.
		At,	// The look at point.
		Up);	// Which way is up.

	// Planet World Matrix
	if (textureName == L"Media\\earth\\moon.jpg")
	{
		XMMATRIX matPlanetTranslate = XMMatrixTranslation(g_f_PlanetX, g_f_PlanetY, g_f_PlanetZ);
		XMMATRIX matPlanetRotate = XMMatrixRotationY(g_f_PlanetRY);
		XMMATRIX matPlanetOrbit = XMMatrixRotationY(planetOrbit);
		XMMATRIX matPlanetScale = XMMatrixScaling(planetScale, planetScale, planetScale);
		matPlanetWorld = matPlanetScale * matPlanetRotate * matPlanetTranslate *matPlanetOrbit * earthMatrix;


		XMMATRIX matWVP = matPlanetWorld * matView * g_MatProjection;
		// 
		CBChangesEveryFrame cb;
		cb.matWorld = XMMatrixTranspose(matPlanetWorld);
		cb.matWorldViewProjection = XMMatrixTranspose(matWVP);
		g_pImmediateContext->UpdateSubresource(g_pCBChangesEveryFrame, 0, NULL, &cb, 0, 0);
	}
	else if (textureName == L"Media\\earth\\test.png")
	{
		XMMATRIX matPlanetTranslate = XMMatrixTranslation(g_f_PlanetX, g_f_PlanetY, g_f_PlanetZ);
		XMMATRIX matPlanetRotate = XMMatrixRotationY(g_f_PlanetRY);
		XMMATRIX matPlanetOrbit = XMMatrixRotationY(planetOrbit);
		XMMATRIX matPlanetScale = XMMatrixScaling(8, 0, 10);
		matPlanetWorld = matPlanetScale * matPlanetRotate * matPlanetTranslate *matPlanetOrbit;

		XMMATRIX matWVP = matPlanetWorld * matView * g_MatProjection;

		CBChangesEveryFrame cb;
		cb.matWorld = XMMatrixTranspose(matPlanetWorld);
		cb.matWorldViewProjection = XMMatrixTranspose(matWVP);
		g_pImmediateContext->UpdateSubresource(g_pCBChangesEveryFrame, 0, NULL, &cb, 0, 0);
	}
	else
	{
		XMMATRIX matPlanetTranslate = XMMatrixTranslation(g_f_PlanetX, g_f_PlanetY, g_f_PlanetZ);
		XMMATRIX matPlanetRotate = XMMatrixRotationY(g_f_PlanetRY);
		XMMATRIX matPlanetOrbit = XMMatrixRotationY(planetOrbit);
		XMMATRIX matPlanetScale = XMMatrixScaling(planetScale, planetScale, planetScale);
		XMMATRIX matPlanetWorld = matPlanetScale * matPlanetRotate * matPlanetTranslate *matPlanetOrbit;

		if (textureName == L"Media\\earth\\NewEarth.jpg")
		{
			//store earth's matrix to use with the moon orbit.
			earthMatrix = matPlanetWorld;
		}


		XMMATRIX matWVP = matPlanetWorld * matView *  g_MatProjection;

		//
		// Update variables that change once per frame
		//
		CBChangesEveryFrame cb;
		cb.matWorld = XMMatrixTranspose(matPlanetWorld);
		cb.matWorldViewProjection = XMMatrixTranspose(matWVP);
		g_pImmediateContext->UpdateSubresource(g_pCBChangesEveryFrame, 0, NULL, &cb, 0, 0);
	}
	//
	// Render the Planet
	//

	// Set vertex buffer
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBNeverChanges);		//Note this one belongs to the pixel shader.
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBChangeOnResize);	// Paremeter 1 relates to pisition in 
	g_pImmediateContext->VSSetConstantBuffers(1, 1, &g_pCBChangesEveryFrame);	// constant buffers.
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	g_pImmediateContext->PSSetConstantBuffers(2, 1, &g_pCBChangesEveryFrame);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
	g_pImmediateContext->DrawIndexed(g_numIndices, 0, 0);

}



