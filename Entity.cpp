#include "Entity.hpp"

void BoundingBox::update_box(float dx, float dy) {
	lo_x += dx;	
	hi_x += dx;
	lo_y += dy;
	hi_y += dy;
}

void Entity::set_box(uint32_t w, uint32_t h) {
	set_box(static_cast<float>(w), static_cast<float>(h));
}

void Entity::set_box(float w, float h) {
	float halfw = w / 2.f;
	float halfh = h / 2.f;
	box.lo_x = x - halfw; 
	box.hi_x = x + halfw;
	box.lo_y = y - halfh;
	box.hi_y = y + halfh;
}

void Entity::move(float dx, float dy) {
	x += dx;	
	y += dy;
	box.update_box(dx, dy);
}

void Entity::get_lower_left(float& lower_left_x, float& lower_left_y) {
	lower_left_x = box.lo_x;	
	lower_left_y = box.lo_y;
}

bool Entity::collide(Entity& other) {
	return !((other.box.hi_x < box.lo_x) || (box.hi_x < other.box.lo_x) || 
			 (other.box.hi_y < box.lo_y) || (box.hi_y < other.box.lo_y)); 
}

void Bullet::move_bullet(float elapsed) {
	this->move(elapsed * velo.x, elapsed * velo.y);	
	lifetime += elapsed;
}