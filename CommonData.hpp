#pragma once

#include <stdint.h>
#include <vector>

constexpr float CLONE_STARTING_HEALTH = 20.f;
constexpr float PLAYER_STARTING_HEALTH = 1000.f;
constexpr float PLAYER0_STARTING_X = -100.f;
constexpr float PLAYER0_STARTING_Y = -300.f;
constexpr float PLAYER1_STARTING_X = -100.f;
constexpr float PLAYER1_STARTING_Y = 300.f;
constexpr float PLAYER_SPEED = 10.f;
constexpr float BULLET_SPEED = 80.f;
constexpr float BULLET_DAMAGE = 10.f;
constexpr float BULLET_LIFETIME = 10.f;
constexpr float BULLET_INTERVAL = 0.8f;
// Radius/width. Currently images are 100x100 so enough far away so it won't hit
// player when you click
constexpr float PLAYER_SIZE = 71.f;


constexpr float PLACE_CLONE_PHASE_DURATION = 20.f;
constexpr float FIND_CLONE_PHASE_DURATION = 10.f;
constexpr float KILL_CLONE_PHASE_DURATION = 30.f;


constexpr uint32_t NUM_CLONES = 2;


constexpr uint32_t NUM_SPRITES = 4;
enum SPRITE {
    PLAYER_SPRITE = 0,
    CLONE_SPRITE = 1,
    WALL_SPRITE = 2,
    BULLET_SPRITE = 3,
};

struct MapObject;
struct Bullet;
struct Clone;
struct ImageData;
struct Character;

// It is a singleton for now, might be changed in the future
// https://refactoring.guru/design-patterns/singleton/cpp/example
struct CommonData {
    std::vector<MapObject> map_objects;
    std::vector<Bullet> bullets;
	std::vector<Clone> clones;
    std::vector<ImageData> sprites;
    std::vector<Character> characters; // character_id is player_id

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
    // CommonData(CommonData &other) = delete;
    // Delete assign operator from singleton // TODO: do this
    // void operator=(const CommonData &) = delete;
    // Delete move operator from singleton
    // CommonData(CommonData&&) = delete;
};