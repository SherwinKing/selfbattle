#pragma once


#include "ImageRenderer.hpp"
#include <deque>
#include "Constant.hpp"

enum TAG : uint8_t {
	MAP_TAG = 0,
	CLONE_TAG = 1,
	PLAYER_TAG = 2,
	BULLET_TAG = 3,
};

struct BoundingBox {
    BoundingBox() = default;
    float dx_left = 0;
	float dx_right = 0;
	float dy_bottom = 0;
	float dy_top = 0;
};

struct Animation {
    Animation() {
        playing = false; 
    }
	void init(const std::vector<SPRITE>& s, float speed, bool start_playing, bool looping) {
		animation = s;
		animation_speed = speed;
		initialized = true;
		playing = start_playing;
		loop = looping;
	}
    std::vector<SPRITE> animation;
    uint8_t sprite_index = 0;
    float elapsed = 0.f;
    float animation_speed = 0.f;
    bool playing = false;
	bool initialized = false;
	bool loop;

    void update(float elapsed);
};

struct Entity {
	Animation anim;
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
	int32_t score = 0;
	bool dead = false;
	float dead_timer = PLAYER_RESPAWN_TIME;
	uint32_t deaths = 0;
	uint32_t kills = 0;

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
	Bullet(float start_x, float start_y, SPRITE sprite_index, glm::vec2& bullet_velo, int shooter_id, float rotation) {
		velo = bullet_velo;
		x = start_x;
		y = start_y;
		tag = TAG::BULLET_TAG;
		this->player_id = player_id;
		this->sprite_index = sprite_index;
		this->rotation = rotation;
		this->box = {-3, 3, -3, 3};
	}	

	void move_bullet(float elapsed);
};

