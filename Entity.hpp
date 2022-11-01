#pragma once

#include "CommonData.hpp"
#include "ImageRenderer.hpp"

enum TAG {
	MAP_TAG = 0,
	CLONE_TAG = 1,
	PLAYER_TAG = 2,
	BULLET_TAG = 3,
};

struct BoundingBox {
    BoundingBox() = default;
    float width = 0;
	float height = 0;
};

struct Entity {
	BoundingBox box;
	bool is_invisible = false;
	bool is_removed = false;
	float x;
	float y;
	uint8_t sprite_index;
	uint8_t player_id = -1;
	TAG tag;

	void move(float dx, float dy);
	// ImageData get_sprite();
	bool collide(Entity& other);

	void remove();
};

struct MapObject : Entity {
	MapObject() = default;	
	MapObject(float start_x, float start_y, uint8_t sprite_index) {
		x = start_x;
		y = start_y;
		this->sprite_index = sprite_index;
		tag = TAG::MAP_TAG;
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
		tag = TAG::PLAYER_TAG;
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
		tag = TAG::CLONE_TAG;
		this->player_id = player_id;
		this->sprite_index = sprite_index;
	}

	bool take_damage(float damage);

};

struct Bullet : Entity {
	float lifetime = 0.f;
	glm::vec2 velo;
	bool active = true;

	Bullet() = default;
	Bullet(float start_x, float start_y, uint8_t sprite_index, glm::vec2& bullet_velo, int shooter_id) {
		velo = bullet_velo;
		x = start_x;
		y = start_y;
		tag = TAG::BULLET_TAG;
		this->player_id = player_id;
		this->sprite_index = sprite_index;
	}	

	void move_bullet(float elapsed);
};
