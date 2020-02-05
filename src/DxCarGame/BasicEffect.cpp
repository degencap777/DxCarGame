#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// ��������Effects.h��d3dUtil.h����
#include "Vertex.h"
using namespace DirectX;



//
// BasicEffect::Impl ��Ҫ����BasicEffect�Ķ���
//

class BasicEffect::Impl : public AlignedType<BasicEffect::Impl>
{
public:

	//
	// ��Щ�ṹ���ӦHLSL�Ľṹ�塣��Ҫ��16�ֽڶ���
	//
	struct CBChangesEveryInstanceDrawing
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX worldInvTranspose;
	};

	struct CBChangesEveryObjectDrawing
	{
		Material material;
	};
	
	struct CBChangesEveryDrawing
	{
		int texIndex;
		XMFLOAT3 gPad;
	};

	struct CBChangesEveryFrame
	{
		XMMATRIX view;
		XMVECTOR eyePos;
	};

	struct CBChangesOnResize
	{
		XMMATRIX proj;
	};

	struct CBDrawingStates
	{
		int textureUsed;
		int reflectionEnabled;
		DirectX::XMFLOAT2 pad;
	};
	
	struct CBChangesRarely
	{
		DirectionalLight dirLight[BasicEffect::maxLights];
		PointLight pointLight[BasicEffect::maxLights];
		SpotLight spotLight[BasicEffect::maxLights];
	};
public:
	// ������ʽָ��
	Impl() = default;
	~Impl() = default;

public:
	// ��Ҫ16�ֽڶ�������ȷ���ǰ��
	CBufferObject<0, CBChangesEveryInstanceDrawing>	m_CBInstDrawing;		// ÿ��ʵ�����Ƶĳ���������
	CBufferObject<1, CBChangesEveryObjectDrawing>	m_CBObjDrawing;		    // ÿ�ζ�����Ƶĳ���������
	CBufferObject<2, CBDrawingStates>				m_CBStates;			    // ÿ�λ���״̬�ı�ĳ���������
	CBufferObject<3, CBChangesEveryFrame>			m_CBFrame;			    // ÿ֡���Ƶĳ���������
	CBufferObject<4, CBChangesOnResize>				m_CBOnResize;			// ÿ�δ��ڴ�С����ĳ���������
	CBufferObject<5, CBChangesRarely>				m_CBRarely;			    // �����������ĳ���������
	CBufferObject<6, CBChangesEveryDrawing>			m_CBDrawing;				// ÿ�ζ�����Ƶĳ���������
	
	BOOL m_IsDirty;											// �Ƿ���ֵ���
	std::vector<CBufferBase*> cBufferPtrs;					// ͳһ�����������еĳ���������

	ComPtr<ID3D11VertexShader> m_pBasicInstanceVS;
	ComPtr<ID3D11VertexShader> m_pBasicObjectVS;

	ComPtr<ID3D11PixelShader> m_pBasicPS;

	ComPtr<ID3D11InputLayout> m_pInstancePosNormalTexLayout;
	ComPtr<ID3D11InputLayout> m_pVertexPosNormalTexLayout;

	ComPtr<ID3D11ShaderResourceView> m_pTextureDiffuse;		// ����������
	ComPtr<ID3D11ShaderResourceView> m_pTextureCube;			// ��պ�����

};

//
// BasicEffect
//

namespace
{
	// BasicEffect����
	static BasicEffect * pInstance = nullptr;
}

BasicEffect::BasicEffect()
{
	if (pInstance)
		throw std::exception("BasicEffect is a singleton!");
	pInstance = this;
	pImpl = std::make_unique<BasicEffect::Impl>();
}

BasicEffect::~BasicEffect()
{
}

BasicEffect::BasicEffect(BasicEffect && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
}

