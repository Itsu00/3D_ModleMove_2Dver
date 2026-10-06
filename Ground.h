#pragma once
#include "Engine/GameObject.h"
#include <vector>

struct MovingFloor
{
	float cx, cy;	// 往復の中心
	float ax, ay;	// 中心からの振れ幅
	float speed;	// 位相の進み（ラジアン/フレーム）
	float t;		// 現在の位相
	float x, y;		// 現在位置
	float dx, dy;	// 直近の移動量
};

class Ground : public GameObject
{
public:
	Ground(GameObject* parent);
	void Initialize() override;//初期化
	const std::vector<std::vector<int>> &GetMapData() const { return mapData_; }
	void Update() override;//更新
	void Draw() override;//描画
	void Release() override;//開放
	
	void AddMovingFloor(float x, float y, float ax, float ay, float speed)
	{
		movingFloors_.push_back({ x, y, ax, ay, speed, 0.0f, x, y, 0.0f, 0.0f });
	}
	const std::vector<MovingFloor>& GetMovingFloors() const { return movingFloors_; }
private:
	int hModel_;
	int hModelt_;

	std::vector<std::vector<int>> mapData_;

	int mapWidth_;
	int mapHeight_;

	std::vector<MovingFloor> movingFloors_;
};