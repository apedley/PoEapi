/*
* InGameUI.cpp, 8/18/2020 6:46 PM
*/

#include <unordered_map>

#include "ui/Inventory.cpp"
#include "ui/Stash.cpp"
#include "ui/Vendor.cpp"
#include "ui/Purchase.cpp"
#include "ui/Sell.cpp"
#include "ui/Trade.cpp"
#include "ui/OverlayMap.cpp"
#include "ui/Chat.cpp"
#include "ui/Notifications.cpp"
#include "ui/Atlas.cpp"
#include "ui/Skills.cpp"

std::map<string, int> in_game_ui_offsets {
    {"inventory",       0x500},
        {"grid",        0x3a8},
    {"stash",           0x538},
        {"tabs",        0x2c0},
    {"overlay_map",     0x5b8},
        {"large",       0x230},
        {"small",       0x238},
    {"chat",            0x410},
    {"lefe_panel",      0x508},
    {"right_panel",     0x510},
    {"panel_flags",     0x518},
    {"skills",          0x560},
    {"atlas",           0x568},
    {"entity_list",     0x5c0},
        {"root",        0x2a0},
        {"count",       0x2a8},
    {"trade",           0x678},
    {"gem_level_up",    0x8d8},
    {"notifications",   0x920},
};

enum {
    InventoryIndex  = 32,
    StashIndex      = 33,
};

class InGameUI : public Element {
public:

    unique_ptr<Inventory> inventory;
    unique_ptr<Stash> stash;
    unique_ptr<Vendor> vendor;
    unique_ptr<Purchase> purchase;
    unique_ptr<Sell> sell;
    unique_ptr<Trade> trade;
    unique_ptr<OverlayMap> large_map, corner_map;
    unique_ptr<Chat> chat;
    unique_ptr<Notifications> notifications;
    unique_ptr<Atlas> atlas;
    unique_ptr<Skills> skills;
    shared_ptr<Entity> nearest_entity;

    InGameUI(addrtype address) : Element(address, &in_game_ui_offsets) {
        get_inventory();
        get_stash();
        get_vendor();
        get_trade();
        get_overlay_map();
        get_chat();
        get_notifications();
        get_atlas();
        get_skills();
    }

    void __new() {
        Element::__new();
        __set(L"inventory", (AhkObjRef*)*inventory, AhkObject,
              L"stash", (AhkObjRef*)*stash, AhkObject,
              L"largeMap", (AhkObjRef*)*large_map, AhkObject,
              L"cornerMap", (AhkObjRef*)*corner_map, AhkObject,
              L"chat", (AhkObjRef*)*chat, AhkObject,
              L"notifications", (AhkObjRef*)*notifications, AhkObject,
              L"atlas", (AhkObjRef*)*atlas, AhkObject,
              L"skills", (AhkObjRef*)*skills, AhkObject,
              nullptr);
    }

    bool has_active_panel() {
        return read<short>("panel_flags")
                || atlas->is_visible()
                || chat->is_opened()
                || skills->is_visible()
                || vendor->is_selected();
    }

    Inventory* get_inventory() {
        addrtype addr = read<addrtype>("inventory", "grid");
        if (!inventory || inventory->address != addr)
            inventory = unique_ptr<Inventory>(new Inventory(addr));
        return inventory.get();
    }

    Stash* get_stash() {
        addrtype addr = read<addrtype>("stash", "tabs");
        if (!stash || stash->address != addr)
            stash = unique_ptr<Stash>(new Stash(addr));
        return stash.get();
    }

    Vendor* get_vendor() {
        if (!vendor) {
            shared_ptr<Element> e = get_child(23);
            vendor = unique_ptr<Vendor>(new Vendor(e->address));
        }
        return vendor.get();
    }

