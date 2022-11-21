#pragma once

#include "CommonData.hpp"

#include <stdint.h>
#include <vector>

struct Animation {
    Animation() {
        playing = false; 
    }
    Animation(const std::vector<SPRITE>& s, float speed) {
        animation = s;
        elapsed = 0.f;
        animation_speed = speed;
        current_sprite_ind = 0;
        playing = false;
    }
    std::vector<SPRITE> animation;
    uint8_t current_sprite_ind;
    float elapsed;
    float animation_speed;
    bool playing;

    void update(float elapsed);
};