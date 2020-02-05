#include "GameApp.h"
#include "d3dUtil.h"
using namespace DirectX;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
	if (!D3DApp::Init())
		return false;

	// 务必先初始化所有渲染状态，以供下面的特效使用
	RenderStates::InitAll(md3dDevice);
	
	if (!mBasicEffect.InitAll(md3dDevice))
		return false;

	if (!mSkyEffect.InitAll(md3dDevice))
		return false;
	
	if (!InitResource())
		return false;

	// 初始化鼠标，键盘不需要
	mMouse->SetWindow(mhMainWnd);
	mMouse->SetMode(DirectX::Mouse::MODE_ABSOLUTE);

	// 初始化滑动延迟时间和点击位置
	mSlideDelay = 0.05f;
	mClickPosX = mClickPosY = -1;
	// 初始化计时器
	mGameTimer.Reset();
	mGameTimer.Stop();
	// 初始化游戏状态
	mGameStatus = GameStatus::Preparing;
	mCurrRotationRecord.dTheta = 0.0f;
	return true;
}

void GameApp::OnResize()
{
	assert(md2dFactory);
	assert(mdwriteFactory);
	// 释放D2D的相关资源
	mColorBrush.Reset();
	md2dRenderTarget.Reset();

	D3DApp::OnResize();

	// 为D2D创建DXGI表面渲染目标
	ComPtr<IDXGISurface> surface;
	HR(mSwapChain->GetBuffer(0, __uuidof(IDXGISurface), reinterpret_cast<void**>(surface.GetAddressOf())));
	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
	HRESULT hr = md2dFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, md2dRenderTarget.GetAddressOf());
	surface.Reset();

	if (hr == E_NOINTERFACE)
	{
		OutputDebugString(L"\n警告：Direct2D与Direct3D互操作性功能受限，你将无法看到文本信息。现提供下述可选方法：\n"
			"1. 对于Win7系统，需要更新至Win7 SP1，并安装KB2670838补丁以支持Direct2D显示。\n"
			"2. 自行完成Direct3D 10.1与Direct2D的交互。详情参阅："
			"https://docs.microsoft.com/zh-cn/windows/desktop/Direct2D/direct2d-and-direct3d-interoperation-overview""\n"
			"3. 使用别的字体库，比如FreeType。\n\n");
	}
	else if (hr == S_OK)
	{
		// 创建固定颜色刷和文本格式
		HR(md2dRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::White),
			mColorBrush.GetAddressOf()));
		HR(mdwriteFactory->CreateTextFormat(L"宋体", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 20, L"zh-cn",
			mTextFormat.GetAddressOf()));
	}
	else
	{
		// 报告异常问题
		assert(md2dRenderTarget);
	}

	// 摄像机变更显示
	if (mCamera != nullptr)
	{
		mCamera->SetFrustum(XM_PI * 0.4f, AspectRatio(), 1.0f, 1000.0f);
		mCamera->SetViewPort(0.0f, 0.0f, (float)mClientWidth, (float)mClientHeight);
		mBasicEffect.SetProjMatrix(mCamera->GetProjXM());
	}
}

void GameApp::UpdateScene(float dt)
{
	// 键鼠更新
	if (mGameStatus == GameStatus::Preparing)
	{
		// 播放摄像机动画
		bool animComplete = PlayCameraAnimation(dt);

		if (animComplete)
		{
			mGameStatus = GameStatus::Ready;
		}
	}
	else
	{
		KeyInput(dt);
		MouseInput(dt);
	}
	
	// 仅实质性旋转才会计时
	if (mGameStatus == GameStatus::Ready)
	{
		// 开始游戏，计时
		mGameTimer.Start();
		mGameStatus = GameStatus::Playing;
	}
	else if (mGameStatus == GameStatus::Playing)
	{
		mGameTimer.Tick();
	}

	// 更新汽车
	mCar.Update(dt);

}

