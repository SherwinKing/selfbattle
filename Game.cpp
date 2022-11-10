#include "Game.hpp"

#include "Connection.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>

#include <glm/gtx/norm.hpp>


//-----------------------------------------

Game::Game() : mt(0x15466666) {
}

void Player::set_position(float new_x, float new_y) {
	c.x = new_x;
	c.y = new_y;
}

const ImageData& Player::get_player_sprite() {
	///TODO: Do stuff with rotations later
	return sprites["player0"];
}

bool Player::recv_message(Connection *connection){
	return true;
}

void Player::world_to_opengl(float world_x, float world_y, glm::uvec2 const &screen_size, float& screen_x, float& screen_y) {
	float w = static_cast<float>(screen_size.x);
	float h = static_cast<float>(screen_size.y);

	// Between -1 and 1	
	screen_x = ((world_x - c.x) / w) * 2.f;	
	screen_y = ((world_y - c.y) / h) * 2.f;
}

void Player::screen_to_world(float screen_x, float screen_y, glm::uvec2 const &screen_size, float& world_x, float& world_y) {
	float w = static_cast<float>(screen_size.x);
	float h = static_cast<float>(screen_size.y);

	float center_x = w / 2.f;
	float center_y = h / 2.f;
	world_x = c.x + (screen_x - center_x); 
	world_y = c.y - (screen_y - center_y);
}

bool Entity::collide(Entity& other) {
	return !((other.box.hi_x < box.lo_x) || (box.hi_x < other.box.lo_x) || 
			 (other.box.hi_y < box.lo_y) || (box.hi_y < other.box.lo_y)); 
}

