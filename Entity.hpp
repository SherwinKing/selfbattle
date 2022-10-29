#pragma once

#include "CommonData.hpp"
#include "ImageRenderer.hpp"

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
		set_box(sprite->size.x, sprite->size.y);
	}
};

struct Character : Entity {
	void init(float start_x, float start_y) {
		x = start_x; 
		y = start_y;
	}

	bool take_damage(float damage);

	float rot; 
    float hp = PLAYER_STARTING_HEALTH; 
	
};

struct Clone : Entity {
	Clone (float start_x, float start_y, ImageData *clone_sprite) {
		x = start_x;
		y = start_y;
		sprite = clone_sprite;
		set_box(sprite->size.x, sprite->size.y);
	}

	bool take_damage(float damage);

	float hp = CLONE_STARTING_HEALTH;	
};

struct Bullet : Entity {
	Bullet (float start_x, float start_y, ImageData *bullet_sprite, glm::vec2& bullet_velo) {
		velo = bullet_velo;
		x = start_x;
		y = start_y;
		sprite = bullet_sprite;
		set_box(sprite->size.x, sprite->size.y);
	}	

	void move_bullet(float elapsed);
	float lifetime = 0.f;
	glm::vec2 velo;
};
