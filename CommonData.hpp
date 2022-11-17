#pragma once

#include <stdint.h>
#include <vector>

constexpr float CLONE_STARTING_HEALTH = 20.f;
constexpr float PLAYER_STARTING_HEALTH = 1000.f;
constexpr float PLAYER0_STARTING_X = -100.f;
constexpr float PLAYER0_STARTING_Y = -300.f;
constexpr float PLAYER1_STARTING_X = -100.f;
constexpr float PLAYER1_STARTING_Y = 300.f;
constexpr float PLAYER_SPEED = 500.f;
constexpr float BULLET_SPEED = 300.f;
constexpr float BULLET_DAMAGE = 10.f;
constexpr float BULLET_LIFETIME = 10.f;
constexpr float BULLET_INTERVAL = 0.8f;
// Radius/width. Currently images are 100x100 so enough far away so it won't hit
// player when you click
constexpr float PLAYER_SIZE = 71.f;


constexpr float PLACE_CLONE_PHASE_DURATION = 40.f;
constexpr float FIND_CLONE_PHASE_DURATION = 30.f;
constexpr float KILL_CLONE_PHASE_DURATION = 50.f;


constexpr uint32_t NUM_CLONES = 10;


enum SPRITE : uint8_t {
    PLAYER_SPRITE_RED,
    PLAYER_SPRITE_BLUE,
    CLONE_SPRITE_RED,
    CLONE_SPRITE_BLUE,
    WALL_SPRITE,
    BULLET_SPRITE,
    FENCE_SELF_H,
    FENCE_SELF_V,
    FENCE_HALF_T,
    FENCE_HALF_R,
    FENCE_HALF_B,
    FENCE_HALF_L,
    FENCE_FULL_H,
    FENCE_FULL_V,
    FENCE_T_T,
    FENCE_T_R,
    FENCE_T_B,
    FENCE_T_L,
    FENCE_CORNER_TR,
    FENCE_CORNER_RB,
    FENCE_CORNER_BL,
    FENCE_CORNER_LT,
    CLOCK_1,
    CLOCK_2,
    CLOCK_3 
};

struct MapObject;
struct Bullet;
struct Clone;
struct ImageData;
struct Character;
typedef Clone Shadow;

// It is a singleton for now, might be changed in the future
// https://refactoring.guru/design-patterns/singleton/cpp/example
struct CommonData {
    std::vector<MapObject> map_objects;
    std::vector<Bullet> bullets;
	std::vector<Clone> clones;
    std::vector<ImageData> sprites;
    std::vector<Character> characters; // character_id is player_id
    std::vector<Shadow> shadows;

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