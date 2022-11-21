#include "Animation.hpp"

void Animation::update(float time_elapsed) {
    this->elapsed += time_elapsed; 
    if (this->elapsed > this->animation_speed) {
        this->elapsed = 0; 
        if (++(this->current_sprite_ind) == this->animation.size()) {
            this->playing = false; 
        }
    }
}