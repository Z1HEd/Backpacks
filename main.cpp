//#define DEBUG_CONSOLE // Uncomment this if you want a debug console to start. You can use the Console class to print. You can use Console::inStrings to get input.

#include <4dm.h>
#include "ItemBackpack.h"
#include "utils.h"
using namespace fdm;

// Initialize the DLLMain
initDLL
std::vector<std::string> backpackNames{
	"Backpack",
	"Reinforced Backpack",
	"Deadly Backpack",
	"Solenoid Backpack",
	"Wormhole Backpack"
};
std::vector<std::string> toolNames{
	"Spindle And Distaff",
	"Weaving Loom"
};
std::vector<std::string> materialNames{
	"Cosmic Bars",
	"Hyperfabric",
	"Reinforced Hyperfabric",
	"Deadly Hyperfabric",
	"Solenoid Hyperfabric",
	"Cosmic Hyperfabric"
};
// Weaving and Spinning
bool isWeaving = false;
bool isSpinning = false;

$hook(void, Player, updateLocal, World* world, double dt, GLFWwindow* window) {
	if (self->inventoryManager.isOpen()) return original(self, world, dt, window);
	
	isWeaving = false;
	isSpinning = false;
	self->inventoryManager.craftingText.setText("Crafting:");
	if (self->keys.rightMouseDown && self->hotbar.getSlot(self->hotbar.selectedIndex) && self->hotbar.getSlot(self->hotbar.selectedIndex)->getName() == "Weaving Loom")
	{
		isWeaving = true;
		self->inventoryManager.craftingText.setText("Weaving:");
		utils::openInventory(StateGame::instanceObj.world.get(), self);
		return;
	}
	if (self->keys.rightMouseDown && self->hotbar.getSlot(self->hotbar.selectedIndex) && self->hotbar.getSlot(self->hotbar.selectedIndex)->getName() == "Spindle And Distaff")
	{
		isSpinning = true;
		self->inventoryManager.craftingText.setText("Spinning:");
		utils::openInventory(StateGame::instanceObj.world.get(), self);
		return;
	}

	return original(self, world, dt, window);
}


$hook(void, CraftingMenu, updateAvailableRecipes)
{
	original(self);

	for (auto it = self->craftableRecipes.begin(); it < self->craftableRecipes.end(); )
	{

		if (isSpinning && it->result->getName().find("Hyperfabric")==std::string::npos) {
			it = self->craftableRecipes.erase(it);
			continue;
		}
		if (!isSpinning && it->result->getName().find("Hyperfabric") != std::string::npos) {
			it = self->craftableRecipes.erase(it);
			continue;
		}
		if (isWeaving && it->result->getName().find("Backpack") == std::string::npos) {
			it = self->craftableRecipes.erase(it);
			continue;
		}
		if (!isWeaving && it->result->getName().find("Backpack") != std::string::npos) {
			it = self->craftableRecipes.erase(it);
			continue;
		}
		it++;
	}
	self->Interface->updateCraftingMenuBox();
}

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
	if (!utils::contains(materialNames,(std::string)self->getName()))
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

