#include "ItemBackpack.h"

stl::string ItemBackpack::getName() { return "Backpack"; }
void ItemBackpack::renderEntity(const m4::Mat5& MV, bool inHand, const glm::vec4& lightDir) {
	glm::vec3 color{ 1 };
	if (this->type == DEADLY)
		color = glm::vec3{ 232.0f / 255.0f, 77.0f / 255.0f, 193.0f / 255.0f } *1.4f;
	else if (this->type == IRON)
		color = glm::vec3{ 242.0f / 255.0f, 240.0f / 255.0f, 240.0f / 255.0f };
	else
		color = glm::vec3{ 226.0f / 255.0f, 229.0f / 255.0f, 254.0f / 255.0f };

	m4::Mat5 material = MV;
	material.translate(glm::vec4{ 0.0f, 0.0f, 0.0f, 0.001f });
	material.scale(glm::vec4{ 0.5f });
	material.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	const Shader* backpackShader = ShaderManager::get("backpackShader");

	backpackShader->use();

	glUniform4f(glGetUniformLocation(backpackShader->id(), "lightDir"), lightDir.x, lightDir.y, lightDir.z, lightDir.w);
	glUniform4f(glGetUniformLocation(backpackShader->id(), "inColor"), color.r, color.g, color.b, 1);
	glUniform1fv(glGetUniformLocation(backpackShader->id(), "MV"), sizeof(material) / sizeof(float), &material[0][0]);

	ItemTool::rockRenderer->render();
}
bool ItemBackpack::isDeadly() { return type == DEADLY; }
uint32_t ItemBackpack::getStackLimit() { return 1; }
bool ItemBackpack::action(World* world, Player* player, int action) {
	Console::printLine(action);
}
void ItemBackpack::postAction(World* world, Player* player, int action) {
	Console::printLine(action);
}
nlohmann::json ItemBackpack::saveAttributes() { return inventory.save(); }