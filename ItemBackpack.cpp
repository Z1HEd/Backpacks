#include "ItemBackpack.h"

MeshRenderer ItemBackpack::renderer{};
std::string ItemBackpack::openSound="";

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
	if (!player->keys.rightMouseDown || (player->inventoryManager.isOpen())) return false;

	
	player->inventoryManager.primary = &player->playerInventory;
	player->shouldResetMouse = true;
	player->inventoryManager.secondary = &inventory;


	player->inventoryManager.craftingMenu.updateAvailableRecipes();
	player->inventoryManager.updateCraftingMenuBox();

	openInstance.inventory = &inventory;
	openInstance.manager = &player->inventoryManager;

	inventory.renderPos =  glm::ivec2{397,50};

	AudioManager::playSound4D(openSound, "ambience", player->cameraPos, { 0,0,0,0 });

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
		color = glm::vec3{ 188.0f / 255.0f, 74.0f / 255.0f, 153.0f / 255.0f };
	else if (this->type == IRON)
		color = glm::vec3{ 181.0f / 255.0f, 179.0f / 255.0f, 174.0f / 255.0f };
	else
		color = glm::vec3(201 / 255.0f, 206 / 255.0f, 255 / 255.0f);

	m4::Mat5 materialLower = MV;
	materialLower.translate(glm::vec4{ 0.0f, 0.0f, 0.0f, 0.001f });
	materialLower.scale(glm::vec4{ 2.f,1.f,1.f,1.f });
	materialLower.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 materialUpper = MV;
	materialUpper.translate(glm::vec4{ 0.0f, 0.0f, 0.0f, 0.001f });
	materialUpper.translate(glm::vec4{ 0, 1.25f, 0.25f, 0 });
	materialUpper.scale(glm::vec4{ 1.5f,0.5f,0.5f,0.5f });
	materialUpper.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 materialMiddle = MV;
	materialMiddle.translate(glm::vec4{ 0.0f, 0.0f, 0.0f, 0.001f });
	materialMiddle.translate(glm::vec4{ 0, 0.75f, 0.1f, 0 });
	materialMiddle.scale(glm::vec4{ 1.8f,0.5f,0.8f,0.8f });
	materialMiddle.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 materialBottom = MV;
	materialBottom.translate(glm::vec4{ 0.0f, 0.0f, 0.0f, 0.001f });
	materialBottom.translate(glm::vec4(0, -0.6f, 0, 0));
	materialBottom.scale(glm::vec4{ 1.8f,0.2f,0.8f,0.8f });
	materialBottom.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 handleLeft = MV;
	handleLeft.translate(glm::vec4{ 0.0f, 0.0f, 0.0f, 0.001f });
	handleLeft.translate(glm::vec4{ .3, 1.6f, 0.35f, 0 });
	handleLeft.scale(glm::vec4{ 0.1f,0.2f,0.1f,0.1f });
	handleLeft.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 handleRight = MV;
	handleRight.translate(glm::vec4{ 0.0f, 0.0f, 0.0f, 0.001f });
	handleRight.translate(glm::vec4{ -.3, 1.6f, 0.35f, 0 });
	handleRight.scale(glm::vec4{ 0.1f,0.2f,0.1f,0.1f });
	handleRight.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 handleTop = MV;
	handleTop.translate(glm::vec4{ 0.0f, 0.0f, 0.0f, 0.001f });
	handleTop.translate(glm::vec4{ 0.0f, 1.75f, 0.35f, 0 });
	handleTop.scale(glm::vec4{ 0.7f,0.1f,0.1f,0.1f });
	handleTop.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	// Just guessing numbers at this point . _.
	m4::Mat5 leftStrapTop = MV;
	leftStrapTop.translate(glm::vec4{ 0.0f, 0.0f, 0.0f, 0.001f });
	leftStrapTop.translate(glm::vec4{ 0.4f, 1.25f, 0.55f, 0 });
	leftStrapTop.scale(glm::vec4{ 0.3f,0.1f,0.15f,0.1f });
	leftStrapTop.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 leftStrapBottom = MV;
	leftStrapBottom.translate(glm::vec4{ 0.0f, 0.0f, 0.0f, 0.001f });
	leftStrapBottom.translate(glm::vec4{ 0.4f, -0.25f, 0.55f, 0 });
	leftStrapBottom.scale(glm::vec4{ 0.3f,0.1f,0.15f,0.1f });
	leftStrapBottom.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 leftStrapMid = MV;
	leftStrapMid.translate(glm::vec4{ 0.0f, 0.0f, 0.0f, 0.001f });
	leftStrapMid.translate(glm::vec4{ 0.4f, 0.5f, 0.65f, 0 });
	leftStrapMid.scale(glm::vec4{ 0.3f,1.6f,0.1f,0.1f });
	leftStrapMid.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 rightStrapTop = MV;
	rightStrapTop.translate(glm::vec4{ 0.0f, 0.0f, 0.0f, 0.001f });
	rightStrapTop.translate(glm::vec4{ -0.4f, 1.25f, 0.55f, 0 });
	rightStrapTop.scale(glm::vec4{ 0.3f,0.1f,0.15f,0.1f });
	rightStrapTop.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 rightStrapBottom = MV;
	rightStrapBottom.translate(glm::vec4{ 0.0f, 0.0f, 0.0f, 0.001f });
	rightStrapBottom.translate(glm::vec4{ -0.4f, -0.25f, 0.55f, 0 });
	rightStrapBottom.scale(glm::vec4{ 0.3f,0.1f,0.15f,0.1f });
	rightStrapBottom.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 rightStrapMid = MV;
	rightStrapMid.translate(glm::vec4{ 0.0f, 0.0f, 0.0f, 0.001f });
	rightStrapMid.translate(glm::vec4{ -0.4f, 0.5f, 0.65f, 0 });
	rightStrapMid.scale(glm::vec4{ 0.3f,1.6f,0.1f,0.1f });
	rightStrapMid.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	const Shader* shader = ShaderManager::get("tetSolidColorNormalShader");

	shader->use();

	glUniform4f(glGetUniformLocation(shader->id(), "lightDir"), lightDir.x, lightDir.y, lightDir.z, lightDir.w);
	glUniform4f(glGetUniformLocation(shader->id(), "inColor"), color.r, color.g, color.b, 1);

	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(materialLower) / sizeof(float), &materialLower[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(materialUpper) / sizeof(float), &materialUpper[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(materialBottom) / sizeof(float), &materialBottom[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(materialMiddle) / sizeof(float), &materialMiddle[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(handleLeft) / sizeof(float), &handleLeft[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(handleRight) / sizeof(float), &handleRight[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(handleTop) / sizeof(float), &handleTop[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(leftStrapTop) / sizeof(float), &leftStrapTop[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(leftStrapMid) / sizeof(float), &leftStrapMid[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(leftStrapBottom) / sizeof(float), &leftStrapBottom[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(rightStrapTop) / sizeof(float), &rightStrapTop[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(rightStrapMid) / sizeof(float), &rightStrapMid[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(rightStrapBottom) / sizeof(float), &rightStrapBottom[0][0]);
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
	return { { "type", (int)type }, { "inventory", inventory.save()}};
}

std::unique_ptr<Item> ItemBackpack::clone() {
  auto result = std::make_unique<ItemBackpack>();

  result->type = type;
  nlohmann::json inventoryAttributes = inventory.save();
  result->inventory = InventoryGrid(result->sizes[result->type]);
  result->inventory.load(inventoryAttributes);
  result->inventory.name = "backpackInventory";
  return result;
}

// instantiating backpack item
$hookStatic(std::unique_ptr<Item>, Item, instantiateItem, const stl::string& itemName, uint32_t count, const stl::string& type, const nlohmann::json& attributes) {
	
	if (itemName.find("Backpack") == std::string::npos)
		return original(itemName, count, type, attributes);

	auto result = std::make_unique<ItemBackpack>();
	result->type = (ItemBackpack::BackpackType)(int)attributes["type"];
	if (!attributes["inventory"].empty()) {
		result->inventory = InventoryGrid(result->sizes[result->type]);
		result->inventory.load(attributes["inventory"]);
	}
	else
		result->inventory = InventoryGrid(result->sizes[result->type]);
	result->inventory.name = "backpackInventory";
	result->count = count;
	return result;
}
