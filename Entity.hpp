#pragma once

#include "CommonData.hpp"
#include "ImageRenderer.hpp"

enum TAG {
	MAP_TAG = 0,
	CLONE0_TAG = 1,
	PLAYER0_TAG = 2,
	BULLET0_TAG = 3,
	CLONE1_TAG = 4,
	PLAYER1_TAG = 5,
	BULLET1_TAG = 6,
};

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

struct Entity {
	BoundingBox box;
	float x;
	float y;
	uint8_t sprite_index;
	TAG tag;

	void move(float dx, float dy);
	void get_lower_left(float& lower_left_x, float& lower_left_y);
	// ImageData get_sprite();
	bool collide(Entity& other);
};

struct MapObject : Entity {
	MapObject() = default;	
	MapObject(float start_x, float start_y, uint8_t sprite_index) {
		x = start_x;
		y = start_y;
		this->sprite_index = sprite_index;
		tag = MAP_TAG;
	}
};

struct Character : Entity {
	float rot = 0; 
    float hp = PLAYER_STARTING_HEALTH;
	uint8_t player_id;

	Character() = default;
	Character(float start_x, float start_y, uint8_t sprite_index, int player_id) {
		x = start_x; 
		y = start_y;
		if (player_id == 0) {
			tag = PLAYER0_TAG;
		}
		else {
			tag = PLAYER1_TAG;
		}
		this->player_id = player_id;
		this->sprite_index = sprite_index;
	}

	bool take_damage(float damage);	
	void move_character(float dx, float dy);
};

struct Clone : Entity {
	float hp = CLONE_STARTING_HEALTH;

	Clone() = default;
	Clone(float start_x, float start_y, uint8_t sprite_index, int player_id) {
		x = start_x;
		y = start_y;
		if (player_id == 0) {
			tag = CLONE0_TAG;
		}
		else {
			tag = CLONE1_TAG;
		}
		this->sprite_index = sprite_index;
	}

	bool take_damage(float damage);

};

struct Bullet : Entity {
	float lifetime = 0.f;
	glm::vec2 velo;
	Bullet() = default;
	Bullet(float start_x, float start_y, uint8_t sprite_index, glm::vec2& bullet_velo, int shooter_id) {
		velo = bullet_velo;
		x = start_x;
		y = start_y;
		if (shooter_id == 0) {
			tag = BULLET0_TAG;
		}
		else {
			tag = BULLET1_TAG;
		}
		this->sprite_index = sprite_index;
	}	

	void move_bullet(float elapsed);
};
