//#define DEBUG_CONSOLE // Uncomment this if you want a debug console to start. You can use the Console class to print. You can use Console::inStrings to get input.

#include <4dm.h>
#include "ItemBackpack.h"
using namespace fdm;

// Initialize the DLLMain
initDLL

std::vector<nlohmann::json> recipes = {};

std::vector<std::string> toolNames{
	"Backpack",
	"Reinforced Backpack",
	"Deadly Backpack",
	"Solenoid Backpack",
	"Wormhole Backpack"
};
std::vector<std::string> materialNames{
	"Cosmic Bars",
	"Hyperfabric",
	"Reinforced Hyperfabric",
	"Deadly Hyperfabric",
	"Solenoid Hyperfabric",
	"Cosmic Hyperfabric"
};

// Dont deselect when open
$hook(void, WorldSingleplayer, localPlayerEvent, Player* player, Packet::ClientPacket eventType, int64_t eventValue, void* data) {
	if (eventType != Packet::C_HOTBAR_SLOT_SELECT) return original(self, player, eventType, eventValue, data);
	if (!player->inventoryManager.isOpen()) return original(self, player, eventType, eventValue, data);
	if (!dynamic_cast<ItemBackpack*>(player->hotbar.getSlot(player->hotbar.selectedIndex).get())) return original(self, player, eventType, eventValue, data);
	if (player->inventoryManager.secondary != &((ItemBackpack*)player->hotbar.getSlot(player->hotbar.selectedIndex).get())->inventory) return original(self, player, eventType, eventValue, data);
}

$hook(void, WorldClient, localPlayerEvent, Player* player, Packet::ClientPacket eventType, int64_t eventValue, void* data) {
	if (eventType != Packet::C_HOTBAR_SLOT_SELECT) return original(self, player, eventType, eventValue, data);
	if (!player->inventoryManager.isOpen()) return original(self, player, eventType, eventValue, data);
	if (!dynamic_cast<ItemBackpack*>(player->hotbar.getSlot(player->hotbar.selectedIndex).get())) return original(self, player, eventType, eventValue, data);
	if (player->inventoryManager.secondary != &((ItemBackpack*)player->hotbar.getSlot(player->hotbar.selectedIndex).get())->inventory) return original(self, player, eventType, eventValue, data);
}

//Deadly text effect for materials
$hook(bool, ItemMaterial, isDeadly)
{
	if (self->getName() == "Deadly Hyperfabric" || self->getName() == "Composite Hyperfabric") return true;

	return original(self);
}

// Render materials icons
$hook(void, ItemMaterial, render, const glm::ivec2& pos)
{
	auto index = std::find(materialNames.begin(), materialNames.end(), self->getName()) - materialNames.begin();

	if (index == materialNames.size())
		return original(self, pos);

	TexRenderer& tr = ItemTool::tr;
	FontRenderer& fr = ItemMaterial::fr;

	const Tex2D* ogTex = tr.texture; // remember the original texture

	static std::string iconPath = "";
	iconPath = std::format("{}{}.png", "assets/", self->getName().c_str());
	iconPath.erase(remove(iconPath.begin(), iconPath.end(), ' '), iconPath.end());

	tr.texture = ResourceManager::get(iconPath, true); // set to custom texture
	tr.setClip(0, 0, 36, 36);
	tr.setPos(pos.x, pos.y, 70, 72);
	tr.render();
	tr.texture = ogTex; // return to the original texture
}

// Backpack quick access
bool handleBackpackAccess(Player& player, int mouseX, int mouseY) {
	static std::unique_ptr<Item>* itemFromBackpack = nullptr;
	static int index = -1;
	enum BackpackAccessMode {
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
		manager.applyTransfer(InventoryManager::ACTION_MOVE, *itemInSlot,manager.cursor.item,&backpack->getUsedInventory());
		manager.craftingMenu.updateAvailableRecipes();
		manager.updateCraftingMenuBox();
	}
	return true;
}
$hook(void, Player, mouseInput, GLFWwindow* window, World* world, double xpos, double ypos) {
	if (!handleBackpackAccess(*self, xpos, ypos)) return original(self, window, world, xpos, ypos);
}
$hook(bool, InventoryManager, mouseButtonInput, uint32_t x, uint32_t y, uint32_t button, int action, int mods) {
	if (!handleBackpackAccess(StateGame::instanceObj.player, x, y)) return original(self, x, y, button, action, mods);
	return true;
}


