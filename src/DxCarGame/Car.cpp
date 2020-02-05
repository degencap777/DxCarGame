#include "Car.h"
#include "d3dUtil.h"
#include "Vertex.h"
#include <fstream>
using namespace DirectX;
using namespace Microsoft::WRL;

DirectX::XMMATRIX Cube::GetWorldMatrix() const
{
	return XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) *
		XMMatrixTranslation(pos.x, pos.y, pos.z);
}

DirectX::XMMATRIX Cylinder::GetWorldMatrix() const
{
	return XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) *
		XMMatrixTranslation(pos.x, pos.y, pos.z);
}

Car::Car()
	: mSpeed(5)
{
}

void Car::InitResources(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> deviceContext)
{

	std::vector<std::wstring> filenames {
		L"Resource/Black.dds",
		L"Resource/Orange.dds",
		L"Resource/Red.dds",
		L"Resource/Green.dds",
		L"Resource/Blue.dds",
		L"Resource/Yellow.dds",
		L"Resource/White.dds",
		L"Resource/Tire0.dds",
		L"Resource/Tire1.dds",
	};
	// 检验所有文件是否存在
	bool fileExists = true;
	for (const std::wstring& filename : filenames)
	{
		std::wifstream wfin(filename);
		if (!wfin.is_open())
		{
			fileExists = false;
			wfin.close();
			break;
		}
		wfin.close();
	}
	if (fileExists)
	{
		// 从文件读取
		mTexArray = CreateDDSTexture2DArrayFromFile(device, deviceContext, filenames);
	}
	else
	{
		// 从内存读取
		// 后续可能会写专门的通用函数
		mTexArray = CreateCarCubeTextureArrayFromMemory(device, deviceContext);
	}
	
	mMaterial.ambient = XMFLOAT4(1, 1, 1, 1);
	mMaterial.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mMaterial.specular = XMFLOAT4(0, 0, 0, 1);

	// 初始化所有面
	Reset();
}

template<class VertexType>
void Car::SetMesh(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> deviceContext, const Geometry::MeshData<VertexType>& meshData)
{
	
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = (UINT)meshData.vertexVec.size() * sizeof(typename decltype(meshData.vertexVec)::value_type);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// 新建顶点缓冲区
	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory(&initData, sizeof(initData));
	initData.pSysMem = meshData.vertexVec.data();
	HR(device->CreateBuffer(&vbd, &initData, mVertexBuffer.ReleaseAndGetAddressOf()));


	// 设置索引缓冲区描述
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(WORD) * (UINT)meshData.indexVec.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	// 新建索引缓冲区
	initData.pSysMem = meshData.indexVec.data();
	HR(device->CreateBuffer(&ibd, &initData, mIndexBuffer.ReleaseAndGetAddressOf()));


	// 预先绑定顶点/索引缓冲区到渲染管线
	UINT strides[1] = { sizeof(typename decltype(meshData.vertexVec)::value_type) };
	UINT offsets[1] = { 0 };
	deviceContext->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), strides, offsets);
	deviceContext->IASetIndexBuffer(mIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

}

