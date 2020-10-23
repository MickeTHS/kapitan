#pragma once

#include "world_player_entity.h"
#include "world_device_entity.h"
#include "world_mission_entity.h"

#include <memory>
#include <stdint.h>
#include <vector>

/// <summary>
/// snapshot that contains world updates that needs to be communicated to players
/// 
/// We send the status of all:
/// - snapshot id (increments from 0) -> snapshot_id
/// - game_status enum Game_status
/// - list of active players and their status -> players
/// - locks/puzzles/doors -> devices
/// - mission updates -> missions
/// - time remaining -> milliseconds_remain
/// </summary>
struct World_snapshot {
    uint32_t snapshot_id;
    uint64_t milliseconds_remain;
    uint8_t game_status;

    std::vector<std::shared_ptr<World_player_entity>> players;
    std::vector<std::shared_ptr<World_device_entity>> devices;
    std::vector<std::shared_ptr<World_mission_entity>> missions;

    bool set_buffer(std::vector<uint8_t>& data, uint32_t offset) {
        
    }
};