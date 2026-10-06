#pragma once
#include "Engine/GameObject.h"
#include "Engine/SphereCollider.h"

class Ground;//前方宣言

class Player : public GameObject
{
public:
	Player(GameObject* parent);
	void Initialize() override;//初期化
	void Update() override;//更新
	void Draw() override;//描画
	void Release() override;//開放
	void SetGround(Ground* ground) { ground_ = ground; }
	void OnCollision(GameObject* pTarget) override;

	void Carry(float dx, float dy) { transform_.position_.x += dx; transform_.position_.y += dy; }
	int GetRidingFloor() const { return ridingFloor_; }
private:
	// プレイヤーの状態
	enum PLAYER_STATE
	{
		PLAYER_IDLE,
		PLAYER_WALK,
		PLAYER_TURN,
		PLAYER_STATE_MAX
	};

	// プレイヤーの向き
	enum PLAYER_DIRECTION
	{
		PLAYER_UP,
		PLAYER_DOWN,
		PLAYER_LEFT,
		PLAYER_RIGHT,
		PLAYER_DIRECTION_MAX
	};

	bool HandleInput();												// 入力処理、ブレーキ中ならtrue
	bool UpdateTurn();												// 回転処理、回転中ならtrue
	void UpdateJump();												// ジャンプ・重力処理
	void ResolveWallCollision(XMVECTOR& pos, const XMVECTOR& move);	// 壁当たり判定

	int hWalkModel_;
	int hIdleModel_;				// 待機アニメーションのモデルハンドル

	Ground* ground_;				// 地面オブジェクトのポインタ

	// 状態変数
	PLAYER_STATE pstate_;						// プレイヤーの状態
	PLAYER_DIRECTION pdirection_;				// プレイヤーの向き

	float turnStartAngle_;						// 回転開始時の角度
	float turnEndAngle_;						// 回転終了時の角度
	PLAYER_DIRECTION turnEndDirection_;			// 回転終了時の向き

	float currentSpeed_;						// 現在の速度
	float turnFrame_;							// 回転中のフレーム数

	float jumpVelocity_;						// ジャンプ中の垂直速度
	bool isGrounded_;							// 地面に接地しているか

	int ridingFloor_;	// 乗っている動く床の番号（-1なら無し）
};