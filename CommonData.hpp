#pragma once

#include <stdint.h>
#include <vector>

#include "Constant.hpp"
#include "Map.hpp"

struct Bullet;
struct Clone;
struct ImageData;
struct Character;
typedef Clone Shadow;

// It is a singleton for now, might be changed in the future
// https://refactoring.guru/design-patterns/singleton/cpp/example
struct CommonData {
    Map map;
    
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