#include "Ground.h"
#include "Engine/Model.h"
#include "Engine/CsvReader.h"

#include "Player.h"
#include <cmath>

namespace
{
	using std::vector;
	const float GROUND_WIDTH = 20.0f;
	const float GROUND_Y = 10.0f;
	const float GROUND_Z = 1.0f;
	const float GROUND_ROTATE_X = -90.0f;
	const float BLOCK_INTERVAL_X = 2.0f;
	const float BLOCK_INTERVAL_Y = 1.0f;
}

Ground::Ground(GameObject* parent)
	:GameObject(parent, "Ground"), hModel_(-1), mapWidth_(-1), mapHeight_(-1)
{
	CsvReader csvData;
	csvData.Load("map.csv"); //CSVファイルを読み込む
	mapWidth_ = csvData.GetWidth(); //列数を取得
	mapHeight_ = csvData.GetHeight() / 2; //行数を取得
	mapData_ = vector<vector<int>>(mapHeight_, vector<int>(mapWidth_, 0));//mapData_を初期化 mapHeight_個のvector<int>の配列を作る
	for (int x = 0; x < mapWidth_; x++)
	{
		for (int y = 0; y < mapHeight_; y++)
		{
			mapData_[y][x] = csvData.GetValue(x, y);//CSVの値をmapData_に格納
		}
	}

	for (int y = 0; y < mapHeight_; y++) {
		for (int x = 0; x < mapWidth_; x++) {
			if (mapData_[y][x] == 3) {
				AddMovingFloor(x * BLOCK_INTERVAL_X, (mapHeight_ - 1 - y) * BLOCK_INTERVAL_Y,
					4.0f, 0.0f, 0.02f);	// 横に±4往復
			}
		}
	}

	for (int y = 0; y < mapHeight_; y++)
	{
		for (int x = 0; x < mapWidth_; x++)
		{
			float px = x * BLOCK_INTERVAL_X;
			float py = (mapHeight_ - 1 - y) * BLOCK_INTERVAL_Y;

			if (mapData_[y][x] == 3)
			{
				AddMovingFloor(px, py, 4.0f, 0.0f, 0.02f);	// 横に±4往復
			}
			else if (mapData_[y][x] == 4)
			{
				AddMovingFloor(px, py, 0.0f, 2.0f, 0.02f);	// 縦に±2往復
			}
		}
	}
}

void Ground::Initialize()
{
	hModel_ = Model::Load("Map.fbx");
	hModelt_ = Model::Load("Block.fbx");
}

void Ground::Update()
{
	Player* player = dynamic_cast<Player*>(FindObject("Player"));
	for (int i = 0; i < (int)movingFloors_.size(); i++)
	{
		MovingFloor& f = movingFloors_[i];
		f.t += f.speed;
		f.dx = f.cx + f.ax * sinf(f.t) - f.x;
		f.dy = f.cy + f.ay * sinf(f.t) - f.y;
		f.x += f.dx;
		f.y += f.dy;
		if (player && player->GetRidingFloor() == i) player->Carry(f.dx, f.dy);
	}
}

void Ground::Draw()
{
	for (int i = 0;i < 3; i++) {
		transform_.position_ = { GROUND_WIDTH / 2.0f + GROUND_WIDTH * i, GROUND_Y, GROUND_Z };
		transform_.rotate_ = { GROUND_ROTATE_X, 0.0f, 0.0f };
		Model::SetTransform(hModel_, transform_);
		Model::Draw(hModel_);
	}

	for (int j = 0;j < mapHeight_;j++) {
		for (int i = 0;i < mapWidth_;i++) {
			if (mapData_[j][i] == 1) {
				Transform tr;
				tr.position_ = { i * BLOCK_INTERVAL_X, (mapHeight_ - 1 - j) * BLOCK_INTERVAL_Y, 0.0f };
				Model::SetTransform(hModelt_, tr);
				Model::Draw(hModelt_);
			}
		}
	}

	for (const MovingFloor& f : movingFloors_){
		Transform tr;
		tr.position_ = { f.x, f.y, 0.0f };
		Model::SetTransform(hModelt_, tr);
		Model::Draw(hModelt_);
	}
}

void Ground::Release(){}