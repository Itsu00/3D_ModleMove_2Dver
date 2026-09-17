#pragma once
#include "Engine/GameObject.h"

enum ItemType
{
    ITEMTYPE_NORMAL,
    ITEMTYPE_POWER,
    ITEMTYPE_MAX
};

class Item :
    public GameObject
{
public:
    Item(GameObject* parent);
    ~Item();
    void Initialize() override;
    void Update() override;
    void Draw() override;
    void Release() override;
    void SetItemType(ItemType type);
    void OnCollision(GameObject* pTarget) override;
    int GetScore() { return score_; }
private:
    ItemType type_;
    int hModel_;
    int score_;
};