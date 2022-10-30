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


constexpr uint32_t NUM_SPRITES = 4;
enum SPRITE {
    player0 = 0,
    clone = 1,
    wall = 2,
    bullet = 3,
};

struct MapObject;
struct Bullet;
struct Clone;
struct ImageData;

// It is a singleton for now, might be changed in the future
// https://refactoring.guru/design-patterns/singleton/cpp/example
struct CommonData {
    std::vector<MapObject> map_objects;
    std::vector<Bullet> bullets;
	std::vector<Clone> clones;
	std::vector<Clone> enemy_clones;
    std::vector<ImageData> sprites;

    CommonData() {

    }

    static CommonData *get_instance() {
        static CommonData *_instance = nullptr;
        if(_instance == nullptr){
            _instance = new CommonData();
        }
        return _instance;
    }

    // Delete clone operator from singleton
    CommonData(CommonData &other) = delete;
    // Delete assigne operator from singleton
    void operator=(const CommonData &) = delete;
    // Delete move operator from singleton
    CommonData(CommonData&&) = delete;
};