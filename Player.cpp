#include "Player.h"
#include "Engine/Model.h"
#include "Engine/Debug.h"
#include "Engine/Input.h"
#include "TestScene.h"
#include "Ground.h"
#include <cmath>

namespace 
{
	// ------------------------------------------------------------
	// プレイヤーの移動に関する定数
	// ------------------------------------------------------------
	const float MAX_SPEED = 0.2f;			// 最大移動速度
	const float BASE_SPEED = 0.1f;			// アニメーション速度1.0になる基準速度
	const float ACCELERATION = 0.005f;		// 移動入力中の加速度
	const float FRICTION = 0.008f;			// 入力を離したときの減速度
	const float BRAKE = 0.02f;				// 進行方向と逆入力したときの減速度
	const float TURN_FRAME = 10.0f;			// 方向転換にかけるフレーム数
	const float BLOCK_INTERVAL_X = 2.0f;			// マップ1マス分のワールドサイズ

	// プレイヤーの初期位置
	const XMFLOAT3 START_POS = { 15.0f, 0.75f, 0.5f };

	// ------------------------------------------------------------
	// ジャンプに関する定数
	// ------------------------------------------------------------
	const float JUMP_POWER = 0.2f;			// ジャンプ開始時の上向き速度
	const float GRAVITY = 0.01f;			// 1フレームごとに減少する垂直速度
	const float AIR_CONTROL = 0.5f;			// 空中での加速・減速の強さ（地上比）

	// ブロックの配置間隔・水平寸法・ゲーム上の歩行面。
	const float BLOCK_INTERVAL_Y = 1.0f;
	const float BLOCK_HALF_WIDTH = 0.99375f;
	const float BLOCK_SURFACE_HEIGHT = 0.75f;

	// プレイヤーの判定寸法。原点を足元として扱う。
	const float PLAYER_FOOT_OFFSET = 0.0f;
	// 身長はマップの縦2マス。判定寸法はワールド座標で定義する。
	const float PLAYER_HEIGHT = BLOCK_INTERVAL_Y * 2.0f;
	const float PLAYER_MODEL_HEIGHT = 3.76537f;
	const float PLAYER_MODEL_SCALE = PLAYER_HEIGHT / PLAYER_MODEL_HEIGHT;
	// 横幅は従来の判定幅を描画モデルと同じ割合で縮小する。
	const float PLAYER_HALF_WIDTH = 0.4f * PLAYER_MODEL_SCALE;
	const float CONTACT_EPSILON = 0.0001f;
	const float WALL_WALK_ANIM_SPEED = 1.0f; // 壁押し中の歩行再生速度

	struct CollisionRect
	{
		float left, right, bottom, top;
	};

	CollisionRect MakePlayerRect(const XMFLOAT3& position)
	{
		const float foot = position.y - PLAYER_FOOT_OFFSET;
		return { position.x - PLAYER_HALF_WIDTH,
			position.x + PLAYER_HALF_WIDTH, foot, foot + PLAYER_HEIGHT };
	}

	CollisionRect MakeBlockRect(int row, int col, int mapHeight)
	{
		const float x = col * BLOCK_INTERVAL_X;
		const float y = (mapHeight - 1 - row) * BLOCK_INTERVAL_Y;
		return { x - BLOCK_HALF_WIDTH, x + BLOCK_HALF_WIDTH,
			y, y + BLOCK_SURFACE_HEIGHT };
	}

	bool OverlapX(const CollisionRect& a, const CollisionRect& b)
	{
		return a.right > b.left + CONTACT_EPSILON &&
			a.left < b.right - CONTACT_EPSILON;
	}

	bool OverlapY(const CollisionRect& a, const CollisionRect& b)
	{
		return a.top > b.bottom + CONTACT_EPSILON &&
			a.bottom < b.top - CONTACT_EPSILON;
	}

	// ------------------------------------------------------------
	// プレイヤーの向きに対応する角度
	//
	// PLAYER_DIRECTION の並びと同じ順番にしている
	// UP / DOWN / LEFT / RIGHT
	// ------------------------------------------------------------
	const float P_ANGLE[4] =
	{
		180.0f,
		0.0f,
		90.0f,
		270.0f
	};

	// ------------------------------------------------------------
	// プレイヤーの向きに対応する移動ベクトル
	// ------------------------------------------------------------
	const XMVECTOR P_MOVE[4] =
	{
		XMVectorSet(0, 0,  1, 0),
		XMVectorSet(0, 0, -1, 0),
		XMVectorSet(-1, 0,  0, 0),
		XMVectorSet(1, 0,  0, 0)
	};

