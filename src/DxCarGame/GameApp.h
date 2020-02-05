#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Car.h"
#include "Camera.h"
#include "SkyRender.h"
#include <ctime>
#include <sstream>
#include "GameObject.h"

class GameApp : public D3DApp
{
public:
	enum class GameStatus {
		Preparing,	// 准备中
		Ready,		// 就绪
		Playing,	// 游玩中
		Finished,	// 已完成
	};

public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();


private:

	// 播放摄像机动画，完成动画将返回true
	bool PlayCameraAnimation(float dt);

	bool InitResource();

	void KeyInput(float dt);
	void MouseInput(float dt);

	std::wstring floating_to_wstring(float val, int precision);

private:
	ComPtr<ID2D1SolidColorBrush> mColorBrush;	// 单色笔刷
	ComPtr<IDWriteFont> mFont;					// 字体
	ComPtr<IDWriteTextFormat> mTextFormat;		// 文本格式

	Car mCar;									// 汽车
	GameObject mGround;							// 地面
	GameObject mCylinder;						// 圆柱
	
	std::unique_ptr<Camera> mCamera;			// 摄像机
	CamType mCamType;							// 摄像机类型

	BasicEffect mBasicEffect;					// 基础特效管理类
	SkyEffect mSkyEffect;						// 天空盒特效管理
	std::unique_ptr<SkyRender> mDaylight;		// 天空盒(白天)

	GameTimer mGameTimer;						// 游戏计时器
	GameStatus mGameStatus;						// 游戏状态
	bool mIsCompleted;							// 是否完成

	float mAnimationTime;						// 动画经过时间

	//
	// 鼠标操作控制
	//
	
	int mClickPosX, mClickPosY;					// 初次点击时鼠标位置
	float mSlideDelay;							// 拖动延迟响应时间 
	float mCurrDelay;							// 当前延迟时间
	bool mDirectionLocked;						// 方向锁

	CarRotationRecord mCurrRotationRecord;	// 当前旋转记录
};


#endif