// Prevent player from doing bad stuff
$hook(bool, InventoryManager, applyTransfer, InventoryManager::TransferAction action, std::unique_ptr<Item>& selectedSlot, std::unique_ptr<Item>& cursorSlot, Inventory* other) {

	InventoryManager& actualInventoryManager = StateGame::instanceObj.player.inventoryManager; // self is bullshit, when taking stuff its nullptr lol

	// How the fuck does this even work
	if (
		(// Dont put backpacks into other backpacks
			(
				( // Mouse swapping
					actualInventoryManager.secondary != nullptr &&
					actualInventoryManager.secondary->name == "backpackInventory" &&

					actualInventoryManager.secondary != other &&
					cursorSlot &&
					dynamic_cast<ItemBackpack*>(cursorSlot.get())) ||
				( // Moving

					action == InventoryManager::ACTION_MOVE &&
					selectedSlot &&
					other->name == "backpackInventory" &&
					dynamic_cast<ItemBackpack*>(selectedSlot.get())
					)
				)
			) ||
		(// Dont take an item that is an open backpack
			actualInventoryManager.secondary != nullptr &&
			selectedSlot &&
			dynamic_cast<ItemBackpack*>(selectedSlot.get()) &&
			(&dynamic_cast<ItemBackpack*>(selectedSlot.get())->inventory == actualInventoryManager.secondary ||
				(
					dynamic_cast<ItemBackpack*>(selectedSlot.get())->type == ItemBackpack::WORMHOLE &&
					actualInventoryManager.secondary->name == "wormholeInventory"
					)
				)
			)
		)
		return true;


	return original(self, action, selectedSlot, cursorSlot, other);
}

//Init stuff
$hookStatic(void, CraftingMenu, loadRecipes)
{
	static bool recipesLoaded = false;

	if (recipesLoaded) return;

	recipesLoaded = true;

	original();

	if (recipes.empty()) return;

	for (const auto& recipe : recipes) {
		if (std::any_of(CraftingMenu::recipes.begin(),
			CraftingMenu::recipes.end(),
			[&recipe](const nlohmann::json& globalRecipe) {
				return globalRecipe == recipe;
			})) continue;
		CraftingMenu::recipes.push_back(recipe);
	}
}
void addRecipe(const std::string& resultName, int resultCount,
	const std::vector<std::pair<std::string, int>>& components) {

	nlohmann::json recipeJson;
	recipeJson["result"] = { {"name", resultName}, {"count", resultCount} };

	nlohmann::json recipeComponents = nlohmann::json::array();
	for (const auto& [name, count] : components) {
		recipeComponents.push_back({ {"name", name}, {"count", count} });
	}

	recipeJson["recipe"] = recipeComponents;
	recipes.push_back(recipeJson);
}
void InitRecipes() {
	addRecipe("Cosmic Bars", 1, { {"Deadly Bars",2},{"Solenoid Bars",2} ,{"Midnight Wood",1} });

	addRecipe("Hyperfabric", 1, { {"Hypersilk",2},{"Stick",1} }); // 2 silk
	addRecipe("Reinforced Hyperfabric", 1, { {"Hyperfabric",2},{"Iron Bars",2} }); // 4 silk, 2 iron
	addRecipe("Deadly Hyperfabric", 1, { {"Reinforced Hyperfabric",2},{"Deadly Bars",2} }); // 8 silk, 4 iron, 2 deadly
	addRecipe("Solenoid Hyperfabric", 1, { {"Deadly Hyperfabric",2},{"Solenoid Bars",2} }); // 16 silk, 8 iron, 4 deadly, 2 solenoid
	addRecipe("Cosmic Hyperfabric", 1, { {"Solenoid Hyperfabric",1},{"Cosmic Bars",2} }); // 16 silk, 8 iron, 8 deadly, 6 solenoid

	addRecipe("Backpack", 1, { {"Hyperfabric",3} }); // 6 silk
	addRecipe("Reinforced Backpack", 1, { {"Reinforced Hyperfabric",2} }); // 8 silk, 4 iron
	addRecipe("Deadly Backpack", 1, { {"Deadly Hyperfabric",2} }); // 16 silk, 8 iron, 4 deadly
	addRecipe("Solenoid Backpack", 1, { {"Solenoid Hyperfabric",2} }); // 32 silk, 16 iron, 8 deadly, 4 solenoid
	addRecipe("Wormhole Backpack", 1, { {"Cosmic Hyperfabric",2} }); // 32 silk, 16 iron, 16 deadly, 12 solenoid
}
void InitBlueprints() {
	// Materials
	for (int i = 0;i < materialNames.size(); i++)
		(Item::blueprints)[materialNames[i]] =
	{
		{ "type", "material"},
		{ "baseAttributes", nlohmann::json::object()}
	};

	// Backpacks
	for (int i = 0;i < toolNames.size(); i++)
		Item::blueprints[toolNames[i]] =
	{
		{ "type", "backpack" },
		{ "baseAttributes", { { "type", i }, { "inventory", nlohmann::json::array() } } }
	};
}
void InitSounds() {
	ItemBackpack::openSound = std::format("../../{}/assets/backpackOpen.ogg", fdm::getModPath(fdm::modID));

	if (!AudioManager::loadSound(ItemBackpack::openSound)) Console::printLine("Cannot load sound: ", ItemBackpack::openSound);
}
void InitShaders() {

}
$hook(void, StateIntro, init, StateManager& s)
{
	original(self, s);

	//Initialize opengl stuff
	glewExperimental = true;
	glewInit();
	glfwInit();

	ItemBackpack::rendererInit();

	InitBlueprints();

	InitRecipes();

	InitSounds();

	InitShaders();
}