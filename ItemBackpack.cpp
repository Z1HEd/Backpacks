#include "ItemBackpack.h"

MeshRenderer ItemBackpack::renderer{};

stl::string ItemBackpack::getName() {
	switch (type) {
	case BackpackType::FABRIC:
		return "Backpack";
	case BackpackType::IRON:
		return "Reinforced Backpack";
	case BackpackType::DEADLY:
		return "Deadly Backpack";
	}
	
}

bool ItemBackpack::action(World* world, Player* player, int action) {
	if (!player->keys.rightMouseDown) return false;

	
	player->inventoryManager.primary = &player->playerInventory;
	player->shouldResetMouse = true;
	player->inventoryManager.secondary = inventory.get();


	player->inventoryManager.craftingMenu.updateAvailableRecipes();
	player->inventoryManager.updateCraftingMenuBox();

	openInstance.inventory = inventory.get();
	openInstance.manager = &player->inventoryManager;

	inventory->renderPos = player->inventory.renderPos + glm::ivec2{290,0};

	return true;
}

void ItemBackpack::render(const glm::ivec2& pos) {
	TexRenderer& tr = *ItemTool::tr; // or TexRenderer& tr = ItemTool::tr; after 0.3
	const Tex2D* ogTex = tr.texture; // remember the original texture

	tr.texture = ResourceManager::get("assets/Tools.png", true); // set to custom texture
	tr.setClip(type * 36, 0, 36, 36);
	tr.setPos(pos.x, pos.y, 70, 72);
	tr.render();

	tr.texture = ogTex; // return to the original texture

	// inputs stuff
}

void ItemBackpack::renderEntity(const m4::Mat5& MV, bool inHand, const glm::vec4& lightDir) {

	glm::vec3 color{ 1 };
	if (this->type == DEADLY)
		color = glm::vec3{ 232.0f / 255.0f, 77.0f / 255.0f, 193.0f / 255.0f };
	else if (this->type == IRON)
		color = glm::vec3{ 181.0f / 255.0f, 179.0f / 255.0f, 174.0f / 255.0f };
	else
		color = glm::vec3(201 / 255.0f, 206 / 255.0f, 255 / 255.0f);

	m4::Mat5 material = MV;
	material.translate(glm::vec4{ 0.0f, 0.0f, 0.0f, 0.001f });
	material.scale(glm::vec4{ 0.5f });
	material.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	const Shader* shader = ShaderManager::get("tetSolidColorNormalShader");

	shader->use();

	glUniform4f(glGetUniformLocation(shader->id(), "lightDir"), lightDir.x, lightDir.y, lightDir.z, lightDir.w);
	glUniform4f(glGetUniformLocation(shader->id(), "inColor"), color.r, color.g, color.b, 1);
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(material) / sizeof(float), &material[0][0]);

	renderer.render();
}

void ItemBackpack::rendererInit() {
	MeshBuilder mesh{ BlockInfo::HYPERCUBE_FULL_INDEX_COUNT };
	// vertex position attribute
	mesh.addBuff(BlockInfo::hypercube_full_verts, sizeof(BlockInfo::hypercube_full_verts));
	mesh.addAttr(GL_UNSIGNED_BYTE, 4, sizeof(glm::u8vec4));
	// per-cell normal attribute
	mesh.addBuff(BlockInfo::hypercube_full_normals, sizeof(BlockInfo::hypercube_full_normals));
	mesh.addAttr(GL_FLOAT, 1, sizeof(GLfloat));

	mesh.setIndexBuff(BlockInfo::hypercube_full_indices, sizeof(BlockInfo::hypercube_full_indices));

	renderer.setMesh(&mesh);
}

bool ItemBackpack::isDeadly() { return type == DEADLY; }
uint32_t ItemBackpack::getStackLimit() { return 1; }

nlohmann::json ItemBackpack::saveAttributes() {
	return { { "type", (int)type }, { "inventory", inventory->save() } };
}

std::unique_ptr<Item> ItemBackpack::clone() {
  auto result = std::make_unique<ItemBackpack>();

  result->type = type;
  nlohmann::json inventoryAttributes = inventory->save();
  result->inventory = std::make_unique<InventoryGrid>(result->sizes[result->type]);
  result->inventory->load(inventoryAttributes);
  result->inventory->name = "backpackInventory";
  return result;
}

// instantiating backpack item
$hookStatic(std::unique_ptr<Item>, Item, instantiateItem, const stl::string& itemName, uint32_t count, const stl::string& type, const nlohmann::json& attributes) {

	if (itemName.find("Backpack") == std::string::npos)
		return original(itemName, count, type, attributes);

	auto result = std::make_unique<ItemBackpack>();
	result->type = (ItemBackpack::BackpackType)(int)attributes["type"];
	if (!attributes["inventory"].empty()) {
		result->inventory = std::make_unique<InventoryGrid>(result->sizes[result->type]);
		result->inventory->load(attributes["inventory"]);
	}
	else
		result->inventory = std::make_unique<InventoryGrid>(result->sizes[result->type]);
	result->inventory->name = "backpackInventory";
	result->count = count;
	return result;
}
