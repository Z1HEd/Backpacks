#define DEBUG_CONSOLE // Uncomment this if you want a debug console to start. You can use the Console class to print. You can use Console::inStrings to get input.

#include <4dm.h>
#include "ItemBackpack.h"
// USED FOR HACKING PLAYER::THROWITEM
//REMOVE BEFORE RELEASING
#include <glm/gtc/random.hpp>
using namespace fdm;

// Initialize the DLLMain
initDLL

std::vector<std::string> toolNames{
	"Backpack",
	"Reinforced Backpack",
	"Deadly Backpack"
};

std::vector<std::string> materialNames{
	"Hyperfabric",
	"Reinforced Hyperfabric",
	"Deadly Hyperfabric"
};

// Item slot material
$hook(void, ItemMaterial, render, const glm::ivec2& pos)
{
	int index = std::find(materialNames.begin(), materialNames.end(), self->name) - materialNames.begin();
	if (index == materialNames.size())
		return original(self, pos);

	TexRenderer& tr = *ItemTool::tr; // or TexRenderer& tr = ItemTool::tr; after 0.3
	const Tex2D* ogTex = tr.texture; // remember the original texture

	tr.texture = ResourceManager::get("assets/Materials.png", true); // set to custom texture
	tr.setClip(index * 36, 0, 36, 36);
	tr.setPos(pos.x, pos.y, 70, 72);
	tr.render();

	tr.texture = ogTex; // return to the original texture
}

//Deadly text effect
$hook(bool, ItemTool, isDeadly)
{
	if (self->name == "Deadly Bakcpack")
		return true;
	return original(self);
}
$hook(bool, ItemMaterial, isDeadly)
{
	if (self->name == "Deadly Hyperfabric")
		return true;
	return original(self);
}

// add recipes
$hookStatic(void, CraftingMenu, loadRecipes)
{
	static bool recipesLoaded = false;

	if (recipesLoaded) return;

	recipesLoaded = true;

	original();

	CraftingMenu::recipes->push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Hypersilk"}, {"count", 2}},{{"name", "Stick"}, {"count", 1}}}},
		{"result", {{"name", "Hyperfabric"}, {"count", 1}}}
		}
	);

	CraftingMenu::recipes->push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Hyperfabric"}, {"count", 2}},{{"name", "Iron Bars"}, {"count", 2}}}},
		{"result", {{"name", "Reinforced Hyperfabric"}, {"count", 1}}}
		}
	);

	CraftingMenu::recipes->push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Reinforced Hyperfabric"}, {"count", 2}},{{"name", "Deadly Bars"}, {"count", 2}}}},
		{"result", {{"name", "Deadly Hyperfabric"}, {"count", 1}}}
		}
	);

	CraftingMenu::recipes->push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Hyperfabric"}, {"count", 3}}}},
		{"result", {{"name", "Backpack"}, {"count", 1}}}
		}
	);

	CraftingMenu::recipes->push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Reinforced Hyperfabric"}, {"count", 2}}}},
		{"result", {{"name", "Reinforced Backpack"}, {"count", 1}}}
		}
	);

	CraftingMenu::recipes->push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Deadly Hyperfabric"}, {"count", 2}}}},
		{"result", {{"name", "Deadly Backpack"}, {"count", 1}}}
		}
	);
}



$hookStatic(std::unique_ptr<Entity>, EntityItem, createWithItem, const std::unique_ptr<Item>& item, const glm::vec4& pos, const glm::vec4& vel) {
	Console::printLine("EntityItem::createWithItem: ",item.get());
	return original(item, pos, vel);
}

// Reinvent dropping items
$hook(void, Player, throwItem, World* world, std::unique_ptr<Item>& item, uint32_t maxCount) {
	if (item == nullptr) return;

	if (!dynamic_cast<ItemBackpack*>(item.get())) return original(self, world, item, maxCount);

	
	glm::vec3 randomVector = glm::ballRand(0.5f);
	glm::vec4 velocity = {
		self->forward.x * 10 + randomVector.x,
		self->forward.y * 10 + randomVector.y,
		self->forward.z * 10 + randomVector.z,
		self->forward.w * 10 + randomVector.z // idc
	};
	std::unique_ptr<Entity> backpackEntity = EntityItem::createWithItem(item, self->cameraPos, velocity);
	Chunk* chunk = world->getChunk(self->pos*0.125f);
	world->addEntityToChunk(backpackEntity, chunk);
}

// Prevent player from doing bad stuff
$hook(bool, InventoryManager, applyTransfer, InventoryManager::TransferAction action, std::unique_ptr<Item>& selectedSlot, std::unique_ptr<Item>& cursorSlot, Inventory* other){

	InventoryManager& actualInventoryManager = StateGame::instanceObj->player.inventoryManager; // self is bullshit, when taking stuff its nullptr lol

	// How the fuck does this even work
	if (
		(// dont put backpacks into other backpacks
			cursorSlot && 
			actualInventoryManager.secondary != nullptr &&
			actualInventoryManager.secondary->name == "backpackInventory" &&
			actualInventoryManager.secondary != other &&
			dynamic_cast<ItemBackpack*>(cursorSlot.get()) != nullptr
			) ||
		(// dont take an item that is an open backpack
			actualInventoryManager.secondary != nullptr && 
			selectedSlot &&
			dynamic_cast<ItemBackpack*>(selectedSlot.get()) &&
			dynamic_cast<ItemBackpack*>(selectedSlot.get())->inventory.get() == actualInventoryManager.secondary
			)
		)
		return true;


	return original(self, action, selectedSlot, cursorSlot, other);
}

// Add item blueprints, load shaders
void initItemNAME()
{
	for (int i = 0;i < materialNames.size(); i++)
		(*Item::blueprints)[materialNames[i]] =
		{
			{ "type", "material" },
			{ "baseAttributes", nlohmann::json::object() } // no attributes
		};

	for (int i = 0;i < toolNames.size(); i++)
		(*Item::blueprints)[toolNames[i]] =
		{
			{ "type", "backpack" },
			{ "baseAttributes", { { "type", i }, { "inventory", nlohmann::json::array() } } }
		};
}

$hook(void, StateIntro, init, StateManager& s)
{
	original(self, s);

	// initialize opengl stuff
	glewExperimental = true;
	glewInit();
	glfwInit();

	initItemNAME();

	ShaderManager::load("backpackShader", "../../assets/shaders/tetNormal.vs", "assets/backpack.fs", "../../assets/shaders/tetNormal.gs");
}