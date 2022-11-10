#include "Entity.hpp"

void Entity::move(float dx, float dy) {
	x += dx;	
	y += dy;
}

bool Entity::collide(Entity& other) {
	// TODO: implement this
	return sqrt((x-other.x)*(x-other.x)+(y-other.y)*(y-other.y)) < 80;
	// float halfw = w / 2.f;
	// float halfh = h / 2.f;
	// box.lo_x = x - halfw; 
	// box.hi_x = x + halfw;
	// box.lo_y = y - halfh;
	// box.hi_y = y + halfh;
	// return !((other.box.hi_x < box.lo_x) || (box.hi_x < other.box.lo_x) || 
	// 		 (other.box.hi_y < box.lo_y) || (box.hi_y < other.box.lo_y)); 
}

void Character::move_character(float dx, float dy) {
	
	move(dx, dy);

	CommonData *common_data = CommonData::get_instance();
	for (auto mapobj : common_data->map_objects)
	{
		if (collide(mapobj)) {
			move(-dx, -dy);
			return;
		}
	}
}

bool Character::take_damage(float damage) {
	hp -= damage;
	return hp < 0.f;
}

bool Clone::take_damage(float damage) {
	hp -= damage;
	return hp < 0.f;
}

void Bullet::move_bullet(float elapsed) {
	this->move(elapsed * velo.x * BULLET_SPEED, elapsed * velo.y * BULLET_SPEED);	
	lifetime += elapsed;
}