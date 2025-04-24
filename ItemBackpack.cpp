#include "ItemBackpack.h"

MeshRenderer ItemBackpack::renderer{};
std::string ItemBackpack::openSound="";

InventoryGrid playerWormholeInventory = {};

// Wormhole inventory

InventoryGrid& ItemBackpack::getUsedInventory() {
	return type == WORMHOLE ? playerWormholeInventory : inventory;
}

$hook(nlohmann::json&, Player, save, nlohmann::json* _) {
	nlohmann::json& ret = original(self, _);
	
	ret["wormholeInventory"] = playerWormholeInventory.save();

	return ret;
}

$hook(void, Player, load, nlohmann::json& j) {
	original(self, j);

	playerWormholeInventory = InventoryGrid(ItemBackpack::sizes[ItemBackpack::WORMHOLE]);
	if (j.contains("wormholeInventory")) {
		playerWormholeInventory.load(j["wormholeInventory"]);
		playerWormholeInventory.name = "wormholeInventory";
		playerWormholeInventory.label = std::format("Wormhole Backpack");
	}
}

// Solenoid backpack effect
void applySolenoidEffect(World* world, Player* player) {
	int xPlayer, zPlayer, wPlayer;
	xPlayer = std::roundf( player->pos.x / 8);
	zPlayer = std::roundf(player->pos.z / 8);
	wPlayer = std::roundf(player->pos.w / 8);

	for (int x = 0;x < 2;x++)
		for (int z = 0;z < 2;z++)
			for (int w = 0;w < 2;w++)
			{
				
				Chunk* chunk = world->getChunk(glm::i64vec3{ x+xPlayer-1,z + zPlayer-1,w + wPlayer-1 });
				if (!chunk) 
					continue;
				for (auto entity : chunk->entities) {
					if (entity->getName() != "Item") continue;
					glm::vec4 towardsPlayer = player->pos - entity->getPos();
					float distance = glm::length(towardsPlayer);
					if (distance < 8) {
						((EntityItem*)entity)->hitbox.deltaVel += towardsPlayer / (distance)*0.15f;
						((EntityItem*)entity)->combineWithNearby(world);
					}
					
				}
			}

}

$hook(void, Player, update,World* world, double dt) {
	original(self, world, dt);
	ItemBackpack* backpack = dynamic_cast<ItemBackpack*>(self->hotbar.getSlot(self->hotbar.selectedIndex).get());
	if (!backpack) backpack = dynamic_cast<ItemBackpack*>(self->equipment.getSlot(0).get());
	if (!backpack) return;
	if (backpack->type == ItemBackpack::SOLENOID && backpack->isSolenoidEffectActive) applySolenoidEffect(world, self);
}

// Automatically store items in solenoid backpack

$hook(void, WorldSingleplayer, localPlayerEvent, Player* player, Packet::ClientPacket eventType, int64_t eventValue, void* data) {
	if (eventType != Packet::C_ITEM_COLLECT) return original(self, player, eventType, eventValue, data);
	
	ItemBackpack* backpack = dynamic_cast<ItemBackpack*>(player->hotbar.getSlot(player->hotbar.selectedIndex).get());
	if (!backpack) backpack = dynamic_cast<ItemBackpack*>(player->equipment.getSlot(0).get());
	if (!backpack) return original(self, player, eventType, eventValue, data);
	if (backpack->type != ItemBackpack::SOLENOID || !backpack->isSolenoidEffectActive) return original(self, player, eventType, eventValue, data);
	if (dynamic_cast<ItemBackpack*>(((EntityItem*)data)->item.get()))  return original(self, player, eventType, eventValue, data);
	((EntityItem*)data)->give(&backpack->inventory, eventType);
}

stl::string ItemBackpack::getName() {
	switch (type) {
	case BackpackType::FABRIC:
		return "Backpack";
	case BackpackType::IRON:
		return "Reinforced Backpack";
	case BackpackType::DEADLY:
		return "Deadly Backpack";
	case BackpackType::SOLENOID:
		return "Solenoid Backpack";
	case BackpackType::WORMHOLE:
		return "Wormhole Backpack";
	}

	return "Backpack";
}

std::unique_ptr<Item>* ItemBackpack::getFirstItem() {
	InventoryGrid& usedInventory = getUsedInventory();
	for (uint32_t i = 0;i < usedInventory.getSlotCount();i++) {
		auto* slot = &usedInventory.getSlot(i);
		if (slot && slot->get()) return slot;
	}
	return nullptr;
}

std::unique_ptr<Item>* ItemBackpack::getLastItem() {
	InventoryGrid& usedInventory = getUsedInventory();
	for (int i = usedInventory.getSlotCount() - 1;i>=0;i--) {
		auto* slot = &usedInventory.getSlot(i);
		if (slot && slot->get()) return slot;
	}
	return nullptr;
}