void Car::Reset()
{
	mPosition = XMFLOAT3(0, 2, 0);
	XMStoreFloat3(&mPosition, XMVectorSet(0,0,0,0));

	// 初始化车身中心位置，用六个面默认填充黑色
	for (int i = 0; i < 6; ++i)
		for (int j = 0; j < 6; ++j)
			for (int k = 0; k < 6; ++k)
			{
				mCubes[i][j][k].pos = XMFLOAT3(-2.0f + 2.0f * i,
					3+2.0f * j, -5.0f + 2.0f * k);
				mCubes[i][j][k].rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
				memset(mCubes[i][j][k].faceColors, 0,
					sizeof mCubes[i][j][k].faceColors);
			}
	
	// +X面为橙色，-X面为红色
	// +Y面为绿色，-Y面为蓝色
	// +Z面为黄色，-Z面为白色
	for (int i = 0; i < 6; ++i)
		for (int j = 0; j < 6; ++j)
		{
			for (int k = 0; k < 6; ++k)
			{
				mCubes[k][i][j].faceColors[CarFace_PosX] = CarFaceColor_Orange;
				mCubes[k][i][j].faceColors[CarFace_NegX] = CarFaceColor_Red;

				mCubes[j][k][i].faceColors[CarFace_PosY] = CarFaceColor_Green;
				mCubes[j][k][i].faceColors[CarFace_NegY] = CarFaceColor_Blue;

				mCubes[i][j][k].faceColors[CarFace_PosZ] = CarFaceColor_Yellow;
				mCubes[i][j][k].faceColors[CarFace_NegZ] = CarFaceColor_White;
			}
		}	


	// 初始化轮胎中心位置和颜色
	mCylinder[0].pos = XMFLOAT3(3, 2, 3);
	mCylinder[1].pos = XMFLOAT3(-3, 2, 3);
	mCylinder[2].pos = XMFLOAT3(3, 2, -3);
	mCylinder[3].pos = XMFLOAT3(-3, 2, -3);
	for (int i = 0; i < 4; ++i)
	{
		mCylinder[i].rotation = XMFLOAT3(0, 0, XM_PIDIV2);
		mCylinder[i].faceColors[0] = Tire0;
		mCylinder[i].faceColors[1] = Tire1;
	}
}

void Car::Update(float dt)
{
	XMVECTOR pos = XMVectorZero();

	for (int i = 0; i < 4; ++i)
	{
		pos += XMLoadFloat3(&mCylinder[i].pos);
	}
	pos /= 4;
	XMStoreFloat3(&mPosition, pos);
	
	pos += XMLoadFloat3(&mCylinder[0].pos) - XMLoadFloat3(&mCylinder[2].pos) + XMVectorSet(0,-2,0,0);
	XMStoreFloat3(&mForwardTarget, pos);
}

void Car::Draw(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> deviceContext, BasicEffect& effect)
{
	// 初始化用于PS的常量缓冲区的值
	effect.SetTextureDiffuse(mTexArray.Get());
	effect.SetMaterial(mMaterial);
	SetMesh(device, deviceContext, Geometry::CreateBox());
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 1; ++j)
		{
			for (int k = j; k < 6; ++k)
			{
				effect.SetWorldMatrix(mCubes[i][j][k].GetWorldMatrix());
				for (int face = 0; face < 6; ++face)
				{
					effect.SetTexIndex(mCubes[i][j][k].faceColors[face]);
					effect.Apply(deviceContext);
					deviceContext->DrawIndexed(6, 6 * face, 0);
				}
			}
		}
	}
	SetMesh(device, deviceContext, Geometry::CreateCylinder(2,1.5f));
	for (int i = 0; i < 4; ++i)
	{
		effect.SetWorldMatrix(mCylinder[i].GetWorldMatrix());

		effect.SetTexIndex(mCylinder[i].faceColors[1]);
		effect.Apply(deviceContext);
		deviceContext->DrawIndexed(20 * 6, 0, 0);
		
		effect.SetTexIndex(mCylinder[i].faceColors[0]);
		effect.Apply(deviceContext);
		deviceContext->DrawIndexed(20 * 3, 20*6, 0);
		
		effect.SetTexIndex(mCylinder[i].faceColors[0]);
		effect.Apply(deviceContext);
		deviceContext->DrawIndexed(20 * 3, 20*9, 0);
	}
}

const XMFLOAT3 & Car::GetPosition()
{
	return mPosition;
}

const float & Car::GetRotateAngle()
{
	return mRotateAngle;
}

const XMFLOAT3 & Car::GetForwardTarget()
{
	return mForwardTarget;
}

XMVECTOR Rotate(XMVECTOR axis,XMVECTOR pos,float angle)
{
	
	return XMVector3TransformCoord(-pos, XMMatrixRotationAxis(axis, angle)) + pos;
}

