#pragma once

#include <unordered_map>
#include <stdint.h>
#include <memory>

#include "transform_entity.h"

struct Game_rules {
    uint8_t num_captains;
    uint8_t num_saboteurs_max;
    uint8_t num_players; // all players, monsters, saboteurs and captains total
    uint8_t num_monsters;

    uint8_t saboteur_min_chance;
    uint8_t saboteur_max_chance;

    uint16_t num_puzzles_per_mission_max;
    uint16_t num_mission;

    uint32_t camera_hack_cooldown;
    uint32_t door_hack_cooldown;
    uint32_t num_hack_stages;
    uint16_t player_speed_mod;
    uint16_t monster_speed_mod;
    uint8_t monster_hp;
    uint8_t player_hp;
    uint8_t saboteur_hp;
    uint8_t captain_hp;

    uint32_t monster_damage;
    uint32_t environment_damage;

    uint32_t game_time;
    uint32_t mission_max_time;
    uint32_t mission_min_time;

    uint16_t door_powered_open_time;
    uint16_t door_powered_close_time;

    uint16_t door_welding_time;
    uint16_t door_cutting_time;
    uint16_t door_pry_open_time;
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

    
    // All the entities in the world
    // maybe split players into a separate entity?
    std::unordered_map<uint16_t, std::shared_ptr<Transform_entity>> entities;

    Game_rules rules;
private:


};