#include "Game.hpp"

#include "Connection.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>

#include <glm/gtx/norm.hpp>


//-----------------------------------------

Game::Game() : mt(0x15466666) {
		std::array<std::pair<uint32_t, const char *>, NUM_SPRITES> sprite_paths = {
			std::pair(player0, "sprites/test.png"),
			std::pair(clone, "sprites/clone.png"),
			std::pair(wall, "sprites/wall.png"),
			std::pair(bullet, "sprites/bullet.png")	
		};
		
		for (size_t i = 0; i < NUM_SPRITES; ++i) {
            const auto& p = sprite_paths[i];
            ImageData s;
            load_png(data_path(std::string(p.second)), &s.size, &s.pixels, LowerLeftOrigin);
			s.sprite_index = i;
            common_data.sprites.emplace_back(s);
	    }
		// TODO: implement this
		// state = PlaceClones;
		// c.init(PLAYER1_STARTING_X, PLAYER1_STARTING_Y);
		// ImageData& player_sprite = sprites["player0"];
		// c.set_box(player_sprite.size.x, player_sprite.size.y);

		common_data.map_objects = create_map();
}

void Player::set_position(float new_x, float new_y) {
	c.x = new_x;
	c.y = new_y;
}

bool Player::recv_message(Connection *connection){
	return true;
}

void Player::move_player(float dx, float dy) {
	// do nothing if we're running to an obstacle
	for (auto mapobj : common_data.map_objects)
	{
		if (c.collide(mapobj)) {
			return;
		}
	}
	for (auto clone : common_data.clones) {
		if (c.collide(clone)) {
			return;
		}
	}	
	for (auto clone : common_data.enemy_clones) {
		if (c.collide(clone)) {
			return;
		}
	}

	c.move(dx, dy);
}

Player *Game::spawn_player() {
	players.emplace_back();
	Player &player = players.back();

	return &player;
}


void Player::shoot (float world_x, float world_y) {	
	glm::vec2 shoot_velo;
	shoot_velo.x = world_x - c.x;
	shoot_velo.y = world_y - c.y;
	shoot_velo = glm::normalize(shoot_velo) * BULLET_SPEED;

	ImageData *bullet_sprite = &(common_data.sprites[bullet]);
	Bullet bullet = Bullet(c.x, c.y, bullet_sprite, shoot_velo); 
	common_data.bullets.emplace_back(bullet);	
	float amount_to_move = static_cast<float>(static_cast<uint32_t>(PLAYER_SIZE / BULLET_SPEED) + 1);

	bullet.move_bullet(amount_to_move);
} 

void Player::place_clone(float world_x, float world_y) {
	ImageData *clone_sprite = &(common_data.sprites[clone]);
	Clone clone = Clone(world_x, world_y, clone_sprite); 
	
	common_data.clones.emplace_back(clone);	
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

std::vector<MapObject> Game::create_map() {
	std::vector<MapObject> objs;
	ImageData *wall_sprite = &(common_data.sprites[wall]);
	std::array<std::pair<float, float>, 3> wall_positions = {
		std::pair(100.f, 340.f),
		std::pair(250.f, 200.f),
		std::pair(-100.f, -100.f),
	};
	for (auto p : wall_positions) {
		objs.emplace_back(MapObject(p.first, p.second, wall_sprite));	
	}
	return objs;
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
	
}
// void Player::update_kill_clones(float elapsed) {
// 	kill_time_elapsed += elapsed;	
// 	if (kill_time_elapsed > KILL_CLONE_PHASE_DURATION) {
// 		printf("Time is up!\n");
// 		state = GameOver;
// 		return;
// 	}
// 	if (clones.size() == 0) {
// 		printf("You Win!\n");
// 		state = GameOver;
// 		return;
// 	}
// 	// Move everything
// 	std::vector<Clone> new_clones;
// 	std::vector<Clone> new_enemy_clones;
// 	std::vector<Bullet> temp_bullets;
// 	std::unordered_set<Bullet> new_bullets;
// 	for (auto bullet : bullets) {
// 		bullet->move_bullet(elapsed);
// 		if (bullet->lifetime <= BULLET_LIFETIME) {
// 			temp_bullets.emplace_back(bullet);
// 		}
// 	}
// 	bullets = temp_bullets;
// 	for (auto clone : clones) {
// 		bool add = true;
// 		temp_bullets.clear();
// 		for (auto bullet : bullets) {
// 			if (bullet->collide(*clone)) {
// 				printf("colliding");
// 				if (clone->take_damage(BULLET_DAMAGE)) {
// 					add = false;
// 					///TODO: clone killed
// 				}	
// 			}
// 			else {
// 				temp_bullets.emplace_back(bullet);
// 			}
// 		}
// 		bullets = temp_bullets;
// 		if (add) {
// 			new_clones.emplace_back(clone);
// 		}
// 	}

// 	for (auto enemy_clone : enemy_clones) {
// 		bool add = true;
// 		temp_bullets.clear();
// 		for (auto bullet : temp_bullets) {
// 			if (bullet->collide(*enemy_clone)) {
// 				if (enemy_clone->take_damage(BULLET_DAMAGE)) {
// 					add = false;
// 					///TODO: enemy clone killed
// 				}	
// 			}
// 			else {
// 				temp_bullets.emplace_back(bullet);
// 			}
			
// 		}
// 		bullets = temp_bullets;
// 		if (add) {
// 			new_enemy_clones.emplace_back(enemy_clone);
// 		}
// 	}

// 	// Update clones
// 	clones = new_clones;
// 	enemy_clones = new_enemy_clones;
// }

void Player::update(float elapsed) {
	switch(state) {
		case PlaceClones:
			update_place_clones(elapsed);
			break;
		case FindClones:
			update_find_clones(elapsed);
			break;
		case KillClones:	
			update_kill_clones(elapsed);
			break;
		default:
			break;
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