void Car::Move(XMFLOAT2 move)
{
	float MaxAngle = XM_PI / 6;
	move.y *= mSpeed;
	float normal_y = move.y == 0 ? 0 : move.y * 1 / abs(move.y);
	float normal_x = move.x == 0 ? 0 : move.x * 1 / abs(move.x);
	float length = XMVectorGetX(XMVector3Length(XMLoadFloat3(&mCylinder[0].pos) - XMLoadFloat3(&mCylinder[2].pos)));
	float width = XMVectorGetX(XMVector3Length(XMLoadFloat3(&mCylinder[3].pos) - XMLoadFloat3(&mCylinder[2].pos)));
	//前轮旋转半径
	float radiu1 = length / sin(MaxAngle);
	//后轮旋转半径
	float radiu2 = length / tan(MaxAngle);
	
	mRotateAngle = normal_x * move.y / radiu1;
	//旋转前车身的前方向
	XMVECTOR forward = XMVector3Normalize(XMLoadFloat3(&mCylinder[0].pos) - XMLoadFloat3(&mCylinder[2].pos));
	//旋转前车身的向心方向
	XMVECTOR normalDir = normal_x * XMVector3Normalize(XMLoadFloat3(&mCylinder[2].pos) - XMLoadFloat3(&mCylinder[3].pos));
	
	if(move.y != 0)
	{
		for (int i = 0; i < 4; ++i)
		{
			if(i != 2)
			{
				XMVECTOR radiuOffset = normalDir * radiu2 + XMLoadFloat3(&mCylinder[2].pos) - XMLoadFloat3(&mCylinder[i].pos);
				radiuOffset -= XMVectorSet(0, XMVectorGetY(radiuOffset), 0, 0);
				XMVECTOR offset = move.x == 0 ? forward * move.y : Rotate(XMVectorSet(0, 1, 0, 0), radiuOffset, mRotateAngle);
				XMStoreFloat3(&mCylinder[i].pos, XMLoadFloat3(&mCylinder[i].pos) + offset);
			}
		}
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 1; ++j)
			{
				for (int k = 0; k < 6 - j; ++k)
				{
					XMVECTOR radiuOffset = normalDir * radiu2 + XMLoadFloat3(&mCylinder[2].pos) - XMLoadFloat3(&mCubes[i][j][k].pos);
					radiuOffset -= XMVectorSet(0, XMVectorGetY(radiuOffset), 0, 0);
					XMVECTOR offset = move.x == 0 ? forward * move.y : Rotate(XMVectorSet(0, 1, 0, 0), radiuOffset, mRotateAngle);
					XMStoreFloat3(&mCubes[i][j][k].pos, XMLoadFloat3(&mCubes[i][j][k].pos) + offset);
					mCubes[i][j][k].rotation.y += mRotateAngle;
				}
			}
		}
		XMVECTOR offset = move.x == 0 ? forward * move.y : Rotate(XMVectorSet(0, 1, 0, 0), normalDir * radiu2, mRotateAngle);
		XMStoreFloat3(&mCylinder[2].pos, XMLoadFloat3(&mCylinder[2].pos) + offset);
	}
	for (int i = 0; i < 2; ++i)
	{
		XMStoreFloat3(&mCylinder[i].rotation, XMLoadFloat3(&mCylinder[i].rotation) + XMVectorSet(move.y / 2, 0, 0, 0));
		XMStoreFloat3(&mCylinder[i + 2].rotation, XMLoadFloat3(&mCylinder[i + 2].rotation) + XMVectorSet(move.y / 2, 0, 0, 0));
		mCylinder[i + 2].rotation.y += mRotateAngle;
		mCylinder[i].rotation.y = move.x * MaxAngle + mCylinder[i + 2].rotation.y;
	}
}

DirectX::XMINT3 Car::HitCube(Ray ray, float * pDist) const
{
	BoundingOrientedBox box(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
	BoundingOrientedBox transformedBox;
	XMINT3 res = XMINT3(-1, -1, -1);
	float dist, minDist = FLT_MAX;

	// 优先拾取暴露在外的立方体(同时也是距离摄像机最近的)
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			for (int k = 0; k < 3; ++k)
			{
				box.Transform(transformedBox, mCubes[i][j][k].GetWorldMatrix());
				if (ray.Hit(transformedBox, &dist) && dist < minDist)
				{
					minDist = dist;
					res = XMINT3(i, j, k);
				}
			}
		}
	}
	if (pDist)
		*pDist = (minDist == FLT_MAX ? 0.0f : minDist);
		
	return res;
}

