#ifndef CAR_H
#define CAR_H

#include <wrl/client.h>
#include "Effects.h"
#include "Collision.h"
#include <vector>
#include <stack>
#include "Geometry.h"
#include "LightHelper.h"

enum CarFaceColor {
	CarFaceColor_Black,		// 黑色
	CarFaceColor_Orange,		// 橙色
	CarFaceColor_Red,			// 红色
	CarFaceColor_Green,		// 绿色
	CarFaceColor_Blue,		// 蓝色
	CarFaceColor_Yellow,		// 黄色
	CarFaceColor_White,		// 白色
	Tire0,					//侧面
	Tire1,					//胎面
};

enum CarFace {
	CarFace_PosX,		// +X面
	CarFace_NegX,		// -X面
	CarFace_PosY,		// +Y面
	CarFace_NegY,		// -Y面
	CarFace_PosZ,		// +Z面
	CarFace_NegZ,		// -Z面
};

enum CarRotationAxis {
	CarRotationAxis_X,	// 绕X轴旋转
	CarRotationAxis_Y,	// 绕Y轴旋转
	CarRotationAxis_Z,	// 绕Z轴旋转
};

struct CarRotationRecord
{
	CarRotationAxis axis;	// 当前旋转轴
	int pos;				// 当前旋转层的索引
	float dTheta;			// 当前旋转的弧度
};

struct Cube
{
	DirectX::XMMATRIX GetWorldMatrix() const;

	CarFaceColor faceColors[6];	// 六个面的颜色，索引0-5分别对应+X, -X, +Y, -Y, +Z, -Z面
	DirectX::XMFLOAT3 pos;			// 旋转结束后中心所处位置
	DirectX::XMFLOAT3 rotation;		// 仅允许存在单轴旋转，记录当前分别绕x轴, y轴, z轴旋转的弧度

};

struct Cylinder
{
	DirectX::XMMATRIX GetWorldMatrix() const;

	CarFaceColor faceColors[2];	
	DirectX::XMFLOAT3 pos;			// 旋转结束后中心所处位置
	DirectX::XMFLOAT3 rotation;		// 仅允许存在单轴旋转，记录当前分别绕x轴, y轴, z轴旋转的弧度

};


class Car
{
public:

	struct PSConstantBuffer
	{
		DirectionalLight dirLight[10];
		PointLight pointLight[10];
		SpotLight spotLight[10];
		Material material;
		int numDirLight;
		int numPointLight;
		int numSpotLight;
		float pad;		// 打包保证16字节对齐
		DirectX::XMFLOAT4 eyePos;
	};
	
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	Car();

	// 初始化资源
	void InitResources(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> deviceContext);
	// 设置需要渲染的模型
	template<class VertexType>
	void SetMesh(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> deviceContext, const Geometry::MeshData<VertexType>& meshData);
	// 立即复原魔方
	void Reset();
	// 更新魔方状态
	void Update(float dt);
	// 绘制魔方
	void Draw(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> deviceContext, BasicEffect& effect);

	const DirectX::XMFLOAT3 & GetPosition();
	const float & GetRotateAngle();
	const DirectX::XMFLOAT3 & GetForwardTarget();

	void Move(DirectX::XMFLOAT2 move);

	// 当前射线拾取到哪个立方体(只考虑可见立方体)的对应索引，未找到则返回(-1, -1, -1)
	DirectX::XMINT3 HitCube(Ray ray, float * pDist = nullptr) const;
	

	// 设置旋转速度(rad/s)
	void SetMoveSpeed(float speed);

	// 获取纹理数组
	ComPtr<ID3D11ShaderResourceView> GetTexArray() const;

private:
	// 从内存中创建纹理
	ComPtr<ID3D11ShaderResourceView> CreateCarCubeTextureArrayFromMemory(ComPtr<ID3D11Device> device,
		ComPtr<ID3D11DeviceContext> deviceContext);

private:
	Material mMaterial;
	// 车身 [X][Y][Z]
	Cube mCubes[6][6][6];
	Cylinder mCylinder[4];
	
	// 当前自动旋转的速度
	float mSpeed;
	float mRotateAngle;

	// 顶点缓冲区
	ComPtr<ID3D11Buffer> mVertexBuffer;	

	// 索引缓冲区，仅6个索引
	ComPtr<ID3D11Buffer> mIndexBuffer;
	
	// 实例缓冲区，

	// 纹理数组，包含7张纹理
	ComPtr<ID3D11ShaderResourceView> mTexArray;

	DirectX::XMFLOAT3 mPosition;
	DirectX::XMFLOAT3 mForwardTarget;
};




#endif