// Render tools icons
$hook(void, ItemTool, render, const glm::ivec2& pos)
{
	if (!utils::contains(toolNames, (std::string)self->getName()))
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
	
	enum BackpackAccessMode {
		Storing,
		Accessing,
		None
	};
	static BackpackAccessMode accessMode = None;
	static ItemBackpack* backpack = nullptr;
	static int index = -1;
	static Inventory* inventory;
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
	inventory = manager.primary;
	index = inventory->getSlotIndex({ mouseX,mouseY });

	if (index == -1) {
		inventory = manager.secondary;
		index = inventory->getSlotIndex({ mouseX,mouseY });
	}


	if (index ==-1) return true; // Didnt even target a slot

	itemInSlot = &inventory->getSlot(index);

	if (accessMode == None && itemInSlot->get() == nullptr) { // Slot is empty, get to accesing backpack
		accessMode = Accessing;
	}
	else if (accessMode == None) { // Slot is not empty, get to storing stuff in a backpack 
		accessMode = Storing;
	}

	if (itemInSlot->get() == nullptr && accessMode == Accessing) { // Slot is empty, put stuff from backpack into it
		//utils::swapIndex(&manager, inventory, &backpack->getInventory(&player), index, utils::getLastItemIndex(&backpack->getInventory(&player)), manager.secondary);
		int backpackIndex = utils::getLastItemIndex(&backpack->getInventory(&player));
		if (backpackIndex == -1) return true;
		utils::cursorTransfer(&manager, inventory, index, manager.secondary);
		utils::cursorTransfer(&manager, &backpack->getInventory(&player), backpackIndex, manager.secondary,std::format("Backpack.{}.{}",inventory->name, index));
		utils::cursorTransfer(&manager, inventory, index, manager.secondary);

		manager.craftingMenu.updateAvailableRecipes();
		manager.updateCraftingMenuBox();
	}
	else if (accessMode == Storing && itemInSlot->get() !=nullptr) { // Slot is not empty, put stuff from it into backpack 
		utils::placeInto(&manager, inventory , index, &backpack->getInventory(&player), manager.primary,std::format("Backpack.{}.{}",inventory->name,index));

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

void addRecipe(const std::string& resultName, int resultCount,
	const std::vector<std::pair<std::string, int>>& components) {

	nlohmann::json recipeJson;
	recipeJson["result"] = { {"name", resultName}, {"count", resultCount} };

	nlohmann::json recipeComponents = nlohmann::json::array();
	for (const auto& [name, count] : components) {
		recipeComponents.push_back({ {"name", name}, {"count", count} });
	}

	recipeJson["recipe"] = recipeComponents;
	if (!CraftingMenu::recipes.contains(recipeJson))
		CraftingMenu::recipes.push_back(recipeJson);
}
void initRecipes() {
	addRecipe("Weaving Loom", 1, { {"Stick",3},{"Wood",2},{"Iron Bars",1} });
	addRecipe("Spindle And Distaff", 1, { {"Stick",2},{"Hypersilk",2},{"Rock",1} });
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
void initBlueprints() {
	// Materials
	for (auto material : materialNames)
		(Item::blueprints)[material] =
	{
		{ "type", "material"},
		{ "baseAttributes", nlohmann::json::object()}
	};

	// Backpacks
	for (int i=0;i<backpackNames.size();i++)
		Item::blueprints[backpackNames[i]] =
	{
		{ "type", "backpack" },
		{ "baseAttributes", { { "type", i }, { "inventory", nlohmann::json::array() },{"isSolenoidEffectActive",false} } }
	};

	// Tools
	for (auto tool : toolNames)
		Item::blueprints[tool] =
	{
		{ "type", "tool" },
		{ "baseAttributes", nlohmann::json::object()}
	};

}
void initSounds() {
	ItemBackpack::openSound = std::format("../../{}/assets/backpackOpen.ogg", fdm::getModPath(fdm::modID));

	if (!AudioManager::loadSound(ItemBackpack::openSound)) Console::printLine("Cannot load sound: ", ItemBackpack::openSound);
}
void initShaders() {

}
$hook(void, StateIntro, init, StateManager& s)
{
	original(self, s);

	//Initialize opengl stuff
	glewExperimental = true;
	glewInit();
	glfwInit();

	ItemBackpack::rendererInit();

	initSounds();

	initShaders();
}

$hookStatic(void, CraftingMenu, loadRecipes)
{
	static bool recipesLoaded = false;

	if (recipesLoaded) return;

	recipesLoaded = true;

	original();

	initRecipes();
}
$hookStatic(bool, Item, loadItemInfo)
{
	static bool loaded = false;
	if (loaded) return false;
	loaded = true;

	bool result = original();

	initBlueprints();

	return result;
}
