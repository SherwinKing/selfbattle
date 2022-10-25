#pragma once


#include "Map.hpp"
#include "Scene.hpp"
#include "Sound.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"

#include <glm/glm.hpp>
#include <stdint.h>
#include <list>
#include <vector>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <cassert>
#include <random>

#include <string>
#include <iostream>
#include <fstream>

///TODO:
// 2nd element is whatever we need for drawing sprites @SHERWIN
// load_png gave us a std::vector< glm::u8vec4 > so using that for now
#define SPRITE_DATA std::vector< glm::u8vec4 >

namespace GameLogic {

constexpr float CLONE_STARTING_HEALTH = 1000.f;
constexpr float PLAYER_STARTING_HEALTH = 100.f;
constexpr float PLAYER1_STARTING_X = 0.f;
constexpr float PLAYER1_STARTING_Y = 0.f;
constexpr float PLAYER2_STARTING_X = 0.f;
constexpr float PLAYER2_STARTING_Y = 0.f;

constexpr uint32_t NUM_SPRITES = 3;
std::pair<const char *, const char *> SPRITES[] = {
    std::pair("player", "s"),
    std::pair("wall", "sounds/3.opus"),
    std::pair("door", "sounds/3.opus")
};

struct Sprite {
    uint32_t w;
    uint32_t h;
    SPRITE_DATA data;
    glm::uvec2 wh;
    
    uint32_t GetW() {
        return wh.x;
    }
    uint32_t GetH() {
        return wh.y;
    }
    
};

struct PlayerState {
    PlayerState() {
        // Load sprites for the player state
        for (size_t i = 0; i < NUM_SPRITES; ++i) {
            const auto& p = SPRITES[i];
            Sprite s;
            load_png(data_path(std::string(p.second)), &s.wh, &s.data, LowerLeftOrigin); 
            sprites.emplace(p.first, s);
        }
    } 
    
    Map map;
    ///TODO:
    // 2nd element is whatever we need for drawing sprites @SHERWIN
    // load_png gave us a std::vector< glm::u8vec4 > so using that for now
    std::unordered_map<std::string, Sprite> sprites;

    Player self; 
    Player opponent;
};

struct Player {
    Player() = default;
    Player(float start_x, float start_y) {
        hp = PLAYER_STARTING_HEALTH;
        x = start_x;
        y = start_y;
    }
    // in radians from positive x (like a unit circle)
    // used to know where the player mouse is pointing right now
    float rotation; 
    float hp; 
    float x;
    float y;

    void update(float new_x, float new_y, float new_hp, float new_rotation) {
        x = new_x; 
        y = new_y;
        hp = new_hp;
        rotation = new_rotation;
    }

};

struct Clone {
    Clone(float start_x, float start_y) {
        hp = CLONE_STARTING_HEALTH;
        x = start_x;
        y = start_y;
    }
    float hp;
    float x;
    float y;
};


struct ServerState {

    ServerState() {

    }

    Player player1; 
    Player player1;

    
    
    private:
};

}
