#pragma once

#include <unordered_map>
#include <stdint.h>
#include <memory>
#include <chrono>
#include <vector>
#include <functional>

#include "trace.h"
#include "transform_entity.h"

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

enum class ItemType {
    Junction = 1,
    Switch = 2,
    Openable = 4,
    Hackable = 8,
    Breakable = 16,
    MustBePoweredForActivate = 32
};

enum class ItemState {
    Power = 1,
    Activated = 2,
    Switch_AND = 4,
    Switch_OR = 8, // require all incoming to be Activated to emit signal
};

enum class ItemPrefab {
    HingedDoor = 0,
    PoweredHingedDoor,
    DoubleSlidingDoor,
    SingleSlidingDoor,
    PulleySwitch,
    PathPuzzleSwitch,
    NumPrefabs
};

struct Scene_item {
    static std::vector<Scene_item> prefabs;

    uint16_t    id;
    uint8_t     types; // CircuitType
    uint8_t     states; // CircuitState

    uint16_t    inc[2];
    uint16_t    out[2];

    Scene_item() : id(0) {
        types = 0;
        states = 0;
        memset(inc, 0, 2 * sizeof(uint16_t));
        memset(out, 0, 2 * sizeof(uint16_t));
    }
};

struct Scene {
    std::unordered_map<uint16_t, Scene_item*> items_by_id;

    std::vector<std::unique_ptr<Scene_item>> items;

    Scene() {
        on_item_states_updated = nullptr;
    }

    bool set_item_state(uint16_t id, uint8_t state, uint8_t on) {
        if (items_by_id.find(id) == items_by_id.end()) {
            return false;
        }

        Scene_item* item = items_by_id[id];

        item->states ^= (-on ^ item->states) & (1UL << state);

        // power our outgoing items
        if (state == (uint8_t)ItemState::Activated) {
            for (int i = 0; i < 2; ++i) {
                if (item->out[i] == 0) {
                    continue;
                }
                Scene_item* out_item = items_by_id[item->out[i]];

                if (check_item_in_state_ok(out_item)) {
                    out_item->states ^= (-1 ^ out_item->states) & (1UL << (uint8_t)ItemState::Power);
                    on_item_states_updated(out_item->id, out_item->states);
                }
            }
        }
        
        on_item_states_updated(item->id, item->states);

        return true;
    }

    // perform item check when the connected items has had their state changed
    bool check_item_in_state_ok(Scene_item* item) {
        int count = 0;
        int ok_count = 0;

        for (int i = 0; i < 2; ++i) {
            if (item->inc[i] == 0) {
                continue;
            }

            count++;

            // check all incoming signals, add to ok_count for each incoming signal activated
            if (CHECK_BIT(items_by_id[item->inc[i]]->states, (uint8_t)ItemState::Activated)) {
                ok_count++;
            }
        }

        // if our state is AND, we require all switches to be activated
        if (CHECK_BIT(item->states, (uint8_t)ItemState::Switch_AND)) {
            if (ok_count == count) {
                return true;
            }
            return false;
        }

        // if our state is OR, we require ONE switch to be activated
        if (CHECK_BIT(item->states, (uint8_t)ItemState::Switch_OR)) {
            if (ok_count > 0) {
                return true;
            }

            return false;
        }

        return false;
    }

    std::function<void(uint16_t id, uint8_t states)> on_item_states_updated;
};


/// <summary>
/// A World instance is a collection of players and rules for a single game
/// In here we generate all the objects, puzzles, codes etc for a match
/// We also keep track of where all the players are so we can check
/// that they are solving a specific task at a specific place, and also
/// to broadcast out to the teammates the position of the player
/// </summary>
struct World_instance {
    World_instance();
    virtual ~World_instance();

    // add a player to the world
    void add_player(uint16_t player_id);

    // when player position has been updated
    void on_player_pos(const Net_pos& pos);

    void start();

    void update(double delta);

    double get_time() const;

    void fill_transform();

    bool set_item_state(const Net_player_set_item_state_request& request, Net_player_set_item_state_response& resp);

    void set_item_snapshot_data(uint8_t* data);

    void set_on_item_states_updated(std::function<void(uint16_t id, uint8_t states)> func);
    
    // All the entities in the world
    std::unordered_map<uint8_t, Transform_entity*> _players_by_index;

    std::unordered_map<uint16_t, Transform_entity*> _items_by_id;

    std::vector<std::unique_ptr<Transform_entity>> _players;

    std::vector<std::unique_ptr<Transform_entity>> _items;

    Net_game_transforms_snapshot data_transforms;

    Scene scene;
private:

    double _time;

    std::function<void(uint16_t, uint8_t)> _on_item_states_updated;
};