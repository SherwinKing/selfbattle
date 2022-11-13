#pragma once

#include "CommonData.hpp"
#include "ImageRenderer.hpp"
#include <deque>

enum TAG : uint8_t {
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
	float x;
	float y;
	float rotation = 0; 
	SPRITE sprite_index;
	int8_t player_id = -1;
	TAG tag;

	void move(float dx, float dy);
	// ImageData get_sprite();
	bool collide(Entity& other);
};

struct MapObject : Entity {
	MapObject() = default;	
	MapObject(float start_x, float start_y, SPRITE sprite_index) {
		x = start_x;
		y = start_y;
		this->sprite_index = sprite_index;
		tag = TAG::MAP_TAG;
	}
};

struct CharacterSnapshot {
	float x=0;
	float y=0;
	float timestamp=0;
	CharacterSnapshot() = default;
	CharacterSnapshot(float x, float y, float timestamp) {
		this->x = x;
		this->y = y;
		this->timestamp = timestamp;
	}
};

struct Character : Entity {
    float hp = PLAYER_STARTING_HEALTH;
	uint8_t player_id;
	uint32_t score = 0;

	// Phase1 (Place clones) replay buffer
	std::deque<CharacterSnapshot> phase1_replay_buffer;
	std::deque<CharacterSnapshot> phase1_replay_buffer_2;

	Character() = default;
	Character(float start_x, float start_y, SPRITE sprite_index, int player_id) {
		x = start_x; 
		y = start_y;
		tag = TAG::PLAYER_TAG;
		this->player_id = static_cast<uint8_t>(player_id);
		this->sprite_index = sprite_index;
	}

	bool take_damage(float damage);	
	void move_character(float dx, float dy);
};

struct Clone : Entity {
	float hp = CLONE_STARTING_HEALTH;

	Clone() = default;
	Clone(float start_x, float start_y, SPRITE sprite_index, int player_id) {
		x = start_x;
		y = start_y;
		tag = TAG::CLONE_TAG;
		this->player_id = static_cast<int8_t>(player_id);
		this->sprite_index = sprite_index;
	}

	bool take_damage(float damage);

};

struct Bullet : Entity {
	float lifetime = 0.f;
	glm::vec2 velo;
	bool active = true;

	Bullet() = default;
	Bullet(float start_x, float start_y, SPRITE sprite_index, glm::vec2& bullet_velo, int shooter_id) {
		velo = bullet_velo;
		x = start_x;
		y = start_y;
		tag = TAG::BULLET_TAG;
		this->player_id = player_id;
		this->sprite_index = sprite_index;
	}	

	void move_bullet(float elapsed);
};
