#pragma once


#include "ImageRenderer.hpp"
#include <deque>


constexpr float CLONE_STARTING_HEALTH = 20.f;
constexpr float PLAYER_STARTING_HEALTH = 1000.f;
constexpr float PLAYER0_STARTING_X = -100.f;
constexpr float PLAYER0_STARTING_Y = -300.f;
constexpr float PLAYER1_STARTING_X = -100.f;
constexpr float PLAYER1_STARTING_Y = 300.f;
constexpr float PLAYER_SPEED = 20.f;
constexpr float BULLET_SPEED = 200.f;
constexpr float BULLET_DAMAGE = 10.f;
constexpr float BULLET_LIFETIME = 10.f;
constexpr float BULLET_INTERVAL = 0.8f;
// Radius/width. Currently images are 100x100 so enough far away so it won't hit
// player when you click
constexpr float PLAYER_SIZE = 71.f;


constexpr float PLACE_CLONE_PHASE_DURATION = 40.f;
constexpr float FIND_CLONE_PHASE_DURATION = 30.f;
constexpr float KILL_CLONE_PHASE_DURATION = 50.f;


constexpr uint32_t NUM_CLONES = 2;


enum SPRITE : uint8_t {
    PLAYER_SPRITE_RED,
    PLAYER_SPRITE_BLUE,
    CLONE_SPRITE_RED,
    CLONE_SPRITE_BLUE,
    WALL_SPRITE,
    BULLET_SPRITE,
    FENCE_SELF_H,
    FENCE_SELF_V,
    FENCE_HALF_T,
    FENCE_HALF_R,
    FENCE_HALF_B,
    FENCE_HALF_L,
    FENCE_FULL_H,
    FENCE_FULL_V,
    FENCE_T_T,
    FENCE_T_R,
    FENCE_T_B,
    FENCE_T_L,
    FENCE_CORNER_TR,
    FENCE_CORNER_RB,
    FENCE_CORNER_BL,
    FENCE_CORNER_LT,
    CLOCK_1,
    CLOCK_2,
    CLOCK_3 
};


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
	SPRITE sprite_index;
	int8_t player_id = -1;
	TAG tag;

	void move(float dx, float dy);
	// ImageData get_sprite();
	bool collide(Entity& other);
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
	float rot = 0; 
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