void GameApp::DrawScene()
{
	assert(md3dImmediateContext);
	assert(mSwapChain);

	// 使用偏紫色的纯色背景
	float backgroundColor[4] = { 0.45882352f, 0.42745098f, 0.51372549f, 1.0f };
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.Get(), backgroundColor);
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	
	// 绘制汽车
	mBasicEffect.SetRenderDefault(md3dImmediateContext, BasicEffect::RenderObject);
	mBasicEffect.SetReflectionEnabled(false);
	mCar.Draw(md3dDevice, md3dImmediateContext, mBasicEffect);

	mGround.Draw(md3dImmediateContext, mBasicEffect);
	mCylinder.Draw(md3dImmediateContext, mBasicEffect);
	
	// 绘制天空盒
	mSkyEffect.SetRenderDefault(md3dImmediateContext);
	mDaylight->Draw(md3dImmediateContext, mSkyEffect, *mCamera);
	
	//
	// 绘制Direct2D部分
	//
	if (md2dRenderTarget != nullptr)
	{
		md2dRenderTarget->BeginDraw();

		// 用于Debug输出
		Mouse::State mouseState = mMouse->GetState();
		std::wstring wstr = L"F1第三人称镜头，鼠标左键按住移动镜头，滚轮缩放\nF2第一人称镜头，鼠标直接移动镜头\nW、A、S、D移动";
		md2dRenderTarget->DrawTextW(wstr.c_str(), (UINT)wstr.size(), mTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, mColorBrush.Get());
		HR(md2dRenderTarget->EndDraw());
	}

	HR(mSwapChain->Present(0, 0));
}

bool GameApp::PlayCameraAnimation(float dt)
{
	// 获取子类
	auto cam3rd = dynamic_cast<ThirdPersonCamera*>(mCamera.get());

	// ******************
	// 第三人称摄像机的操作
	//
	mAnimationTime += dt;
	float theta, dist;

	theta = -XM_PIDIV2 + XM_PIDIV4 * mAnimationTime * 0.2f;
	dist = 20.0f - mAnimationTime * 2.0f;
	if (theta > -XM_PIDIV4)
		theta = -XM_PIDIV4;
	if (dist < 10.0f)
		dist = 10.0f;

	cam3rd->SetRotationY(theta);
	cam3rd->SetDistance(dist);
	cam3rd->SetTarget(mCar.GetPosition());

	// 更新观察矩阵
	mCamera->UpdateViewMatrix();
	mBasicEffect.SetViewMatrix(mCamera->GetViewXM());

	if (fabs(theta + XM_PIDIV4) < 1e-5f && fabs(dist - 10.0f) < 1e-5f)
		return true;
	return false;
}


