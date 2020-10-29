#pragma once

#include <vector>
#include <stdint.h>

#include "trace.h"

enum struct GameSessionRule {
    None = -1,
    NumCaptains,
    NumSaboteursMax,
    NumPlayers,
    NumMonsters,
    SaboteurMinChance,
    SaboteurMaxChance,
    NumPuzzlesPerMissionMax,
    NumMissions,
    CameraHackCooldown,
    DoorHackCooldown,
    NumHackStages,
    PlayerSpeedMod,
    MonsterSpeedMod,
    MonsterHP,
    PlayerHP,
    SaboteurHP,
    CaptainHP,
    MonsterDamage,
    EnvironmentDamage,
    GameTime,
    MissionMaxTime,
    MissionMinTime,
    DoorPoweredOpenTime,
    DoorPoweredCloseTime,
    DoorWeldingTime,
    DoorCuttingTime,
    DoorPryOpenTime,
    NumRules
};

struct Net_session_rules {
    std::vector<int32_t> rules;

    uint8_t  num_captains;
    uint8_t  num_saboteurs_max;
    uint8_t  num_players; // all players, monsters, saboteurs and captains total
    uint8_t  num_monsters;

    uint8_t  saboteur_min_chance;
    uint8_t  saboteur_max_chance;

    uint16_t num_puzzles_per_mission_max;
    uint16_t num_mission;

    uint32_t camera_hack_cooldown;
    uint32_t door_hack_cooldown;
    uint32_t num_hack_stages;
    uint16_t player_speed_mod;
    uint16_t monster_speed_mod;
    uint8_t  monster_hp;
    uint8_t  player_hp;
    uint8_t  saboteur_hp;
    uint8_t  captain_hp;

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

    Net_session_rules() {
        uint8_t num_rules = (uint8_t)GameSessionRule::NumRules;

        rules.resize(num_rules);

        for (uint8_t i = 0; i < num_rules; ++i) {
            set_rule(i, 0);
        }
    }

    void set_rule(uint8_t rule_int, int32_t value) {
        GameSessionRule rule = (GameSessionRule)rule_int;

        switch (rule) {
            case GameSessionRule::NumCaptains: 
                num_captains = value;
                break;
            case GameSessionRule::NumSaboteursMax: 
                num_saboteurs_max = value;
                break;
            case GameSessionRule::NumPlayers: 
                num_players = value;
                break;
            case GameSessionRule::NumMonsters: 
                num_monsters = value;
                break;
            case GameSessionRule::SaboteurMinChance: 
                saboteur_min_chance = value;
                break;
            case GameSessionRule::SaboteurMaxChance: 
                saboteur_max_chance = value;
                break;
            case GameSessionRule::NumPuzzlesPerMissionMax: 
                num_puzzles_per_mission_max = value;
                break;
            case GameSessionRule::NumMissions: 
                num_mission = value;
                break;
            case GameSessionRule::CameraHackCooldown: 
                camera_hack_cooldown = value;
                break;
            case GameSessionRule::DoorHackCooldown: 
                door_hack_cooldown = value;
                break;
            case GameSessionRule::NumHackStages: 
                num_hack_stages = value;
                break;
            case GameSessionRule::PlayerSpeedMod: 
                player_speed_mod = value;
                break;
            case GameSessionRule::MonsterSpeedMod: 
                monster_speed_mod = value;
                break;
            case GameSessionRule::MonsterHP: 
                monster_hp = value;
                break;
            case GameSessionRule::PlayerHP: 
                player_hp = value;
                break;
            case GameSessionRule::SaboteurHP: 
                saboteur_hp = value;
                break;
            case GameSessionRule::CaptainHP: 
                captain_hp = value;
                break;
            case GameSessionRule::MonsterDamage: 
                monster_damage = value;
                break;
            case GameSessionRule::EnvironmentDamage: 
                environment_damage = value;
                break;
            case GameSessionRule::GameTime: 
                game_time = value;
                break;
            case GameSessionRule::MissionMaxTime: 
                mission_max_time = value;
                break;
            case GameSessionRule::MissionMinTime: 
                mission_min_time = value;
                break;
            case GameSessionRule::DoorPoweredOpenTime: 
                door_powered_open_time = value;
                break;
            case GameSessionRule::DoorPoweredCloseTime: 
                door_powered_close_time = value;
                break;
            case GameSessionRule::DoorWeldingTime: 
                door_welding_time = value;
                break;
            case GameSessionRule::DoorCuttingTime: 
                door_cutting_time = value;
                break;
            case GameSessionRule::DoorPryOpenTime: 
                door_pry_open_time = value;
                break;
            default:
                TRACE("ERROR: Undefined rule index: %d\n", rule_int);
                break;
        }
    }
};
