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
	ImageData *wall_sprite = &(sprites["wall"]);
	std::array<std::pair<float, float>, 3> wall_positions = {
		std::pair(100.f, 340.f),
		std::pair(250.f, 200.f),
		std::pair(-100.f, -100.f),
	};
	for (auto p : wall_positions) {
		objs.emplace_back(std::make_shared<MapObject>(p.first, p.second, wall_sprite));	
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