$hook(void,Player, mouseButtonInput, GLFWwindow* window, World* world, int button, int action, int mods) {
	ItemBackpack* backpack;
	backpack = dynamic_cast<ItemBackpack*>(self->hotbar.getSlot(self->hotbar.selectedIndex).get());
	if (!backpack) backpack = dynamic_cast<ItemBackpack*>(self->equipment.getSlot(0).get());
	if (!backpack) return original(self, window, world, button, action, mods);

	if (self->inventoryManager.isOpen()) return original(self, window, world, button, action, mods);
	if (action != GLFW_PRESS || button!=GLFW_MOUSE_BUTTON_2) return original(self, window, world, button, action, mods);

	if (self->keys.shift) {
		backpack->isSolenoidEffectActive = !backpack->isSolenoidEffectActive;
		return;
	}

	self->inventoryManager.primary = &self->playerInventory;
	self->shouldResetMouse = true;
	self->inventoryManager.secondary = backpack->type == ItemBackpack::WORMHOLE ? &playerWormholeInventory : &backpack->inventory;


	self->inventoryManager.craftingMenu.updateAvailableRecipes();
	self->inventoryManager.updateCraftingMenuBox();

	backpack->openInstance.inventory = &backpack->inventory;
	backpack->openInstance.manager = &self->inventoryManager;

	((InventoryGrid*)self->inventoryManager.secondary)->renderPos = glm::ivec2{ 397,50 };

	AudioManager::playSound4D(ItemBackpack::openSound, "ambience", self->cameraPos, { 0,0,0,0 });
}

void ItemBackpack::render(const glm::ivec2& pos) {
	TexRenderer& tr = ItemTool::tr; // or TexRenderer& tr = ItemTool::tr; after 4dmodding v2.2
	const Tex2D* ogTex = tr.texture; // remember the original texture

	static std::string iconPath = "";
	iconPath = std::format("{}{}.png", "assets/", this->getName().c_str());
	iconPath.erase(remove(iconPath.begin(), iconPath.end(), ' '), iconPath.end());

	tr.texture = ResourceManager::get(iconPath, true); // set to custom texture
	tr.setClip(0, 0, 36, 36);
	tr.setPos(pos.x, pos.y, 70, 72);
	tr.render();

	tr.texture = ogTex; // return to the original texture
}

void ItemBackpack::renderEntity(const m4::Mat5& MV, bool inHand, const glm::vec4& lightDir) {

	glm::vec3 color{ 1 };

	switch (type) {
	case BackpackType::FABRIC:
		color = glm::vec3(201 / 255.0f, 206 / 255.0f, 255 / 255.0f);
		break;
	case BackpackType::IRON:
		color = glm::vec3{ 181.0f / 255.0f, 179.0f / 255.0f, 174.0f / 255.0f };
		break;
	case BackpackType::DEADLY:
		color = glm::vec3{ 188.0f / 255.0f, 74.0f / 255.0f, 153.0f / 255.0f };
		break;
	case BackpackType::SOLENOID:
		color = glm::vec3{ 197.0f / 255.0f, 62.0f / 255.0f, 107.0f / 255.0f };
		if (!isSolenoidEffectActive)
			color *= 0.8f;
		break;
	case BackpackType::WORMHOLE:
		color = glm::vec3{ 34.0f / 255.0f, 29.0f / 255.0f, 50.0f / 255.0f };
		break;
	}

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

	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(m4::Mat5) / sizeof(float), &materialLower[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(m4::Mat5) / sizeof(float), &materialUpper[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(m4::Mat5) / sizeof(float), &materialBottom[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(m4::Mat5) / sizeof(float), &materialMiddle[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(m4::Mat5) / sizeof(float), &handleLeft[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(m4::Mat5) / sizeof(float), &handleRight[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(m4::Mat5) / sizeof(float), &handleTop[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(m4::Mat5) / sizeof(float), &leftStrapTop[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(m4::Mat5) / sizeof(float), &leftStrapMid[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(m4::Mat5) / sizeof(float), &leftStrapBottom[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(m4::Mat5) / sizeof(float), &rightStrapTop[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(m4::Mat5) / sizeof(float), &rightStrapMid[0][0]);
	renderer.render();
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(m4::Mat5) / sizeof(float), &rightStrapBottom[0][0]);
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
	return { { "type", (int)type }, { "inventory", inventory.save()},{"isSolenoidEffectActive",isSolenoidEffectActive}};
}

std::unique_ptr<Item> ItemBackpack::clone() {
  auto result = std::make_unique<ItemBackpack>();

  result->type = type;
  result->isSolenoidEffectActive = isSolenoidEffectActive;
  result->inventory = inventory;
  result->inventory.name = "backpackInventory";
  return result;
}

bool ItemBackpack::isCompatible(const std::unique_ptr<Item>& other)
{
	auto* backpack = dynamic_cast<ItemBackpack*>(other.get());
	return backpack && backpack->type == type;
}

// instantiating backpack item
$hookStatic(std::unique_ptr<Item>, Item, instantiateItem, const stl::string& itemName, uint32_t count, const stl::string& type, const nlohmann::json& attributes) {
	
	// I'm a never nester
	if (type != "backpack") return original(itemName, count, type, attributes);

	auto result = std::make_unique<ItemBackpack>();
	result->type = (ItemBackpack::BackpackType)(int)attributes["type"];
	if (attributes.contains("isSolenoidEffectActive"))
		result->isSolenoidEffectActive = attributes["isSolenoidEffectActive"];
	result->inventory = InventoryGrid(ItemBackpack::sizes[result->type]);
	result->inventory.load(attributes["inventory"]);
	result->inventory.name = result->type==ItemBackpack::WORMHOLE ? "wormholeInventory" : "backpackInventory";
	result->inventory.label = std::format("{}:", result->getName());
	result->count = count;
	return result;
}
