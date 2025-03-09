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

	BackpackType type = FABRIC;
	InventoryGrid inventory;
	static MeshRenderer renderer;
	static std::string openSound;

	std::unique_ptr<Item> clone() override;
	static void rendererInit();
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