	// ------------------------------------------------------------
	// 角度差を -180度 ～ 180度 の範囲に補正する
	//
	// 例：
	//   270度回転する代わりに -90度回転させることで、
	//   常に近い方向へ回転できるようにする
	// ------------------------------------------------------------
	float AdjustAngle(float angle)
	{
		if (angle >= 180.0f)
		{
			angle -= 360.0f;
		}
		else if (angle < -180.0f)
		{
			angle += 360.0f;
		}
		return angle;
	}
}

CollisionRect MakeFloorRect(const MovingFloor& f)
{
	return { f.x - BLOCK_HALF_WIDTH, f.x + BLOCK_HALF_WIDTH,
		f.y, f.y + BLOCK_SURFACE_HEIGHT };
}

Player::Player(GameObject* parent)
	: GameObject(parent, "Player"),
	hWalkModel_(-1),
	hIdleModel_(-1),
	ground_(nullptr),
	pstate_(PLAYER_IDLE),
	pdirection_(PLAYER_DOWN),
	turnStartAngle_(0.0f),
	turnEndAngle_(0.0f),
	turnEndDirection_(PLAYER_DOWN),
	currentSpeed_(0.0f),
	turnFrame_(0.0f),
	jumpVelocity_(0.0f),
	isGrounded_(true), 
	ridingFloor_(-1)//これ増えた
{
}

// ------------------------------------------------------------
// 初期化
//
// モデル、アニメーション、初期位置、コライダーを設定する
// ------------------------------------------------------------
void Player::Initialize()
{
	// 歩行用モデル
	hWalkModel_ = Model::Load("Zombie_Walk.fbx");
	if (hWalkModel_ == -1)
	{
		Debug::Log("Walking.fbxの読み込みに失敗しました", true);
	}
	else
	{
		Model::SetAnimFrame(hWalkModel_, 0, 59, 1.0);
	}

	// 初期位置
	transform_.position_ = START_POS;

	// 待機用モデル
	hIdleModel_ = Model::Load("Zombie_Idle.fbx");
	if (hIdleModel_ == -1)
	{
		Debug::Log("Idle.fbxの読み込みに失敗しました", true);
	}
	else
	{
		Model::SetAnimFrame(hIdleModel_, 0, 117, 1.0);
	}

	// プレイヤー用の球コライダー
	SphereCollider* collision =
		new SphereCollider(XMFLOAT3(0, 0.25f, 0), 0.5f);

	AddCollider(collision);
}

void Player::Update()
{
	// 方向転換中でなければ、いったん待機状態に戻す
	// この後 HandleInput() で移動入力があれば WALK に変わる
	if (pstate_ != PLAYER_TURN)
	{
		pstate_ = PLAYER_IDLE;
	}

	// 入力を調べる
	// 進行方向と逆方向が入力されていれば true が返る
	bool isBraking = HandleInput();

	// 方向転換中は、そのフレームでは通常の移動処理を行わない
	if (UpdateTurn())
	{
		UpdateJump();
		return;
	}

	// 現在位置を DirectXMath のベクトルとして取得
	XMVECTOR pos = XMLoadFloat3(&transform_.position_);

	// このフレームで進む方向
	// 入力がなければゼロベクトル
	XMVECTOR move = XMVectorSet(0, 0, 0, 0);

	// --------------------------------------------------------
	// 移動入力中
	// --------------------------------------------------------
	if (pstate_ == PLAYER_WALK)
	{
		// 空中では地上より加速を弱くする
		float accel = isGrounded_
			? ACCELERATION
			: ACCELERATION * AIR_CONTROL;

		currentSpeed_ += accel;

		// 最大速度を超えないようにする
		if (currentSpeed_ > MAX_SPEED)
		{
			currentSpeed_ = MAX_SPEED;
		}

		// 現在向いている方向へ移動する
		move = P_MOVE[pdirection_];

		// モデルの向きも移動方向に合わせる
		transform_.rotate_.y = P_ANGLE[pdirection_];
	}
	// --------------------------------------------------------
	// 移動入力がない場合
	// --------------------------------------------------------
	else
	{
		// 慣性で動いている間は徐々に減速する
		if (currentSpeed_ > 0.0f)
		{
			float decel;

			if (isGrounded_)
			{
				// 地上では、
				// 逆方向入力中なら強いブレーキ、
				// 入力なしなら通常の摩擦で減速する
				decel = isBraking ? BRAKE : FRICTION;
			}
			else
			{
				// 空中では減速を弱くする
				decel = FRICTION * AIR_CONTROL;
			}

			currentSpeed_ -= decel;

			// 速度がマイナスにならないようにする
			if (currentSpeed_ < 0.0f)
			{
				currentSpeed_ = 0.0f;
			}

			// 入力を離しても、減速中はこれまでの方向へ進み続ける
			move = P_MOVE[pdirection_];
		}
	}

	// --------------------------------------------------------
	// 水平方向の移動
	// --------------------------------------------------------
	pos = pos + currentSpeed_ * move;
	XMStoreFloat3(&transform_.position_, pos);

	// 移動後の位置が壁に入っていないか調べる
	// 壁に入っていた場合は水平方向の移動を取り消す
	const float speedBeforeCollision = currentSpeed_;
	ResolveWallCollision(pos, move);

	// 水平方向の処理が終わってから、
	// ジャンプ・重力によるY方向の移動を行う
	const bool blockedByWall =
		speedBeforeCollision > 0.0f && currentSpeed_ == 0.0f;

	// 実際の移動速度と、壁に向かって歩くアニメの速度を分離する。
	// 入力を離した場合や逆方向へのブレーキ中は固定再生しない。
	const bool pushingWall = blockedByWall && pstate_ == PLAYER_WALK;
	const float walkAnimSpeed = pushingWall
		? WALL_WALK_ANIM_SPEED
		: currentSpeed_ / BASE_SPEED;
	Model::SetAnimSpeed(hWalkModel_, walkAnimSpeed);

	// ジャンプ・重力・ブロックへの着地
	UpdateJump();
}

