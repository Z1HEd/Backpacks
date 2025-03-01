#define DEBUG_CONSOLE // Uncomment this if you want a debug console to start. You can use the Console class to print. You can use Console::inStrings to get input.

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

// instantiating backpack item
$hookStatic(std::unique_ptr<Item>, Item, instantiateItem, const stl::string& itemName, uint32_t count, const stl::string& type, const nlohmann::json& attributes) {
	
	if (itemName.find("Backpack") == std::string::npos)
		return original(itemName, count, type, attributes);
	ItemBackpack backpack;

	if (itemName == "Backpack")
		backpack.type = ItemBackpack::FABRIC;
	else if (itemName == "Reinforced Backpack")
		backpack.type = ItemBackpack::IRON;
	else
		backpack.type = ItemBackpack::DEADLY;

	backpack.count = 1;

	nlohmann::json constAttributes = attributes; // For some reason in InventoryGrid::load() attributes are not const
	backpack.inventory.load(constAttributes);
	return std::make_unique<Item>(backpack);
}

// add item blueprints, load shaders
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
			{ "type", "tool" },
			{ "baseAttributes", InventoryGrid().save()}
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