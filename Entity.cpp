#include "Entity.hpp"
#include "CommonData.hpp"

void Entity::move(float dx, float dy) {
	x += dx;	
	y += dy;
}

bool Entity::collide(Entity& other) {
	// // TODO: implement this
	// return sqrt((x-other.x)*(x-other.x)+(y-other.y)*(y-other.y)) < 80;

	// Ref: https://stackoverflow.com/questions/20925818/algorithm-to-check-if-two-boxes-overlap
	float xmax = x + box.dx_right;
	float xmin = x + box.dx_left;
	float ymax = y + box.dy_top;
	float ymin = y + box.dy_bottom;
	// float xmax = x + box.width / 2;
	// float xmin = x - box.width / 2;
	// float ymax = y + box.height / 2;
	// float ymin = y - box.height / 2;

	float other_xmax = other.x + other.box.dx_right;
	float other_xmin = other.x + other.box.dx_left;
	float other_ymax = other.y + other.box.dy_top;
	float other_ymin = other.y + other.box.dy_bottom;

	bool if_box_collide = (xmax > other_xmin && xmin < other_xmax) && (ymax > other_ymin && ymin < other_ymax);
	return if_box_collide;

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
	
	int section_id = common_data->map.get_section_id(x, y);
	for (auto mapobj : common_data->map.sections[section_id])
	{
		if (collide(mapobj)) {
			move(-dx, -dy);
			return;
		}
	}
}

bool Character::take_damage(float damage) {
	hp -= damage;
	if (hp <= 0.f) {
		hp = 0.f;	
		dead = true;
	}
	return hp <= 0.f;
}

bool Clone::take_damage(float damage) {
	hp -= damage;
	return hp <= 0.f;
}

void Bullet::move_bullet(float elapsed) {
	this->move(elapsed * velo.x, elapsed * velo.y);	
	lifetime += elapsed;
}