void Car::SetMoveSpeed(float speed)
{
	assert(speed > 0.0f);
	mSpeed = speed;
}

ComPtr<ID3D11ShaderResourceView> Car::GetTexArray() const
{
	return mTexArray;
}

ComPtr<ID3D11ShaderResourceView> Car::CreateCarCubeTextureArrayFromMemory(
	ComPtr<ID3D11Device> device,
	ComPtr<ID3D11DeviceContext> deviceContext)
{
	// 只有文件缺失的情况才会来到这里
	// 从内存创建

	// 创建纹理数组
	D3D11_TEXTURE2D_DESC texArrayDesc;
	texArrayDesc.Width = 128;
	texArrayDesc.Height = 128;
	texArrayDesc.MipLevels = 0;	// 指定后将生成完整mipmap链
	texArrayDesc.ArraySize = 7;
	texArrayDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texArrayDesc.SampleDesc.Count = 1;		// 不使用多重采样
	texArrayDesc.SampleDesc.Quality = 0;
	texArrayDesc.Usage = D3D11_USAGE_DEFAULT;
	texArrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;	// 生成mipmap需要绑定渲染目标
	texArrayDesc.CPUAccessFlags = 0;
	texArrayDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;	// 指定需要生成mipmap

	ComPtr<ID3D11Texture2D> texArray;
	HR(device->CreateTexture2D(&texArrayDesc, nullptr, texArray.GetAddressOf()));
	// 创建后立马获取纹理数组描述以获取生成的mipLevel
	texArray->GetDesc(&texArrayDesc);

	// (r, g, b, a)
	unsigned colors[7] = {
		'\x0\x0\x0\xff',		// 黑色
		'\xff\x6c\x0\xff',		// 橙色
		'\xdc\x42\x2f\xff',		// 红色
		'\x0\x9d\x54\xff',		// 绿色
		'\x3d\x81\xf6\xff',		// 蓝色
		'\xfd\xcc\x9\xff',		// 黄色
		'\xff\xff\xff\xff'		// 白色
	};


	uint32_t textureMap[128][128];
	// 默认先创建黑色
	for (int i = 0; i < 128; ++i)
		for (int j = 0; j < 128; ++j)
			textureMap[i][j] = colors[0];

	deviceContext->UpdateSubresource(texArray.Get(),
		D3D11CalcSubresource(0, 0, texArrayDesc.MipLevels),
		nullptr,
		textureMap, 
		128 * 4,
		128 * 128 * 4
	);
	// 创建其它颜色的纹理
	for (int i = 1; i <= 6; ++i)
	{
		for (int y = 7; y <= 17; ++y)
			for (int x = 25 - y; x <= 102 + y; ++x)
				textureMap[y][x] = textureMap[127 - y][x] = colors[i];

		for (int y = 18; y <= 109; ++y)
			for (int x = 7; x <= 120; ++x)
				textureMap[y][x] = colors[i];


		// 更新数据
		deviceContext->UpdateSubresource(texArray.Get(),
			D3D11CalcSubresource(0, i, texArrayDesc.MipLevels),
			nullptr,
			textureMap,
			128 * 4,
			128 * 128 * 4
		);

	}
	// 创建纹理数组的SRV
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texArrayDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	viewDesc.Texture2DArray.MostDetailedMip = 0;
	viewDesc.Texture2DArray.MipLevels = -1;	// 生成mipamp
	viewDesc.Texture2DArray.FirstArraySlice = 0;
	viewDesc.Texture2DArray.ArraySize = 7;

	ComPtr<ID3D11ShaderResourceView> texArraySRV;
	HR(device->CreateShaderResourceView(texArray.Get(), &viewDesc, texArraySRV.GetAddressOf()));
	// 生成mipmap
	deviceContext->GenerateMips(texArraySRV.Get());
	return texArraySRV;
}