bool Player::HandleInput()
{
	bool isBraking = false;

	// 入力前の向きを保存しておく
	// 入力後に向きが変わったかを判定するために使用する
	PLAYER_DIRECTION oldDir = pdirection_;

	// 方向転換中は新しい左右入力を受け付けない
	if (pstate_ != PLAYER_TURN)
	{
		// ----------------------------------------------------
		// 完全に停止していて、地上にいる場合
		//
		// 左右入力された方向へすぐに向きを変更する
		// ----------------------------------------------------
		if (currentSpeed_ == 0.0f && isGrounded_)
		{
			if (Input::IsKey(DIK_LEFT))
			{
				pdirection_ = PLAYER_LEFT;
				pstate_ = PLAYER_WALK;
			}

			if (Input::IsKey(DIK_RIGHT))
			{
				pdirection_ = PLAYER_RIGHT;
				pstate_ = PLAYER_WALK;
			}
		}

		// ----------------------------------------------------
		// すでに移動中、または空中にいる場合
		// ----------------------------------------------------
		else
		{
			if (Input::IsKey(DIK_LEFT))
			{
				// 左へ進んでいる状態で左入力
				// →そのまま移動を続ける
				if (pdirection_ == PLAYER_LEFT)
				{
					pstate_ = PLAYER_WALK;
				}

				// 右へ進んでいる状態で左入力
				// →地上ならブレーキをかける
				else if (pdirection_ == PLAYER_RIGHT)
				{
					isBraking = isGrounded_;
				}
			}

			if (Input::IsKey(DIK_RIGHT))
			{
				// 右へ進んでいる状態で右入力
				// →そのまま移動を続ける
				if (pdirection_ == PLAYER_RIGHT)
				{
					pstate_ = PLAYER_WALK;
				}

				// 左へ進んでいる状態で右入力
				// →地上ならブレーキをかける
				else if (pdirection_ == PLAYER_LEFT)
				{
					isBraking = isGrounded_;
				}
			}
		}
	}

	// --------------------------------------------------------
	// ジャンプ開始
	//
	// Spaceを押した瞬間、かつ地上にいる場合だけジャンプする
	// --------------------------------------------------------
	if (Input::IsKeyDown(DIK_SPACE) && isGrounded_)
	{
		jumpVelocity_ = JUMP_POWER;
		isGrounded_ = false;
	}

	// --------------------------------------------------------
	// 入力によって向きが変わった場合は方向転換を開始する
	// --------------------------------------------------------
	if (oldDir != pdirection_)
	{
		pstate_ = PLAYER_TURN;

		// 方向転換の経過フレームをリセット
		turnFrame_ = 0.0f;

		// 回転開始時の角度
		turnStartAngle_ = P_ANGLE[oldDir];

		// 回転量を -180～180度に補正し、
		// 最短方向へ回転させる
		float diff =
			AdjustAngle(P_ANGLE[pdirection_] - P_ANGLE[oldDir]);

		// 回転完了後の向き
		turnEndDirection_ = pdirection_;

		// 補間に使用する終了角度
		turnEndAngle_ = turnStartAngle_ + diff;
	}

	return isBraking;
}

