#include "Item.h"
#include "Engine/Model.h"
#include "Engine/SphereCollider.h"
#include "TestScene.h"
#include "Ground.h"

Item::Item(GameObject* parent)
	:GameObject(parent, "Item"), type_(ITEMTYPE_NORMAL), hModel_(-1), score_(0){}

Item::~Item(){}

void Item::Initialize()
{
	transform_.scale_ = { 0.3f, 0.3f, 0.3f };
}

void Item::Update()
{
	if (type_ == ItemType::ITEMTYPE_POWER)
	{
		transform_.rotate_.y += 1.0f;
	}
}

void Item::Draw()
{
	Model::SetTransform(hModel_, transform_);
	Model::Draw(hModel_);
}

void Item::Release(){}

void Item::SetItemType(ItemType type)
{
	type_ = type;
	if (type_ == ItemType::ITEMTYPE_NORMAL)
	{
		SphereCollider* collision = new SphereCollider(XMFLOAT3(0, 0.3, 0), 0.16f);
		AddCollider(collision);
		hModel_ = Model::Load("Item.fbx");
		score_ = 1;
	}
	else if (type_ == ItemType::ITEMTYPE_POWER)
	{
		SphereCollider* collision = new SphereCollider(XMFLOAT3(0, 0.6, 0), 0.32f);
		AddCollider(collision);
		hModel_ = Model::Load("PItem.fbx");
		score_ = 5;
	}
}

void Item::OnCollision(GameObject* pTarget)
{
	TestScene* testScene = dynamic_cast<TestScene*>(GetParent()->GetParent());
	testScene->AddScore(score_);//スコアを加算
	Ground* ground = dynamic_cast<Ground*>(FindObject("Ground"));
	if (pTarget->GetObjectName() == "Player")
	{
		KillMe();
	}
}