BasicEffect & BasicEffect::operator=(BasicEffect && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

BasicEffect & BasicEffect::Get()
{
	if (!pInstance)
		throw std::exception("BasicEffect needs an instance!");
	return *pInstance;
}


bool BasicEffect::InitAll(ComPtr<ID3D11Device> device)
{
	if (!device)
		return false;

	if (!pImpl->cBufferPtrs.empty())
		return true;
	
	ComPtr<ID3DBlob> blob;

	// ʵ�����벼��
	D3D11_INPUT_ELEMENT_DESC basicInstLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "World", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "World", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "World", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "World", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "WorldInvTranspose", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "WorldInvTranspose", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 80, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "WorldInvTranspose", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 96, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "WorldInvTranspose", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 112, D3D11_INPUT_PER_INSTANCE_DATA, 1}
	};
	// ******************
	// ����������ɫ��
	//
	HR(CreateShaderFromFile(L"HLSL\\BasicInstance_VS.cso", L"HLSL\\BasicInstance_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pBasicInstanceVS.GetAddressOf()));
	// �������㲼��
	HR(device->CreateInputLayout(basicInstLayout, ARRAYSIZE(basicInstLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pInstancePosNormalTexLayout.GetAddressOf()));

	HR(CreateShaderFromFile(L"HLSL\\BasicObject_VS.cso", L"HLSL\\BasicObject_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pBasicObjectVS.GetAddressOf()));
	// �������㲼��
	HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTexLayout.GetAddressOf()));

	// ******************
	// ����������ɫ��
	//
	HR(CreateShaderFromFile(L"HLSL\\Basic_PS.cso", L"HLSL\\Basic_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pBasicPS.GetAddressOf()));

	pImpl->cBufferPtrs.assign({
		&pImpl->m_CBInstDrawing,
		&pImpl->m_CBObjDrawing,
		&pImpl->m_CBStates,
		&pImpl->m_CBFrame,
		&pImpl->m_CBOnResize,
		&pImpl->m_CBRarely,
		&pImpl->m_CBDrawing});

	// ��������������
	for (auto& pBuffer : pImpl->cBufferPtrs)
	{
		pBuffer->CreateBuffer(device);
	}

	return true;
}

void BasicEffect::SetRenderDefault(ComPtr<ID3D11DeviceContext> deviceContext, RenderType type)
{
	if (type == RenderInstance)
	{
		deviceContext->IASetInputLayout(pImpl->m_pInstancePosNormalTexLayout.Get());
		deviceContext->VSSetShader(pImpl->m_pBasicInstanceVS.Get(), nullptr, 0);
		deviceContext->PSSetShader(pImpl->m_pBasicPS.Get(), nullptr, 0);
	}
	else
	{
		deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
		deviceContext->VSSetShader(pImpl->m_pBasicObjectVS.Get(), nullptr, 0);
		deviceContext->PSSetShader(pImpl->m_pBasicPS.Get(), nullptr, 0);
	}

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	deviceContext->GSSetShader(nullptr, nullptr, 0);
	deviceContext->RSSetState(nullptr);

	// ע�������Ϊ�������Թ�����
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSAnistropicWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void XM_CALLCONV BasicEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
	auto& cBuffer = pImpl->m_CBInstDrawing;
	cBuffer.data.world = XMMatrixTranspose(W);
	cBuffer.data.worldInvTranspose = XMMatrixInverse(nullptr, W);
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetViewMatrix(FXMMATRIX V)
{
	auto& cBuffer = pImpl->m_CBFrame;
	cBuffer.data.view = XMMatrixTranspose(V);
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetProjMatrix(FXMMATRIX P)
{
	auto& cBuffer = pImpl->m_CBOnResize;
	cBuffer.data.proj = XMMatrixTranspose(P);
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetWorldViewProjMatrix(FXMMATRIX W, CXMMATRIX V, CXMMATRIX P)
{
	pImpl->m_CBInstDrawing.data.world = XMMatrixTranspose(W);
	pImpl->m_CBInstDrawing.data.worldInvTranspose = XMMatrixInverse(nullptr, W);
	pImpl->m_CBFrame.data.view = XMMatrixTranspose(V);
	pImpl->m_CBOnResize.data.proj = XMMatrixTranspose(P);

	auto& pCBuffers = pImpl->cBufferPtrs;
	pCBuffers[0]->isDirty = pCBuffers[3]->isDirty = pCBuffers[4]->isDirty = true;
	pImpl->m_IsDirty = true;
}

void BasicEffect::SetDirLight(size_t pos, const DirectionalLight & dirLight)
{
	auto& cBuffer = pImpl->m_CBRarely;
	cBuffer.data.dirLight[pos] = dirLight;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetPointLight(size_t pos, const PointLight & pointLight)
{
	auto& cBuffer = pImpl->m_CBRarely;
	cBuffer.data.pointLight[pos] = pointLight;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetSpotLight(size_t pos, const SpotLight & spotLight)
{
	auto& cBuffer = pImpl->m_CBRarely;
	cBuffer.data.spotLight[pos] = spotLight;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetMaterial(const Material & material)
{
	auto& cBuffer = pImpl->m_CBObjDrawing;
	cBuffer.data.material = material;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetTexIndex(int index)
{
	auto& cBuffer = pImpl->m_CBDrawing;
	cBuffer.data.texIndex = index;
	
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetTextureUsed(bool isUsed)
{
	auto& cBuffer = pImpl->m_CBStates;
	cBuffer.data.textureUsed = isUsed;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetTextureDiffuse(ID3D11ShaderResourceView * textureDiffuse)
{
	pImpl->m_pTextureDiffuse = textureDiffuse;
}

void BasicEffect::SetTextureCube(ID3D11ShaderResourceView * textureCube)
{
	pImpl->m_pTextureCube = textureCube;
}

void BasicEffect::SetCamPos(DirectX::XMFLOAT3 camPos)
{
	auto& cBuffer = pImpl->m_CBFrame;
	cBuffer.data.eyePos = XMLoadFloat3(&camPos);
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetReflectionEnabled(bool isEnable)
{
	auto& cBuffer = pImpl->m_CBStates;
	cBuffer.data.reflectionEnabled = isEnable;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::Apply(ComPtr<ID3D11DeviceContext> deviceContext)
{
	auto& pCBuffers = pImpl->cBufferPtrs;
	// ���������󶨵���Ⱦ������
	pCBuffers[0]->BindVS(deviceContext);
	pCBuffers[3]->BindVS(deviceContext);
	pCBuffers[4]->BindVS(deviceContext);

	pCBuffers[1]->BindPS(deviceContext);
	pCBuffers[2]->BindPS(deviceContext);
	pCBuffers[3]->BindPS(deviceContext);
	pCBuffers[5]->BindPS(deviceContext);
	pCBuffers[6]->BindPS(deviceContext);

	// ��������
	deviceContext->PSSetShaderResources(0, 1, pImpl->m_pTextureDiffuse.GetAddressOf());
	deviceContext->PSSetShaderResources(1, 1, pImpl->m_pTextureCube.GetAddressOf());

	if (pImpl->m_IsDirty)
	{
		pImpl->m_IsDirty = false;
		for (auto& pCBuffer : pCBuffers)
		{
			pCBuffer->UpdateBuffer(deviceContext);
		}
	}
}