bool Player::UpdateTurn()
{
	if (pstate_ != PLAYER_TURN)
	{
		return false;
	}

	// 方向転換開始からの経過フレーム
	turnFrame_ += 1.0f;

	// 0.0 ～ 1.0 の補間率を求める
	float t = min(turnFrame_ / TURN_FRAME, 1.0f);

	// 開始角度から終了角度まで線形補間する
	transform_.rotate_.y =
		turnStartAngle_
		+ (turnEndAngle_ - turnStartAngle_) * t;

	// 指定フレーム数に到達したら方向転換終了
	if (turnFrame_ >= TURN_FRAME)
	{
		pdirection_ = turnEndDirection_;

		// 補間誤差が残らないように最終角度を設定する
		transform_.rotate_.y = P_ANGLE[pdirection_];

		// 回転後は歩行状態へ戻す
		pstate_ = PLAYER_WALK;
	}

	return true;
}

void Player::UpdateJump()
{
	if (ground_ == nullptr)
		return;

	const auto& gmap = ground_->GetMapData();
	const int mapHeight = static_cast<int>(gmap.size());
	const CollisionRect before = MakePlayerRect(transform_.position_);

	ridingFloor_ = -1;
	const auto& floors = ground_->GetMovingFloors();

	if (isGrounded_)
	{
		// 既存仕様の常設床。穴を作る場合はこの床もマップで管理する。
		bool supported = transform_.position_.y <= START_POS.y + CONTACT_EPSILON;
		float supportY = START_POS.y;
		for (int row = 0; row < mapHeight; ++row)
		{
			for (int col = 0; col < static_cast<int>(gmap[row].size()); ++col)
			{
				if (gmap[row][col] != 1) continue;
				const CollisionRect block = MakeBlockRect(row, col, mapHeight);
				if (OverlapX(before, block) &&
					std::fabs(before.bottom - block.top) <= CONTACT_EPSILON)
				{
					supported = true;
					supportY = block.top + PLAYER_FOOT_OFFSET;
				}
			}
		}
		
		for (int i = 0; i < (int)floors.size(); ++i)
		{
			const CollisionRect r = MakeFloorRect(floors[i]);
			if (OverlapX(before, r) &&
				std::fabs(before.bottom - r.top) <= CONTACT_EPSILON)
			{
				supported = true;
				supportY = r.top + PLAYER_FOOT_OFFSET;
				ridingFloor_ = i;
			}
		}

		if (supported)
		{
			transform_.position_.y = supportY;
			jumpVelocity_ = 0.0f;
			return;
		}
		isGrounded_ = false;
		jumpVelocity_ = 0.0f;
	}

	const float dy = jumpVelocity_;
	transform_.position_.y += dy;
	jumpVelocity_ -= GRAVITY;
	const CollisionRect after = MakePlayerRect(transform_.position_);
	float resolvedY = transform_.position_.y;
	bool hit = false;

	// 移動前後で面を跨いだかを調べ、最初に接触する面で止める。
	for (int row = 0; row < mapHeight; ++row)
	{
		for (int col = 0; col < static_cast<int>(gmap[row].size()); ++col)
		{
			if (gmap[row][col] != 1) continue;
			const CollisionRect block = MakeBlockRect(row, col, mapHeight);
			if (!OverlapX(after, block)) continue;

			if (dy <= 0.0f && before.bottom >= block.top - CONTACT_EPSILON &&
				after.bottom <= block.top)
			{
				const float y = block.top + PLAYER_FOOT_OFFSET;
				if (!hit || y > resolvedY) resolvedY = y;
				hit = true;
			}
			else if (dy > 0.0f && before.top <= block.bottom + CONTACT_EPSILON &&
				after.top >= block.bottom)
			{
				const float y = block.bottom - PLAYER_HEIGHT + PLAYER_FOOT_OFFSET;
				if (!hit || y < resolvedY) resolvedY = y;
				hit = true;
			}
		}
	}

	for (int i = 0; i < (int)floors.size(); ++i)
	{
		const CollisionRect r = MakeFloorRect(floors[i]);
		if (!OverlapX(after, r)) continue;

		if (dy <= 0.0f &&
			before.bottom >= r.top - floors[i].dy - CONTACT_EPSILON &&
			after.bottom <= r.top)
		{
			// 上から着地
			const float y = r.top + PLAYER_FOOT_OFFSET;
			if (!hit || y > resolvedY) { resolvedY = y; ridingFloor_ = i; }
			hit = true;
		}
		else if (dy > 0.0f &&
			before.top <= r.bottom + CONTACT_EPSILON &&
			after.top >= r.bottom)
		{
			// 下から頭をぶつける
			const float y = r.bottom - PLAYER_HEIGHT + PLAYER_FOOT_OFFSET;
			if (!hit || y < resolvedY) resolvedY = y;
			hit = true;
		}
	}

	// 常設床も着地候補に含める。
	if (dy <= 0.0f && resolvedY <= START_POS.y)
	{
		resolvedY = START_POS.y;
		hit = true;
	}
	transform_.position_.y = resolvedY;
	if (hit)
	{
		jumpVelocity_ = 0.0f;
		// 頭突きでは接地させない。次の更新から重力で落下する。
		isGrounded_ = dy <= 0.0f;
	}
}