    Purchase* get_purchase() {
        map<int, vector<int>> v = {{96, {11}}, {100, {6, 1, 0, 0}}, {101, {8, 1, 0, 0}}};

        purchase.reset();
        for (auto& i : v) {
            shared_ptr<Element> e = get_child(i.first);
            if (e->is_visible()) {
                if (e = e->get_child(i.second))
                    purchase = unique_ptr<Purchase>(new Purchase(e->address));
                break;
            }
        }

        return purchase.get();
    }

    Sell* get_sell() {
        map<int, vector<int>> v = {{102, {3}}, {103, {4}}};

        sell.reset();
        for (auto& i : v) {
            shared_ptr<Element> e = get_child(i.first);
            if (e->is_visible()) {
                bool reverse = e->get_child(1)->is_visible();
                if (e = e->get_child(i.second)) {
                    sell = unique_ptr<Sell>(new Sell(e->address, reverse));
                }
                break;
            }
        }

        return sell.get();
    }

    Trade* get_trade() {
        shared_ptr<Element> e = get_child({104, 3, 1, 0, 0});
        if (e) {
            trade = unique_ptr<Trade>(new Trade(e->address));
            return trade.get();
        }

        return nullptr;
    }

    OverlayMap* get_overlay_map() {
        if (!large_map) {
            large_map.reset(new OverlayMap(read<addrtype>("overlay_map", "large")));
            large_map->shift_modifier = -20.0;
            corner_map.reset(new OverlayMap(read<addrtype>("overlay_map", "small")));
            corner_map->shift_modifier = 0;
        }
        
        auto& overlay_map = large_map->is_visible() ? large_map : corner_map;
        return overlay_map.get();
    }

    Chat* get_chat() {
        if (!chat)
            chat = unique_ptr<Chat>(new Chat(read<addrtype>("chat")));
        return chat.get();
    }

    Notifications* get_notifications() {
        if (!notifications)
            notifications.reset(new Notifications(read<addrtype>("notifications")));
        return notifications.get();
    }

    Atlas* get_atlas() {
        if (!atlas)
            atlas = unique_ptr<Atlas>(new Atlas(read<addrtype>("atlas")));
        return atlas.get();
    }

    Skills* get_skills() {
        if (!skills) {
            shared_ptr<Element> e = get_child(25);
            skills = unique_ptr<Skills>(new Skills(e->address));
        }
        return skills.get();
    }

    int get_all_entities(EntityList& entities, EntityList& removed) {
        entities.swap(removed);
        entities.clear();
        addrtype root = read<addrtype>("entity_list", "root");
        int count = read<addrtype>("entity_list", "count");
        addrtype next = root;

        while (1) {
            next = PoEMemory::read<addrtype>(next);
            if (!next || next == root || entities.size() > count)
                break;

            addrtype label = PoEMemory::read<addrtype>(next + 0x10);
            bool is_visible = PoEMemory::read<byte>(label + 0x161) & 0x8;
            if (!is_visible)
                continue;

            addrtype entity_address = PoEMemory::read<addrtype>(next + 0x18);
            int entity_id = PoEMemory::read<int>(entity_address + 0x60);
            auto i = removed.find(entity_id);
            if (i != removed.end()) {
                entities.insert(*i);
                removed.erase(i);
                continue;
            }

            std::shared_ptr<Entity> entity(new Entity(entity_address));
            entity->label = shared_ptr<Element>(new Element(label));
            entities[entity_id] = entity;
        }

        return entities.size();
    }

    shared_ptr<Entity>& get_nearest_entity(LocalPlayer& player, wstring text) {
        unsigned int dist, max_dist = -1;
        EntityList entities, removed;

        get_all_entities(entities, removed);
        nearest_entity = nullptr;
        for (auto i : entities) {
            if ((dist = player.dist(*i.second)) < max_dist) {
                if (i.second->name().find(text) != -1 || i.second->path.find(text) != -1) {
                    nearest_entity = i.second;
                    max_dist = dist;
                }
            }
        }

        return nearest_entity;
    }
};
