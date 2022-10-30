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
		
		CommonData *common_data = CommonData::get_instance();
		for (size_t i = 0; i < NUM_SPRITES; ++i) {
            const auto& p = sprite_paths[i];
            ImageData s;
            load_png(data_path(std::string(p.second)), &s.size, &s.pixels, LowerLeftOrigin);
			s.sprite_index = i;
            common_data->sprites.emplace_back(s);
	    }
		// TODO: implement this
		// state = PlaceClones;
		// c.init(PLAYER1_STARTING_X, PLAYER1_STARTING_Y);
		// ImageData& player_sprite = sprites["player0"];
		// c.set_box(player_sprite.size.x, player_sprite.size.y);

		common_data->map_objects = create_map();
}

void Player::send_message(Connection *connection_) const {
	assert(connection_);
	auto &connection = *connection_;

	uint32_t size = 5;
	connection.send(Message::C2S_Controls);
	connection.send(uint8_t(size));
	connection.send(uint8_t(size >> 8));
	connection.send(uint8_t(size >> 16));

	auto send_button = [&](Button const &b) {
		if (b.downs & 0x80) {
			std::cerr << "Wow, you are really good at pressing buttons!" << std::endl;
		}
		connection.send(uint8_t( (b.pressed ? 0x80 : 0x00) | (b.downs & 0x7f) ) );
	};

	send_button(left);
	send_button(right);
	send_button(up);
	send_button(down);
	send_button(mouse);
}

bool Player::recv_message(Connection *connection_) {
	assert(connection_);
	auto &connection = *connection_;

	auto &recv_buffer = connection.recv_buffer;

	//expecting [type, size_low0, size_mid8, size_high8]:
	if (recv_buffer.size() < 4) return false;
	if (recv_buffer[0] != uint8_t(Message::C2S_Controls)) return false;
	uint32_t size = (uint32_t(recv_buffer[3]) << 16)
	              | (uint32_t(recv_buffer[2]) << 8)
	              |  uint32_t(recv_buffer[1]);
	if (size != 5) throw std::runtime_error("Controls message with size " + std::to_string(size) + " != 5!");
	
	//expecting complete message:
	if (recv_buffer.size() < 4 + size) return false;

	auto recv_button = [](uint8_t byte, Button *button) {
		button->pressed = (byte & 0x80);
		uint32_t d = uint32_t(button->downs) + uint32_t(byte & 0x7f);
		if (d > 255) {
			std::cerr << "got a whole lot of downs" << std::endl;
			d = 255;
		}
		button->downs = uint8_t(d);
	};

	recv_button(recv_buffer[4+0], &left);
	recv_button(recv_buffer[4+1], &right);
	recv_button(recv_buffer[4+2], &up);
	recv_button(recv_buffer[4+3], &down);
	recv_button(recv_buffer[4+4], &mouse);

	//delete message from buffer:
	recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

	return true;
}

Player *Game::spawn_player() {
	players.emplace_back();
	Player &player = players.back();

	return &player;
}


void Game::shoot (float world_x, float world_y, int player_id) {
	CommonData *common_data = CommonData::get_instance();
	Character c = characters[player_id];

	glm::vec2 shoot_velo;
	shoot_velo.x = world_x - c.x;
	shoot_velo.y = world_y - c.y;
	shoot_velo = glm::normalize(shoot_velo) * BULLET_SPEED;

	ImageData *bullet_sprite = &(common_data->sprites[bullet]);
	Bullet bullet = Bullet(c.x, c.y, bullet_sprite, shoot_velo, player_id); 
	common_data->bullets.emplace_back(bullet);	
	float amount_to_move = static_cast<float>(static_cast<uint32_t>(PLAYER_SIZE / BULLET_SPEED) + 1);

	bullet.move_bullet(amount_to_move);
} 

void Game::place_clone(float world_x, float world_y, int player_id) {
	CommonData *common_data = CommonData::get_instance();
	ImageData *clone_sprite = &(common_data->sprites[clone]);
	Clone clone = Clone(world_x, world_y, clone_sprite, player_id); 
	
	common_data->clones.emplace_back(clone);	
}

void Game::remove_player(Player *player) {
	bool found = false;
	for (auto pi = players.begin(); pi != players.end(); ++pi) {
		if (&*pi == player) {
			players.erase(pi);
			found = true;
			// TODO: add mechanism for reconnecting players
			break;
		}
	}
	assert(found);
}

std::vector<MapObject> Game::create_map() {
	CommonData *common_data = CommonData::get_instance();
	std::vector<MapObject> objs;
	ImageData *wall_sprite = &(common_data->sprites[wall]);
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

void Game::update_place_clones(float elapsed) {
	place_time_elapsed += elapsed;	
	if (place_time_elapsed > PLACE_CLONE_PHASE_DURATION) {
		state = FindClones;
		return;
	}
}

void Game::update_find_clones(float elapsed) {
	find_time_elapsed += elapsed;		
	if (find_time_elapsed > FIND_CLONE_PHASE_DURATION) {
		state = KillClones;
		return;
	}
}

void Game::update_kill_clones(float elapsed) {
	
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

void Game::update(float elapsed) {
	//position/velocity update:
	// switch(state) {
	// 	case PlaceClones:
	// 		update_place_clones(elapsed);
	// 		break;
	// 	case FindClones:
	// 		update_find_clones(elapsed);
	// 		break;
	// 	case KillClones:	
	// 		update_kill_clones(elapsed);
	// 		break;
	// 	default:
	// 		break;
	// }

	// switch (common_data.state) {
	// 	case PlaceClones: 
	// 		if (common_data.clones.size() >= NUM_CLONES) {
	// 			return false;	
	// 		}
	// 		player.place_clone(screen_x, screen_y, window_size);	
	// 		return true;
	// 	case KillClones:
	// 		player.shoot(screen_x, screen_y, window_size);
	// 		return true;
	// 	default:
	// 		return false;	
	// }
	// player.move_player(-1.0 * PLAYER_SPEED, 0.0);
}


void Game::send_state_message(Connection *connection_, Player *connection_player) const {
	
}

bool Game::recv_state_message(Connection *connection_) {

	return true;
}