bool GameApp::InitResource()
{
	// 初始化天空盒相关
	mDaylight = std::make_unique<SkyRender>(
		md3dDevice.Get(), md3dImmediateContext.Get(),
		L"Resource\\daylight.jpg",
		5000.0f);
	mBasicEffect.SetTextureCube(mDaylight->GetTextureCube());
	// ******************
	// 初始化游戏对象
	//

	Model model;
	// 地面
	model.SetMesh(md3dDevice.Get(),
		Geometry::CreatePlane(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(100.0f, 100.0f), XMFLOAT2(5.0f, 5.0f)));
	model.modelParts[0].material.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	model.modelParts[0].material.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	model.modelParts[0].material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	model.modelParts[0].material.Reflect = XMFLOAT4();
	HR(CreateDDSTextureFromFile(md3dDevice.Get(),
		L"Resource\\floor.dds",
		nullptr,
		model.modelParts[0].texDiffuse.GetAddressOf()));
	mGround.SetModel(std::move(model));
	// 柱体
	model.SetMesh(md3dDevice.Get(),
		Geometry::CreateCylinder(3.0f, 20.0f));
	model.modelParts[0].material.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	model.modelParts[0].material.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	model.modelParts[0].material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	model.modelParts[0].material.Reflect = XMFLOAT4();
	HR(CreateDDSTextureFromFile(md3dDevice.Get(),
		L"Resource\\bricks.dds",
		nullptr,
		model.modelParts[0].texDiffuse.GetAddressOf()));
	mCylinder.SetModel(std::move(model));
	mCylinder.SetWorldMatrix(XMMatrixTranslation(-12.0f, 1.0f, 0.0f));
	// 汽车
	mCar.InitResources(md3dDevice, md3dImmediateContext);
	mCar.SetMoveSpeed(5);
	// 初始化摄像机
	mCamType = Third;
	mCamera.reset(new ThirdPersonCamera);
	auto cam3rd = dynamic_cast<ThirdPersonCamera*>(mCamera.get());
	cam3rd->SetDistance(10.0f);
	cam3rd->SetDistanceMinMax(10.0f, 200.0f);
	cam3rd->SetFrustum(XM_PI * 0.4f, AspectRatio(), 1.0f, 1000.0f);
	cam3rd->SetViewPort(0.0f, 0.0f, (float)mClientWidth, (float)mClientHeight);
	cam3rd->SetTarget(mCar.GetPosition());
	cam3rd->SetRotationX(XM_PIDIV2 * 0.6f);
	mBasicEffect.SetProjMatrix(cam3rd->GetProjXM());
	mBasicEffect.SetTextureDiffuse(mCar.GetTexArray().Get());


	// ******************
	// 初始化不会变化的值
	//

	// 方向光
	DirectionalLight dirLight[5];
	PointLight pointLight[5];
	SpotLight spotLight[5];
	dirLight[0].ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	dirLight[0].diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	dirLight[0].specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	dirLight[0].direction = XMFLOAT3(-1, -1, 1);
	
	for (int i = 0; i < 5; ++i)
	{
		pointLight[i].ambient = XMFLOAT4(0, 0, 0, 0);
		pointLight[i].diffuse = XMFLOAT4(0, 0, 0, 0);
		pointLight[i].specular = XMFLOAT4(0, 0, 0, 0);
		spotLight[i].ambient = XMFLOAT4(0, 0, 0, 0);
		spotLight[i].diffuse = XMFLOAT4(0, 0, 0, 0);
		spotLight[i].specular = XMFLOAT4(0, 0, 0, 0);
		if(i > 0)
		{
			dirLight[i].ambient = XMFLOAT4(0, 0, 0, 0);
			dirLight[i].diffuse = XMFLOAT4(0, 0, 0, 0);
			dirLight[i].specular = XMFLOAT4(0, 0, 0, 0);
		}
		mBasicEffect.SetDirLight(i, dirLight[i]);
		mBasicEffect.SetPointLight(i, pointLight[i]);
		mBasicEffect.SetSpotLight(i, spotLight[i]);
	}

	return true;
}

