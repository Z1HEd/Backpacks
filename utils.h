#pragma once
#include<4dm.h>

using namespace fdm;

namespace utils {
    inline static int getFirstItemIndex(Inventory* inventory) {
        for (uint32_t i = 0;i < inventory->getSlotCount();i++) {
            auto* slot = &inventory->getSlot(i);
            if (slot && slot->get()) return i;
        }
        return -1;
    }

    inline static int getLastItemIndex(Inventory* inventory) {
        for (int i = inventory->getSlotCount() - 1;i >= 0;i--) {
            auto* slot = &inventory->getSlot(i);
            if (slot && slot->get()) return i;
        }
        return -1;
    }
    inline static int getFirstEmptyIndex(Inventory* inventory) {
        for (uint32_t i = 0;i < inventory->getSlotCount();i++) {
            auto* slot = &inventory->getSlot(i);
            if (!slot || !slot->get()) return i;
        }
        return -1;
    }

    inline static int getLastEmptyIndex(Inventory* inventory) {
        for (int i = inventory->getSlotCount() - 1;i >= 0;i--) {
            auto* slot = &inventory->getSlot(i);
            if (!slot || !slot->get()) return i;
        }
        return -1;
    }


    inline static void cursorTransfer(InventoryManager* manager, Inventory* inventory, int index, Inventory* other,std::string inventoryNameOverride="") {
        const nlohmann::json action = {
                {"action","transfer"},
                {"cursorContents",manager->cursor.item != nullptr ? manager->cursor.item->save().dump() : ""},
                {"inventory",inventoryNameOverride == "" ? (std::string)inventory->name : inventoryNameOverride},
                {"other",other->name},
                {"slotContents", inventory->getSlot(index) != nullptr ? inventory->getSlot(index)->save().dump() : ""},
                {"slotIndex",index},
                {"transferAction",InventoryManager::ACTION_SWAP}
        };
        if (manager->callback)
            manager->callback(action, manager->user);

        manager->applyTransfer(InventoryManager::ACTION_SWAP, inventory->getSlot(index), manager->cursor.item, other);;
    }

    inline static void swapIndex(InventoryManager* manager, Inventory* inventoryA, Inventory* inventoryB, int indexA, int indexB, Inventory* other) {
        if (indexA == -1 || indexB == -1) return;
        cursorTransfer(manager, inventoryA, indexA, other);
        cursorTransfer(manager, inventoryB, indexB, other);
        cursorTransfer(manager, inventoryA, indexA, other);
    }

    inline static void combineInto(InventoryManager* manager, Inventory* fromInventory, int fromIndex, Inventory* intoInventory, Inventory* other, std::string intoNameOverride = "") {
        if (fromIndex == -1) return;
        cursorTransfer(manager, fromInventory, fromIndex, other); // Make sure the item to be placed is in the cursor slot


        for (int i = 0;i < (int)intoInventory->getSlotCount();i++) {
            if (intoInventory->getSlot(i) == nullptr ||
                intoInventory->getSlot(i)->getName() != manager->cursor.item->getName() ||
                intoInventory->getSlot(i)->count >= intoInventory->getSlot(i)->getStackLimit()) continue;
            if (manager->callback)
                manager->callback({
                    {"action","transfer"},
                    {"cursorContents",manager->cursor.item != nullptr ? manager->cursor.item->save().dump() : ""},
                    {"inventory",intoNameOverride==""? intoInventory->name.c_str() : intoNameOverride},
                    {"other",other->name},
                    {"slotContents", intoInventory->getSlot(i) != nullptr ? intoInventory->getSlot(i)->save().dump() : ""},
                    {"slotIndex",i},
                    {"transferAction",InventoryManager::ACTION_GIVE_MAX}
                    },
                    manager->user);
            manager->applyTransfer(InventoryManager::ACTION_GIVE_MAX, intoInventory->getSlot(i), manager->cursor.item, manager->secondary);
            if (!manager->cursor.item || manager->cursor.item->count < 1) return;
        }
        cursorTransfer(manager, fromInventory, fromIndex, other);
    }
    inline static void placeInto(InventoryManager* manager, Inventory* fromInventory, int fromIndex, Inventory* intoInventory, Inventory* other, std::string intoNameOverride = "") {
        cursorTransfer(manager, fromInventory, fromIndex, other); // Make sure the item to be placed is in the cursor slot
        int freeIndex = getFirstEmptyIndex(intoInventory);
        if (manager->cursor.item && freeIndex != -1) { // if anything left, put into new slot
            cursorTransfer(manager, intoInventory, freeIndex, other, intoNameOverride);
        }
        cursorTransfer(manager, fromInventory, fromIndex, other);
    }
    inline static void placeAndCombineInto(InventoryManager* manager, Inventory* fromInventory, int fromIndex, Inventory* intoInventory, Inventory* other, std::string intoNameOverride="") {
        if (fromIndex == -1) return;

        combineInto(manager, fromInventory, fromIndex, intoInventory, other,intoNameOverride);

        placeInto(manager, fromInventory, fromIndex, intoInventory, other,intoNameOverride);
    }

    template <class T>
    inline static bool contains(const std::vector<T>& vec, const T& value)
    {
        return std::find(vec.begin(), vec.end(), value) != vec.end();
    }

    inline static void openInventory(World* world, Player* player) {
        player->inventoryManager.primary = &player->hotbar;
        player->inventoryManager.secondary = &player->inventoryAndEquipment;
        player->inventoryManager.craftingMenu.updateAvailableRecipes();
        player->inventoryManager.updateCraftingMenuBox();
        player->resetMouse(StateGame::instanceObj.ui.getGLFWwindow());
        world->localPlayerEvent(player, Packet::C_INVENTORY_OPEN, 0, nullptr);
    }

    inline static std::vector<std::string> split(std::string string, std::string delimeter) {
        std::vector<std::string> output = {};
        

        auto pos = string.find(delimeter);
        auto prevPos = string.find("");
        
        while (pos != std::string::npos) {
            output.push_back(string.substr(prevPos, pos - prevPos));

            prevPos = pos+1;
            pos = string.find(delimeter,pos+1);
        }
        output.push_back(string.substr(prevPos, string.size()));
        return output;
    }
}