void Entity::set_box(uint w, uint h) {
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

void BoundingBox::update_box(float dx, float dy) {
	lo_x += dx;	
	hi_x += dx;
	lo_y += dy;
	hi_y += dy;
}

void Player::move_player(float dx, float dy) {
	c.move(dx, dy);
	for (auto mapobj : map->map_objects)
	{
		if (c.collide(*mapobj)) {
			c.move(-dx, -dy);	
			return;
		}
	}
	for (auto clone : clones) {
		if (c.collide(*clone)) {
			c.move(-dx, -dy);
			return;
		}
	}	
	for (auto clone : enemy_clones) {
		if (c.collide(*clone)) {
			c.move(-dx, -dy);
			return;
		}
	}
}

Player *Game::spawn_player() {
	players.emplace_back();
	Player &player = players.back();

	return &player;
}


void Player::draw(glm::uvec2 const &screen_size) {
	renderer.resize(screen_size.x, screen_size.y);
	float screen_x;
	float screen_y;
	float lower_x;
	float lower_y;

	const ImageData& player_sprite = get_player_sprite();	

	c.get_lower_left(lower_x, lower_y);
	world_to_opengl(lower_x, lower_y, screen_size, screen_x, screen_y);	
	renderer.render_image(player_sprite, screen_x, screen_y);

	for (auto bullet : bullets) {
		bullet->get_lower_left(lower_x, lower_y);	
		world_to_opengl(lower_x, lower_y ,screen_size, screen_x, screen_y);	
		renderer.render_image(*(bullet->sprite), screen_x, screen_y);
	}

	for (auto clone : clones) {
		clone->get_lower_left(lower_x, lower_y);	
		world_to_opengl(lower_x, lower_y ,screen_size, screen_x, screen_y);	
		renderer.render_image(*(clone->sprite), screen_x, screen_y);
	}

	for (auto mapobj : map->map_objects) {
		mapobj->get_lower_left(lower_x, lower_y);	
		world_to_opengl(lower_x, lower_y ,screen_size, screen_x, screen_y);	
		renderer.render_image(*(mapobj->sprite), screen_x, screen_y);
	}
}

void Entity::get_lower_left(float& lower_left_x, float& lower_left_y) {
	lower_left_x = box.lo_x;	
	lower_left_y = box.lo_y;
}

void Bullet::move_bullet(float elapsed) {
	this->move(elapsed * velo.x, elapsed * velo.y);	
	lifetime += elapsed;
}

void Player::shoot (float screen_x, float screen_y, glm::uvec2 const &window_size) {
	float world_x, world_y;
	screen_to_world(screen_x, screen_y, window_size, world_x, world_y);		

	glm::vec2 shoot_velo;
	shoot_velo.x = world_x - c.x;
	shoot_velo.y = world_y - c.y;
	shoot_velo = glm::normalize(shoot_velo) * BULLET_SPEED;

	ImageData *bullet_sprite = &(sprites["bullet"]);
	std::shared_ptr<Bullet> bullet = std::make_shared<Bullet>(c.x, c.y, bullet_sprite, shoot_velo); 
	bullets.emplace_back(bullet);	
	float amount_to_move = static_cast<float>(static_cast<uint32_t>(PLAYER_SIZE / BULLET_SPEED) + 1);

	bullet->move_bullet(amount_to_move);
} 

void Player::place_clone(float screen_x, float screen_y, glm::uvec2 const &window_size) {
	float world_x, world_y;
	screen_to_world(screen_x, screen_y, window_size, world_x, world_y);	

	ImageData *clone_sprite = &(sprites["clone"]);
	std::shared_ptr<Clone> clone = std::make_shared<Clone>(world_x, world_y, clone_sprite); 
	
	clones.emplace_back(clone);	
}

void Game::remove_player(Player *player) {
	bool found = false;
	for (auto pi = players.begin(); pi != players.end(); ++pi) {
		if (&*pi == player) {
			players.erase(pi);
			found = true;
			break;
		}
	}
	assert(found);
}

bool Character::take_damage(float damage) {
	hp -= damage;
	return hp < 0.f;
}

bool Clone::take_damage(float damage) {
	hp -= damage;
	return hp < 0.f;
}

std::shared_ptr<Map> Player::create_map() {
	std::shared_ptr<Map> new_map = std::make_shared<Map>();	
	std::vector<std::shared_ptr<MapObject>>& objs = new_map->map_objects;
	ImageData *rw1 = &(sprites["redwall1"]);
	ImageData *rw2 = &(sprites["redwall2"]);
	ImageData *rw3 = &(sprites["redwall2"]);

	ImageData *fh = &(sprites["fence_horizontal"]);
	ImageData *fv = &(sprites["fence_vertical"]);
	ImageData *fc1 = &(sprites["fence_corner1"]);
	ImageData *fc2 = &(sprites["fence_corner2"]);
	// ImageData *fc3 = &(sprites["fence_corner3"]);
	// ImageData *fc4 = &(sprites["fence_corner4"]);
	struct wp {
		wp(float x, float y, ImageData *id) {
			this->x = x;
			this->y = y;
			this->id = id;
		}
		float x;
		float y;
		ImageData *id;
	};
	std::vector<wp> walls = {
		// RED
		// 2
		wp(600.f, -200.f, rw3),
		wp(400.f, -200.f, rw1),
		wp(200.f, -200.f, rw2),
		wp(0.f, -200.f, rw3),

		// 3
		wp(800.f, 0.f, rw3),

		// 4
		wp(600.f, 200.f, rw3),
		wp(400.f, 200.f, rw1),
		wp(200.f, 200.f, rw2),
		wp(0.f, 200.f, rw3),

		// 1
		wp(600.f, -800.f, rw3),
		wp(400.f, -800.f, rw1),
		wp(200.f, -800.f, rw2),
		wp(0.f, -800.f, rw3),

		// 5
		wp(600.f, 800.f, rw3),
		wp(400.f, 800.f, rw1),
		wp(200.f, 800.f, rw2),
		wp(0.f, 800.f, rw3),

		// 6
		wp(800.f, 1000.f, rw3),
		wp(800.f, 1200.f, rw1),

		// 11
		wp(800.f, -1000.f, rw3),
		wp(800.f, -1200.f, rw1),
		wp(800.f, -1400.f, rw2),
		wp(800.f, -1600.f, rw3),

		// 12
		wp(1000.f, -1800.f, rw3),
		wp(1200.f, -1800.f, rw3),
		wp(1400.f, -1800.f, rw3),
		wp(1600.f, -1800.f, rw3),
		wp(1800.f, -1800.f, rw3),
		wp(2000.f, -1800.f, rw3),
		wp(2200.f, -1800.f, rw3),
		wp(2400.f, -1800.f, rw3),
		wp(2600.f, -1800.f, rw3),
		wp(2800.f, -1800.f, rw3),
		wp(3000.f, -1800.f, rw3),
		wp(3200.f, -1800.f, rw3),
		wp(3400.f, -1800.f, rw3),
		wp(3600.f, -1800.f, rw3),
		wp(3800.f, -1800.f, rw3),
		wp(4000.f, -1800.f, rw3),
		wp(4200.f, -1800.f, rw3),
		wp(4400.f, -1800.f, rw3),
		wp(4600.f, -1800.f, rw3),
		wp(4800.f, -1800.f, rw3),
		wp(5000.f, -1800.f, rw3),

		// 13
		wp(1000.f, 1400.f, rw3),
		wp(1200.f, 1400.f, rw3),
		wp(1400.f, 1400.f, rw3),
		wp(1600.f, 1400.f, rw3),
		wp(1800.f, 1400.f, rw3),
		wp(2000.f, 1400.f, rw3),
		wp(2200.f, 1400.f, rw3),
		wp(2400.f, 1400.f, rw3),
		wp(2600.f, 1400.f, rw3),
		wp(2800.f, 1400.f, rw3),
		wp(3000.f, 1400.f, rw3),
		wp(3200.f, 1400.f, rw3),
		wp(3400.f, 1400.f, rw3),
		wp(3600.f, 1400.f, rw3),
		wp(3800.f, 1400.f, rw3),
		wp(4000.f, 1400.f, rw3),
		wp(4200.f, 1400.f, rw3),
		wp(4400.f, 1400.f, rw3),
		wp(4600.f, 1400.f, rw3),
		wp(4800.f, 1400.f, rw3),
		wp(5000.f, 1400.f, rw3),

		// 15
		wp(2400.f, 1200.f, rw3),
		wp(2400.f, 1000.f, rw3),
		wp(2400.f, 800.f, rw3),
		wp(2400.f, 600.f, rw3),

		// 8
		wp(1800.f, 1000.f, rw3),
		wp(1800.f, 800.f, rw3),
		wp(1800.f, 600.f, rw3),
		wp(1800.f, 400.f, rw3),
		wp(1800.f, 200.f, rw3),
		wp(1800.f, 0.f, rw3),
		wp(1800.f, -200.f, rw3),
		wp(1800.f, -400.f, rw3),
		wp(1800.f, -600.f, rw3),
		wp(1800.f, -800.f, rw3),
		wp(1800.f, -1000.f, rw3),

		// 23
		wp(2000.f, -1000.f, rw3),
		wp(2200.f, -1000.f, rw3),
		wp(2400.f, -1000.f, rw3),
		wp(2600.f, -1000.f, rw3),
		wp(2800.f, -1000.f, rw3),

		// 20
		wp(2400.f, -400.f, rw3),
		wp(2400.f, -200.f, rw3),

		// 25
		wp(2400.f, -1600.f, rw3),
		wp(2400.f, -1400.f, rw3),

		// 17
		wp(3000.f, 0.f, rw3),
		wp(3000.f, 200.f, rw3),
		wp(3000.f, 400.f, rw3),
		wp(3000.f, 600.f, rw3),
		wp(3000.f, 800.f, rw3),
		wp(3000.f, 800.f, rw3),
		wp(3000.f, 1000.f, rw3),

		// 16
		wp(3200.f, 1000.f, rw3),
		wp(3400.f, 1000.f, rw3),
		wp(3600.f, 1000.f, rw3),

		// 18
		wp(4000.f, 1200.f, rw3),
		wp(4000.f, 1000.f, rw3),
		wp(4000.f, 800.f, rw3),
		wp(4000.f, 600.f, rw3),

		// 19
		wp(3800.f, 600.f, rw3),
		wp(3600.f, 600.f, rw3),

		// 21
		wp(3600.f, 0.f, rw3),
		wp(3600.f, -200.f, rw3),
		wp(3600.f, -400.f, rw3),
		wp(3600.f, -600.f, rw3),
		wp(3600.f, -800.f, rw3),
		wp(3600.f, -1000.f, rw3),

		// 22
		wp(3400.f, -600.f, rw3),

		// 23
		wp(4200.f, 400.f, rw3),
		wp(4200.f, -100.f, rw3),
		wp(4200.f, -600.f, rw3),
		wp(4200.f, -1100.f, rw3),
		wp(4800.f, 400.f, rw3),
		wp(4800.f, -100.f, rw3),
		wp(4800.f, -600.f, rw3),
		wp(4800.f, -1100.f, rw3),

		// 24
		wp(5200.f, 1400.f, rw3),
		wp(5200.f, 1200.f, rw3),
		wp(5200.f, 1000.f, rw3),
		wp(5200.f, 800.f, rw3),
		wp(5200.f, 600.f, rw3),
		wp(5200.f, 400.f, rw3),
		wp(5200.f, 200.f, rw3),
		wp(5200.f, 00.f, rw3),
		wp(5200.f, -200.f, rw3),
		wp(5200.f, -400.f, rw3),
		wp(5200.f, -600.f, rw3),
		wp(5200.f, -800.f, rw3),
		wp(5200.f, -1000.f, rw3),
		wp(5200.f, -1200.f, rw3),
		wp(5200.f, -1400.f, rw3),
		wp(5200.f, -1600.f, rw3),
		wp(5200.f, -1800.f, rw3),


		// BLUE
		// 2
		wp(-600.f, -200.f, fc1),
		wp(-400.f, -200.f, fh),
		wp(-200.f, -200.f, fh),

		// 10
		wp(-600.f, 0.f, fv),

		// 4
		wp(-600.f, 200.f, fc2),
		wp(-400.f, 200.f, fh),
		wp(-200.f, 200.f, fh),

		// 1
		wp(-600.f, -800.f, rw3),
		wp(-400.f, -800.f, rw1),
		wp(-200.f, -800.f, rw2),
		wp(0.f, -800.f, rw3),

		// 9
		wp(-600.f, -1000.f, rw1),
		wp(-600.f, -1200.f, rw2),
		wp(-600.f, -1400.f, rw2),

		// 5
		wp(-600.f, 800.f, rw3),
		wp(-400.f, 800.f, rw1),
		wp(-200.f, 800.f, rw2),

		// 11
		wp(-800.f, 800.f, rw3),
		wp(-800.f, 1000.f, rw1),
		wp(-800.f, 1200.f, rw2),
		wp(-800.f, 1400.f, rw2),

		// 6
		wp(-1000.f, 1400.f, rw2),
		wp(-1200.f, 1400.f, rw2),
		wp(-1400.f, 1400.f, rw2),
		wp(-1600.f, 1400.f, rw2),
		wp(-1800.f, 1400.f, rw2),
		wp(-2000.f, 1400.f, rw2),
		wp(-2200.f, 1400.f, rw2),
		wp(-2400.f, 1400.f, rw2),
		wp(-2600.f, 1400.f, rw2),
		wp(-2800.f, 1400.f, rw2),
		wp(-3000.f, 1400.f, rw2),
		wp(-3200.f, 1400.f, rw2),
		wp(-3400.f, 1400.f, rw2),
		wp(-3600.f, 1400.f, rw2),
		wp(-3800.f, 1400.f, rw2),
		wp(-4000.f, 1400.f, rw2),
		wp(-4200.f, 1400.f, rw2),
		wp(-4400.f, 1400.f, rw2),
		wp(-4600.f, 1400.f, rw2),
		wp(-4800.f, 1400.f, rw2),

		// 8
		wp(-800.f, -1400.f, rw2),
		wp(-1000.f, -1400.f, rw2),
		wp(-1200.f, -1400.f, rw2),
		wp(-1400.f, -1400.f, rw2),
		wp(-1600.f, -1400.f, rw2),
		wp(-1800.f, -1400.f, rw2),
		wp(-2000.f, -1400.f, rw2),
		wp(-2200.f, -1400.f, rw2),
		wp(-2400.f, -1400.f, rw2),
		wp(-2600.f, -1400.f, rw2),
		wp(-2800.f, -1400.f, rw2),
		wp(-3000.f, -1400.f, rw2),
		wp(-3200.f, -1400.f, rw2),
		wp(-3400.f, -1400.f, rw2),
		wp(-3600.f, -1400.f, rw2),
		wp(-3800.f, -1400.f, rw2),
		wp(-4000.f, -1400.f, rw2),
		wp(-4200.f, -1400.f, rw2),
		wp(-4400.f, -1400.f, rw2),
		wp(-4600.f, -1400.f, rw2),
		wp(-4800.f, -1400.f, rw2),
		
		// 12
		wp(-1200.f, 600.f, rw1),
		wp(-1200.f, 400.f, rw2),
		wp(-1200.f, 200.f, rw2),

		// 13
		wp(-1400.f, 200.f, rw3),
		wp(-1600.f, 200.f, rw1),

		// 14
		wp(-1200.f, -800.f, rw3),
		wp(-1200.f, -600.f, rw1),
		wp(-1200.f, -400.f, rw2),
		wp(-1200.f, -200.f, rw2),


		// 15
		wp(-1400.f, -800.f, rw3),
		wp(-1600.f, -800.f, rw1),
		wp(-1800.f, -800.f, rw3),
		wp(-2000.f, -800.f, rw1),
		wp(-2200.f, -800.f, rw3),
		wp(-2400.f, -800.f, rw1),

		// 16
		wp(-2000.f, -400.f, rw1),
		wp(-2200.f, -400.f, rw3),
		wp(-2400.f, -400.f, rw1),

		// 17
		wp(-2400.f, -200.f, rw1),
		wp(-2400.f, 0.f, rw3),
		wp(-2400.f, 200.f, rw1),
		wp(-2400.f, 400.f, rw1),
		wp(-2400.f, 600.f, rw1),
		wp(-2400.f, 800.f, rw1),
		wp(-2400.f, 1000.f, rw1),

		// 18
		wp(-2200.f, 1000.f, rw1),
		wp(-2000.f, 1000.f, rw1),
		wp(-1800.f, 1000.f, rw1),

		// 19
		wp(-1600.f, -300.f, rw1),

		// 3
		wp(-1400.f, 1200.f, rw1),

		// 19.2
		wp(-2600.f, 0.f, rw1),
		wp(-2800.f, 0.f, rw1),
		wp(-3000.f, 0.f, rw1),
		wp(-3200.f, 0.f, rw1),

		// 20
		wp(-3200.f, -200.f, rw1),
		wp(-3200.f, -400.f, rw1),
		wp(-3200.f, -600.f, rw1),
		wp(-3200.f, -800.f, rw1),

		// 21
		wp(-2800.f, -1200.f, rw1),
		wp(-2800.f, -1000.f, rw1),

		// 22.1
		wp(-2800.f, -400.f, rw1),

		// 22
		wp(-3200.f, 400.f, rw1),

		// 23
		wp(-3000.f, 800.f, rw1),
		wp(-3000.f, 1000.f, rw1),
		wp(-3000.f, 1200.f, rw1),

		// 24
		wp(-3600.f, 1000.f, rw1),
		wp(-3800.f, 1000.f, rw1),
		wp(-4000.f, 1000.f, rw1),

		// 25
		wp(-4200.f, 800.f, rw1),
		wp(-4200.f, 1000.f, rw1),

		// 26
		wp(-4200.f, 400.f, rw1),
		wp(-4200.f, -200.f, rw1),
		wp(-4200.f, -800.f, rw1),
		wp(-3700.f, 400.f, rw1),
		wp(-3700.f, -200.f, rw1),
		wp(-3700.f, -800.f, rw1),

		// 7
		wp(-5000.f, 1400.f, rw1),
		wp(-5000.f, 1200.f, rw1),
		wp(-5000.f, 1000.f, rw1),
		wp(-5000.f, 800.f, rw1),
		wp(-5000.f, 600.f, rw1),
		wp(-5000.f, 400.f, rw1),
		wp(-5000.f, 200.f, rw1),
		wp(-5000.f, 0.f, rw1),
		wp(-5000.f, -400.f, rw1),
		wp(-5000.f, -600.f, rw1),
		wp(-5000.f, -800.f, rw1),
		wp(-5000.f, -1000.f, rw1),
		wp(-5000.f, -1200.f, rw1),
		wp(-5000.f, -1400.f, rw1),
		

	};
	for (auto w : walls) {
		objs.emplace_back(std::make_shared<MapObject>(w.x, w.y, w.id));	
	}
	return new_map;
}

void Player::update_place_clones(float elapsed) {
	place_time_elapsed += elapsed;	
	if (place_time_elapsed > PLACE_CLONE_PHASE_DURATION) {
		state = FindClones;
		return;
	}
}

void Player::update_find_clones(float elapsed) {
	find_time_elapsed += elapsed;		
	if (find_time_elapsed > FIND_CLONE_PHASE_DURATION) {
		state = KillClones;
		return;
	}
}

void Player::update_kill_clones(float elapsed) {
	kill_time_elapsed += elapsed;	
	if (kill_time_elapsed > KILL_CLONE_PHASE_DURATION) {
		printf("Time is up!\n");
		state = GameOver;
		return;
	}
	if (clones.size() == 0) {
		printf("You Win!\n");
		state = GameOver;
		return;
	}
	// Move everything
	std::vector<std::shared_ptr<Clone>> new_clones;
	std::vector<std::shared_ptr<Clone>> new_enemy_clones;
	std::vector<std::shared_ptr<Bullet>> temp_bullets;
	std::unordered_set<std::shared_ptr<Bullet>> new_bullets;
	for (auto bullet : bullets) {
		bullet->move_bullet(elapsed);
		if (bullet->lifetime <= BULLET_LIFETIME) {
			temp_bullets.emplace_back(bullet);
		}
	}
	bullets = temp_bullets;
	for (auto clone : clones) {
		bool add = true;
		temp_bullets.clear();
		for (auto bullet : bullets) {
			if (bullet->collide(*clone)) {
				printf("colliding");
				if (clone->take_damage(BULLET_DAMAGE)) {
					add = false;
					///TODO: clone killed
				}	
			}
			else {
				temp_bullets.emplace_back(bullet);
			}
		}
		bullets = temp_bullets;
		if (add) {
			new_clones.emplace_back(clone);
		}
	}

	for (auto enemy_clone : enemy_clones) {
		bool add = true;
		temp_bullets.clear();
		for (auto bullet : temp_bullets) {
			if (bullet->collide(*enemy_clone)) {
				if (enemy_clone->take_damage(BULLET_DAMAGE)) {
					add = false;
					///TODO: enemy clone killed
				}	
			}
			else {
				temp_bullets.emplace_back(bullet);
			}
			
		}
		bullets = temp_bullets;
		if (add) {
			new_enemy_clones.emplace_back(enemy_clone);
		}
	}

	// Update clones
	clones = new_clones;
	enemy_clones = new_enemy_clones;

}
void Player::update(float elapsed) {
	switch(state) {
		case PlaceClones:
			update_place_clones(elapsed);
			return;
		case FindClones:
			update_find_clones(elapsed);
			return;
		case KillClones:	
			update_kill_clones(elapsed);
			return;
	}
}

void Game::update(float elapsed) {
	//position/velocity update:

}


void Game::send_state_message(Connection *connection_, Player *connection_player) const {
	
}

bool Game::recv_state_message(Connection *connection_) {

	return true;
}