void GameApp::KeyInput(float dt)
{
	Keyboard::State keyState = mKeyboard->GetState();
	mKeyboardTracker.Update(keyState);

	static XMFLOAT2 move = XMFLOAT2(0,0);
	if(move.y == 0)
	{
		if (mKeyboardTracker.IsKeyPressed(Keyboard::W))
		{
			move.y = dt;
		}
		else if (mKeyboardTracker.IsKeyPressed(Keyboard::S))
		{
			move.y = -dt;
		}
	}
	if(move.x == 0)
	{
		if (mKeyboardTracker.IsKeyPressed(Keyboard::A))
		{
			move.x = -1;
		}
		if (mKeyboardTracker.IsKeyPressed(Keyboard::D))
		{
			move.x = 1;
		}
	}
	if((mKeyboardTracker.IsKeyReleased(Keyboard::W) && move.y > 0)||(mKeyboardTracker.IsKeyReleased(Keyboard::S) && move.y < 0))
	{
		move.y = 0;
	}
	if((mKeyboardTracker.IsKeyReleased(Keyboard::D) && move.x > 0)||(mKeyboardTracker.IsKeyReleased(Keyboard::A) && move.x < 0))
	{
		move.x = 0;
	}
	mCar.Move(move);
	//
	// 特殊操作
	//

	if (mKeyboardTracker.IsKeyPressed(Keyboard::F12))
	{
		std::wstring wstr = L"作者：陈镇东\n"
			"版本：v1.0\n"
			"本汽车游戏可供学习和游玩\n";
		MessageBox(nullptr, wstr.c_str(), L"关于作者", MB_OK);
	}
	else if (mKeyboardTracker.IsKeyPressed(Keyboard::F1))
	{
		mCamType = Third;
		mCamera.reset(new ThirdPersonCamera);
		auto cam3rd = dynamic_cast<ThirdPersonCamera*>(mCamera.get());
		cam3rd->SetDistance(10.0f);
		cam3rd->SetDistanceMinMax(10.0f, 200.0f);
		cam3rd->SetFrustum(XM_PI * 0.4f, AspectRatio(), 1.0f, 1000.0f);
		cam3rd->SetViewPort(0.0f, 0.0f, (float)mClientWidth, (float)mClientHeight);
		cam3rd->SetTarget(mCar.GetPosition());
		cam3rd->SetRotationX(XM_PIDIV2 * 0.6f);
		cam3rd->SetRotationY(-XM_PIDIV4);
		mBasicEffect.SetProjMatrix(cam3rd->GetProjXM());
	}
	else if (mKeyboardTracker.IsKeyPressed(Keyboard::F2))
	{
		mCamType = First;
		mCamera.reset(new FirstPersonCamera);
		auto cam1rd = dynamic_cast<FirstPersonCamera*>(mCamera.get());
		cam1rd->SetFrustum(XM_PI * 0.4f, AspectRatio(), 1.0f, 1000.0f);
		cam1rd->SetViewPort(0.0f, 0.0f, (float)mClientWidth, (float)mClientHeight);
		cam1rd->LookAt(mCar.GetPosition(), mCar.GetForwardTarget(), XMFLOAT3(0, 1, 0));
		XMFLOAT3 pos = mCar.GetPosition();
		pos.y += 6;
		cam1rd->SetPosition(pos);
		mBasicEffect.SetProjMatrix(cam1rd->GetProjXM());
	}
	
}

void GameApp::MouseInput(float dt)
{
	Mouse::State mouseState = mMouse->GetState();
	Mouse::State lastState = mMouseTracker.GetLastState();
	mMouseTracker.Update(mouseState);

	int dx = mouseState.x - lastState.x;
	int dy = mouseState.y - lastState.y;

	switch (mCamType)
	{
	case First:
		{
			// 获取子类
			auto cam1rd = dynamic_cast<FirstPersonCamera*>(mCamera.get());
			XMFLOAT3 pos = mCar.GetPosition();
			pos.y += 6;
			cam1rd->SetPosition(pos);

			cam1rd->RotateY(dx * 0.008f + mCar.GetRotateAngle());
			cam1rd->Pitch(dy * 0.008f);
		}
		break;
	case Third:
		{
			// 获取子类
			auto cam3rd = dynamic_cast<ThirdPersonCamera*>(mCamera.get());
			cam3rd->SetTarget(mCar.GetPosition());
			// 绕物体旋转
			if (mouseState.leftButton)
			{
				cam3rd->RotateX(dy * 0.008f);
				cam3rd->RotateY(dx * 0.008f);
			}
			cam3rd->Approach(-mouseState.scrollWheelValue / 120 * 1.0f);
		}
		break;
	}

	// 更新观察矩阵
	mCamera->UpdateViewMatrix();
	mBasicEffect.SetViewMatrix(mCamera->GetViewXM());
	mBasicEffect.SetCamPos(mCamera->GetPosition());
	
	// 重置滚轮值
	mMouse->ResetScrollWheelValue();
	
}

std::wstring GameApp::floating_to_wstring(float val, int precision)
{
	std::wostringstream oss;
	oss.setf(std::ios::fixed);
	oss.precision(precision);
	oss << val;
	return oss.str();
}