void Player::ResolveWallCollision(XMVECTOR& pos, const XMVECTOR& move)
{
	if (ground_ == nullptr)
		return;

	// Update() で適用した水平移動から、移動前の矩形を復元する。
	XMFLOAT3 oldPosition;
	XMStoreFloat3(&oldPosition, pos - currentSpeed_ * move);
	const float dx = transform_.position_.x - oldPosition.x;
	if (dx == 0.0f) return;

	const CollisionRect before = MakePlayerRect(oldPosition);
	const CollisionRect after = MakePlayerRect(transform_.position_);
	const auto& gmap = ground_->GetMapData();
	const int mapHeight = static_cast<int>(gmap.size());
	float resolvedX = transform_.position_.x;
	bool hit = false;

	for (int row = 0; row < mapHeight; ++row)
	{
		for (int col = 0; col < static_cast<int>(gmap[row].size()); ++col)
		{
			if (gmap[row][col] != 1) continue;
			const CollisionRect block = MakeBlockRect(row, col, mapHeight);
			if (!OverlapY(before, block)) continue;

			if (dx > 0.0f && before.right <= block.left + CONTACT_EPSILON &&
				after.right >= block.left)
			{
				const float x = block.left - PLAYER_HALF_WIDTH;
				if (!hit || x < resolvedX) resolvedX = x;
				hit = true;
			}
			else if (dx < 0.0f && before.left >= block.right - CONTACT_EPSILON &&
				after.left <= block.right)
			{
				const float x = block.right + PLAYER_HALF_WIDTH;
				if (!hit || x > resolvedX) resolvedX = x;
				hit = true;
			}
		}
	}

	const auto& floors = ground_->GetMovingFloors();
	for (int i = 0; i < (int)floors.size(); ++i)
	{
		const CollisionRect r = MakeFloorRect(floors[i]);
		if (!OverlapY(before, r)) continue;

		if (dx > 0.0f && before.right <= r.left + CONTACT_EPSILON &&
			after.right >= r.left)
		{
			const float x = r.left - PLAYER_HALF_WIDTH;
			if (!hit || x < resolvedX) resolvedX = x;
			hit = true;
		}
		else if (dx < 0.0f && before.left >= r.right - CONTACT_EPSILON &&
			after.left <= r.right)
		{
			const float x = r.right + PLAYER_HALF_WIDTH;
			if (!hit || x > resolvedX) resolvedX = x;
			hit = true;
		}
	}

	if (hit)
	{
		transform_.position_.x = resolvedX;
		pos = XMLoadFloat3(&transform_.position_);
		currentSpeed_ = 0.0f;
	}
}

void Player::Draw()
{
	// 描画用のコピーだけを縮小する。位置と矩形判定には倍率を重ねない。
	// Idle.fbx も Walking.fbx と同じ元サイズを前提とする。
	Transform drawTransform = transform_;
	drawTransform.scale_.x *= PLAYER_MODEL_SCALE;
	drawTransform.scale_.y *= PLAYER_MODEL_SCALE;
	drawTransform.scale_.z *= PLAYER_MODEL_SCALE;
	drawTransform.position_.y += 0.3f;

	// 待機中
	if (pstate_ == PLAYER_IDLE)
	{
		Model::SetTransform(hIdleModel_, drawTransform);
		Model::Draw(hIdleModel_);
	}
	// 歩行中・方向転換中
	else if (pstate_ == PLAYER_WALK || pstate_ == PLAYER_TURN)
	{
		Model::SetTransform(hWalkModel_, drawTransform);
		Model::Draw(hWalkModel_);
	}
}

void Player::Release() {}

void Player::OnCollision(GameObject* pTarget) {}