#pragma once

#include "CommonData.hpp"
#include "ImageRenderer.hpp"

enum TAG {
	MAP = 0,
	CLONE_0 = 1,
	PLAYER_0 = 2,
	BULLET_0 = 3,
	CLONE_1 = 4,
	PLAYER_1 = 5,
	BULLET_1 = 6,
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
	void update_box(float dx, float dy);
};

struct Entity {
	float hp;
	BoundingBox box;
	float x;
	float y;
	ImageData *sprite;
	TAG tag;

	void set_box(uint32_t w, uint32_t h);
	void set_box(float w, float h);
	void move(float dx, float dy);
	void get_lower_left(float& lower_left_x, float& lower_left_y);
	bool collide(Entity& other);
	ImageData *get_sprite();
};

struct MapObject : Entity {
	MapObject() = default;	
	MapObject(float start_x, float start_y, ImageData *obj_sprite) {
		x = start_x;
		y = start_y;
		sprite = obj_sprite;
		tag = MAP;
		set_box(sprite->size.x, sprite->size.y);
	}
};

struct Character : Entity {
	float rot; 
    float hp = PLAYER_STARTING_HEALTH; 

	void init(float start_x, float start_y, int player_id) {
		x = start_x; 
		y = start_y;
		if (player_id == 0) {
			tag = PLAYER_0;
		}
		else {
			tag = PLAYER_1;
		}
	}

	bool take_damage(float damage);	
	void move_character(float dx, float dy);
};

struct Clone : Entity {
	float hp = CLONE_STARTING_HEALTH;

	Clone (float start_x, float start_y, ImageData *clone_sprite, int player_id) {
		x = start_x;
		y = start_y;
		sprite = clone_sprite;
		if (player_id == 0) {
			tag = CLONE_0;
		}
		else {
			tag = CLONE_1;
		}
		set_box(sprite->size.x, sprite->size.y);
	}

	bool take_damage(float damage);

};

struct Bullet : Entity {
	float lifetime = 0.f;
	glm::vec2 velo;

	Bullet (float start_x, float start_y, ImageData *bullet_sprite, glm::vec2& bullet_velo, int shooter_id) {
		velo = bullet_velo;
		x = start_x;
		y = start_y;
		sprite = bullet_sprite;
		if (shooter_id == 0) {
			tag = BULLET_0;
		}
		else {
			tag = BULLET_1;
		}
		set_box(sprite->size.x, sprite->size.y);
	}	

	void move_bullet(float elapsed);
};
