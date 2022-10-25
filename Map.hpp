#pragma once

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

namespace GameLogic {

struct BoundingBox {
    BoundingBox() = default;
    BoundingBox(float low_x, float low_y, float high_x, float high_y) {
        lo_x = low_x;
        lo_y = low_y;
        hi_x = high_x;
        hi_y = high_y;
    } 
    float lo_x;
    float lo_y;
    float hi_x;
    float hi_y;
};

struct MapObject {
    MapObject(std::string& s, BoundingBox b) {
        sprite_path = s; 
        box = b;
    }
    BoundingBox box;
    std::string& sprite_path;
    // Sprite? how to draw it?
};


struct Map {
    std::vector<MapObject> map_objects;
};


}