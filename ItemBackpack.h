#pragma once

#include "4dm.h"

using namespace fdm;

class ItemBackpack : public Item {
public:
	enum BackpackType {
		FABRIC,
		IRON,
		DEADLY,
		SOLENOID,
		WORMHOLE
	};
	inline static constexpr glm::ivec2 sizes[5]
	{
		{ 1, 8 },
		{ 2, 8 },
		{ 4, 8 },
		{ 6, 8 },
		{ 4, 8 }
	};

	BackpackType type = FABRIC;
	InventorySession openInstance;
	InventoryGrid inventory;
	bool isSolenoidEffectActive = false;

	InventoryGrid& getUsedInventory();

	static MeshRenderer renderer;
	static std::string openSound;

	std::unique_ptr<Item>* getFirstItem();
	std::unique_ptr<Item>* getLastItem();
	std::unique_ptr<Item> clone() override;
	static void rendererInit();
	stl::string getName() override;
	void render(const glm::ivec2& pos) override;
	void renderEntity(const m4::Mat5& MV, bool inHand, const glm::vec4& lightDir) override;
	bool isDeadly() override;
	uint32_t getStackLimit() override;
	nlohmann::json saveAttributes() override;
	bool isCompatible(const std::unique_ptr<Item>& other) override;
	
};