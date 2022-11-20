#pragma once

#include "Entity.hpp"

struct MapObject : Entity {
	MapObject() = default;	
	MapObject(float start_x, float start_y, SPRITE sprite_index) {
		x = start_x;
		y = start_y;
		this->sprite_index = sprite_index;
		tag = TAG::MAP_TAG;
	}
};
