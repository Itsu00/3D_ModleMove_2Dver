#pragma once
#include "Engine/GameObject.h"
#include "Engine/Model.h"

class Player;
class Text;

class TestScene : public GameObject
{
public:
	TestScene(GameObject* parent);
	void Initialize() override;//‰Šú‰»
	void Update() override;//XV
	void Draw() override;//•`‰æ
	void Release() override;//ŠJ•ú
	void AddScore(int score) { myScore += score; }
private:
	Player* pPlayer_;
	Text* pText_;
	int myScore;
};