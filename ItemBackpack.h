#pragma once

#include "4dm.h"

using namespace fdm;

class ItemBackpack : public Item {
public:
	enum BackpackType {
		FABRIC,
		IRON,
		DEADLY
	};
	glm::ivec2 sizes[3] = {
		{1,8},
		{2,8},
		{4,8},
	};

	bool isOpen = false;

	BackpackType type = FABRIC;
	std::unique_ptr<InventoryGrid> inventory;

	std::unique_ptr<Item> clone() override;

	stl::string getName() override;
	bool action(World* world, Player* player, int action) override;
	void render(const glm::ivec2& pos) override;
	void renderEntity(const m4::Mat5& MV, bool inHand, const glm::vec4& lightDir) override;
	bool isDeadly() override;
	uint32_t getStackLimit() override;
	nlohmann::json saveAttributes() override;
private:
	InventorySession openInstance;
};