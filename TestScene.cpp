#define  _CRT_SECURE_NO_WARNINGS
#include "TestScene.h"
#include "Player.h"
#include "Ground.h"
#include "Engine/Camera.h"
#include "Engine/Text.h"

namespace {
	Ground* pGround;
	const int CAMERA_HEIGHT = 8.0f;
	XMFLOAT3 START_POS = { 15.0f, 0.75f, 0.5f };
	const float END_POS_X = 43.0f;
}

TestScene::TestScene(GameObject* parent)
	: GameObject(parent, "TestScene"), myScore(0) {}

void TestScene::Initialize()
{
	pPlayer_ = Instantiate <Player>(this);
	pGround = Instantiate<Ground>(this);
	pPlayer_->SetGround(pGround);

	Camera::SetPosition({ pPlayer_->GetPosition().x, pPlayer_->GetPosition().y + CAMERA_HEIGHT,-22 });
	Camera::SetTarget({ pPlayer_->GetPosition().x, pPlayer_->GetPosition().y + CAMERA_HEIGHT,0 });

	pText_ = new Text;
	pText_->Initialize();//テキストの初期化
}

void TestScene::Update()
{
	if (pPlayer_->GetPosition().x > START_POS.x && pPlayer_->GetPosition().x < END_POS_X) {
		Camera::SetPosition({ pPlayer_->GetPosition().x, START_POS.y + CAMERA_HEIGHT,-22 });
		Camera::SetTarget({ pPlayer_->GetPosition().x, START_POS.y + CAMERA_HEIGHT,0 });
	}
}

void TestScene::Draw(){}

void TestScene::Release()
{
	pText_->Release();//テキストの開放
}