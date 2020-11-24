#pragma once

#include <unordered_map>
#include <stdint.h>
#include <memory>
#include <chrono>
#include <vector>

#include "trace.h"
#include "transform_entity.h"

struct Item_states {
    std::vector<uint8_t> lock_states;
    std::vector<uint8_t> power_states;

    bool updated;

    Item_states() {
        lock_states.resize(16);
        power_states.resize(16);
        updated = true;
    }

    void set_lock(uint16_t id, uint8_t state) {
        int row = id / 8;
        int col = id % 8;

        // will set the bit to 1 if state is 1, will set the bit to 0 if state is 0
        lock_states[row] ^= (-state ^ lock_states[row]) & (1UL << col);

        updated = true;
    }

    void set_power(uint16_t id, uint8_t state) {
        int row = id / 8;
        int col = id % 8;

        // will set the bit to 1 if state is 1, will set the bit to 0 if state is 0
        power_states[row] ^= (-state ^ power_states[row]) & (1UL << col);

        updated = true;
    }

    void set_data(uint8_t* data) {
        memcpy(&data[0], &lock_states[0], 16);
        memcpy(&data[16], &power_states[0], 16);

        updated = false;
    }
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
    
    // All the entities in the world
    std::unordered_map<uint8_t, Transform_entity*> _players_by_index;

    std::unordered_map<uint16_t, Transform_entity*> _items_by_id;

    std::vector<std::unique_ptr<Transform_entity>> _players;

    std::vector<std::unique_ptr<Transform_entity>> _items;

    Net_game_transforms_snapshot data_transforms;

    Item_states item_states;
private:

    double _time;

    
};