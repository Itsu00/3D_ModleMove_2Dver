#pragma once
#include "Engine/GameObject.h"
#include <vector>

class Ground : public GameObject
{
public:
	Ground(GameObject* parent);
	void Initialize() override;//初期化
	const std::vector<std::vector<int>> &GetMapData() const { return mapData_; }
	void Update() override;//更新
	void Draw() override;//描画
	void Release() override;//開放
	
private:
	int hModel_;
	int hModelt_;

	std::vector<std::vector<int>> mapData_;

	int mapWidth_;
	int mapHeight_;
};