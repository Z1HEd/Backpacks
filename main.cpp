//#define DEBUG_CONSOLE // Uncomment this if you want a debug console to start. You can use the Console class to print. You can use Console::inStrings to get input.

#include <4dm.h>
#include "ItemBackpack.h"
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

	TexRenderer& tr = ItemTool::tr; // or TexRenderer& tr = ItemTool::tr; after 0.3
	const Tex2D* ogTex = tr.texture; // remember the original texture

	tr.texture = ResourceManager::get("assets/Materials.png", true); // set to custom texture
	tr.setClip(index * 36, 0, 36, 36);
	tr.setPos(pos.x, pos.y, 70, 72);
	tr.render();

	tr.texture = ogTex; // return to the original texture
	
}

// Backpack quick access
bool handleBackpackAccess(Player& player, int mouseX,int mouseY) {
	static std::unique_ptr<Item>* itemFromBackpack = nullptr;
	static int index = -1;
	static enum BackpackAccessMode {
		Storing,
		Accessing,
		None
	};
	static BackpackAccessMode accessMode = None;
	static ItemBackpack* backpack = nullptr;
	static std::unique_ptr<Item>* itemInSlot = nullptr;


	InventoryManager& manager = player.inventoryManager;

	if (!manager.isOpen()) return false; // Inventory is closed

	backpack = dynamic_cast<ItemBackpack*>(manager.cursor.item.get());

	if (backpack == nullptr) return false; // Not holding a backpack

	if (!player.keys.rightMouseDown) { // Not pressing RMB
		accessMode = None;
		return false;
	}

	// Find targeted slot
	index = manager.primary->getSlotIndex({ mouseX,mouseY });
	itemInSlot = nullptr;

	if (index != -1)
		itemInSlot = &manager.primary->getSlot(index);
	else {
		index = manager.secondary->getSlotIndex({ mouseX,mouseY });
		if (index != -1)
			itemInSlot = &manager.secondary->getSlot(index);
	}

	if (!itemInSlot) return true; // Didnt even target a slot

	if (accessMode == None && itemInSlot->get() == nullptr) { // Slot is empty, get to accesing backpack
		accessMode = Accessing;
	}
	else if (accessMode == None) { // Slot is not empty, get to storing stuff in a backpack 
		accessMode = Storing;
	}

	if (itemInSlot->get() == nullptr && accessMode == Accessing) { // Slot is empty, put stuff from backpack into it
		itemFromBackpack = backpack->getLastItem();
		if (itemFromBackpack == nullptr) return true;
		// Lol i could do that directly
		manager.applyTransfer(InventoryManager::ACTION_SWAP, *itemFromBackpack, *itemInSlot, manager.secondary);
		manager.craftingMenu.updateAvailableRecipes();
		manager.updateCraftingMenuBox();
	}
	else if (accessMode == Storing) { // Slot is not empty, put stuff from it into backpack 
		backpack->inventory.addItem(*itemInSlot);
		manager.craftingMenu.updateAvailableRecipes();
		manager.updateCraftingMenuBox();
	}
	return true;
}
$hook(void, Player, mouseInput, GLFWwindow* window, World* world, double xpos, double ypos) {
	if (!handleBackpackAccess(*self, xpos, ypos)) return original(self, window, world, xpos, ypos);
}
$hook(bool, InventoryManager, mouseButtonInput, uint32_t x, uint32_t y, uint32_t button, int action, int mods) {
	if (!handleBackpackAccess(StateGame::instanceObj.player, x, y)) return original(self, x,y,button,action,mods);
	return true;
}

//Deadly text effect
$hook(bool, ItemMaterial, isDeadly)
{
	if (self->name == "Deadly Hyperfabric")
		return true;
	return original(self);
}

// A dd recipes
$hookStatic(void, CraftingMenu, loadRecipes)
{
	static bool recipesLoaded = false;

	if (recipesLoaded) return;

	recipesLoaded = true;

	original();

	CraftingMenu::recipes.push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Hypersilk"}, {"count", 2}},{{"name", "Stick"}, {"count", 1}}}},
		{"result", {{"name", "Hyperfabric"}, {"count", 1}}}
		}
	);

	CraftingMenu::recipes.push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Hyperfabric"}, {"count", 2}},{{"name", "Iron Bars"}, {"count", 2}}}},
		{"result", {{"name", "Reinforced Hyperfabric"}, {"count", 1}}}
		}
	);

	CraftingMenu::recipes.push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Reinforced Hyperfabric"}, {"count", 2}},{{"name", "Deadly Bars"}, {"count", 2}}}},
		{"result", {{"name", "Deadly Hyperfabric"}, {"count", 1}}}
		}
	);

	CraftingMenu::recipes.push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Hyperfabric"}, {"count", 3}}}},
		{"result", {{"name", "Backpack"}, {"count", 1}}}
		}
	);

	CraftingMenu::recipes.push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Reinforced Hyperfabric"}, {"count", 2}}}},
		{"result", {{"name", "Reinforced Backpack"}, {"count", 1}}}
		}
	);

	CraftingMenu::recipes.push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Deadly Hyperfabric"}, {"count", 2}}}},
		{"result", {{"name", "Deadly Backpack"}, {"count", 1}}}
		}
	);
}

// Prevent player from doing bad stuff
$hook(bool, InventoryManager, applyTransfer, InventoryManager::TransferAction action, std::unique_ptr<Item>& selectedSlot, std::unique_ptr<Item>& cursorSlot, Inventory* other){

	InventoryManager& actualInventoryManager = StateGame::instanceObj.player.inventoryManager; // self is bullshit, when taking stuff its nullptr lol

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
			&dynamic_cast<ItemBackpack*>(selectedSlot.get())->inventory == actualInventoryManager.secondary
			)
		)
		return true;


	return original(self, action, selectedSlot, cursorSlot, other);
}

// Initialize stuff
void initItemNAME()
{
	for (int i = 0;i < materialNames.size(); i++)
		Item::blueprints[materialNames[i]] =
		{
			{ "type", "material" },
			{ "baseAttributes", nlohmann::json::object() } // no attributes
		};

	for (int i = 0;i < toolNames.size(); i++)
		Item::blueprints[toolNames[i]] =
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

	ItemBackpack::openSound = std::format("../../{}/assets/backpackOpen.ogg", fdm::getModPath(fdm::modID));

	AudioManager::loadSound(ItemBackpack::openSound);

	ItemBackpack::rendererInit();
}