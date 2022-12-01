#pragma once

#include <stdint.h>

constexpr float CLOCK_ANIMATION_SPEED = 0.5f;
constexpr float PLAYER_SHOOTING_ANIMATION_SPEED = 0.15f;
constexpr float CLONE_STARTING_HEALTH = 20.f;
constexpr float PLAYER_STARTING_HEALTH = 50.f;
constexpr float PLAYER_RESPAWN_TIME = 5.f;
/*
constexpr float PLAYER0_STARTING_X = -100.f;
constexpr float PLAYER0_STARTING_Y = -500.f;
constexpr float PLAYER1_STARTING_X = -100.f;
constexpr float PLAYER1_STARTING_Y = 500.f;
*/
// BLUE IS PLAYER 0, RED IS PLAYER 1
constexpr float PLAYER0_STARTING_X = 4000.f;
constexpr float PLAYER0_STARTING_Y = 0.f;
constexpr float PLAYER1_STARTING_X = -4000.f;
constexpr float PLAYER1_STARTING_Y = 0.f;
constexpr float BACKGROUND_X = 0.f;
constexpr float BACKGROUND_Y = 0.f;
constexpr float PLAYER_SPEED = 300.f;
constexpr float BULLET_SPEED = 250.f;
constexpr float BULLET_DAMAGE = 10.f;
constexpr float BULLET_LIFETIME = 10.f;
constexpr float BULLET_INTERVAL = 0.8f;
// Radius/width. Currently images are 100x100 so enough far away so it won't hit
// player when you click
constexpr float PLAYER_SIZE = 71.f;


// constexpr float PLACE_CLONE_PHASE_DURATION = 30.f;
// constexpr float FIND_CLONE_PHASE_DURATION = 20.f;
constexpr float PLACE_CLONE_PHASE_DURATION = 20.f;
constexpr float FIND_CLONE_PHASE_DURATION = 20.f;
constexpr float KILL_CLONE_PHASE_DURATION = 60.f;

constexpr uint32_t WINDOW_WIDTH = 1280; 
constexpr uint32_t WINDOW_HEIGHT = 720;

constexpr uint32_t NUM_CLONES = 2;
const glm::vec4 START_TEXT_COLOR(1.0f, .5f, .00f, 1.0f);
const glm::vec4 END_TEXT_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
const glm::vec4 RESPAWN_TEXT_COLOR(1.0f, 1.0f, 1.0f, 1.0f);

enum SPRITE : uint8_t {
    PLAYER_SPRITE_RED,
    PLAYER_SPRITE_BLUE,
    CLONE_SPRITE_RED,
    CLONE_SPRITE_BLUE,
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
    CLOCK_3,
    CLOCK_4,
    CLOCK_5,
    CLOCK_6,
    CLOCK_7,
    CLOCK_8,
    PLAYER_SPRITE_RELOAD_RED_1,
    PLAYER_SPRITE_RELOAD_RED_2,
    PLAYER_SPRITE_RELOAD_RED_3,
    PLAYER_SPRITE_RELOAD_RED_4,
    PLAYER_SPRITE_RELOAD_BLUE_1,
    PLAYER_SPRITE_RELOAD_BLUE_2,
    PLAYER_SPRITE_RELOAD_BLUE_3,
    PLAYER_SPRITE_RELOAD_BLUE_4,
    START,
    END,
};