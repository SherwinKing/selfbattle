#include "Entity.hpp"

void Animation::update(float time_elapsed) {
	if (!this->playing || !this->initialized) {
		return;
	}
	// printf("WTF: %d %f %zd %f\n", (int)this->sprite_index, this->elapsed, this->animation.size(), this->elapsed);
    this->elapsed += time_elapsed; 
	// printf("WTF2: %d %f %zd %f %f\n", (int)this->sprite_index, this->elapsed, this->animation.size(), this->elapsed, this->animation_speed);
    if (this->elapsed > this->animation_speed) {
        this->elapsed = 0.f; 
		this->sprite_index += 1;
        if (this->sprite_index == this->animation.size()) {
			this->sprite_index = 0;
			if (!this->loop) {
				this->playing = false; 
			}
        }
    }
}