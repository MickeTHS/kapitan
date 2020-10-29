#pragma once

#include <unordered_map>
#include <stdint.h>
#include <memory>

#include "trace.h"
#include "transform_entity.h"

struct Net_session_rules;

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

    
    // All the entities in the world
    // maybe split players into a separate entity?
    std::unordered_map<uint16_t, std::shared_ptr<Transform_entity>> entities;

    Net_session_rules* rules;
private:


};