#pragma once

#include <stdint.h>
#include <vector>

constexpr float CLONE_STARTING_HEALTH = 50.f;
constexpr float PLAYER_STARTING_HEALTH = 100.f;
constexpr float PLAYER1_STARTING_X = 0.f;
constexpr float PLAYER1_STARTING_Y = 0.f;
constexpr float PLAYER2_STARTING_X = 0.f;
constexpr float PLAYER2_STARTING_Y = 0.f;
constexpr float PLAYER_SPEED = 10.f;
constexpr float BULLET_SPEED = 80.f;
constexpr float BULLET_DAMAGE = 10.f;
constexpr float BULLET_LIFETIME = 10.f;
// Radius/width. Currently images are 100x100 so enough far away so it won't hit
// player when you click
constexpr float PLAYER_SIZE = 71.f;


constexpr float PLACE_CLONE_PHASE_DURATION = 30.f;
constexpr float FIND_CLONE_PHASE_DURATION = 15.f;
constexpr float KILL_CLONE_PHASE_DURATION = 30.f;


constexpr uint32_t NUM_CLONES = 2;

struct MapObject;
struct Bullet;
struct Clone;

struct CommonData {
    std::vector<MapObject> map_objects;
    std::vector<Bullet> bullets;
	std::vector<Clone> clones;
	std::vector<Clone> enemy_clones;
};