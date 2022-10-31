#include "Game.hpp"

#include "Connection.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>
#include <cassert>

#include <glm/gtx/norm.hpp>


//-----------------------------------------

Game::Game() : mt(0x15466666) {
		std::array<std::pair<uint32_t, const char *>, NUM_SPRITES> sprite_paths = {
			std::pair(PLAYER_SPRITE, "sprites/test.png"),
			std::pair(CLONE_SPRITE, "sprites/clone.png"),
			std::pair(WALL_SPRITE, "sprites/wall.png"),
			std::pair(BULLET_SPRITE, "sprites/bullet.png")	
		};
		
		common_data = CommonData::get_instance();

		for (size_t i = 0; i < NUM_SPRITES; ++i) {
            const auto& p = sprite_paths[i];
            ImageData s;
            load_png(data_path(std::string(p.second)), &s.size, &s.pixels, LowerLeftOrigin);
			s.sprite_index = i;
            common_data->sprites.emplace_back(s);
	    }

		common_data->map_objects = create_map();

		common_data->clones.emplace_back();
		common_data->clones.emplace_back();
		common_data->bullets.emplace_back();
		common_data->bullets.emplace_back();
		common_data->bullets.emplace_back();
		common_data->bullets.emplace_back();
		common_data->bullets.emplace_back();
		common_data->bullets.emplace_back();
		common_data->bullets.emplace_back();
		common_data->bullets.emplace_back();
		common_data->bullets.emplace_back();
		common_data->bullets.emplace_back();
		common_data->bullets.emplace_back();
}

void Game::send_setup_message(Connection *connection_, Player *connection_player) const {
	assert(connection_);
	auto &connection = *connection_;

	connection.send(Message::S2C_Setup);
	//will patch message size in later, for now placeholder bytes:
	connection.send(uint8_t(0));
	connection.send(uint8_t(0));
	connection.send(uint8_t(0));
	size_t mark = connection.send_buffer.size(); //keep track of this position in the buffer

	connection.send(connection_player->player_id);

	// TODO: send character info
	
	//compute the message size and patch into the message header:
	uint32_t size = uint32_t(connection.send_buffer.size() - mark);
	connection.send_buffer[mark-3] = uint8_t(size);
	connection.send_buffer[mark-2] = uint8_t(size >> 8);
	connection.send_buffer[mark-1] = uint8_t(size >> 16);
}

bool Game::recv_setup_message(Connection *connection_, Player *client_player) {
	assert(connection_);
	auto &connection = *connection_;

	auto &recv_buffer = connection.recv_buffer;

	//expecting [type, size_low0, size_mid8, size_high8]:
	if (recv_buffer.size() < 4) return false;
	if (recv_buffer[0] != uint8_t(Message::S2C_Setup)) return false;
	uint32_t size = (uint32_t(recv_buffer[3]) << 16)
	              | (uint32_t(recv_buffer[2]) << 8)
	              |  uint32_t(recv_buffer[1]);
	
	//expecting complete message:
	if (recv_buffer.size() < 4 + size) return false;

	client_player->player_id = recv_buffer[4];

	// TODO: read character info

	assert(common_data->characters.size() == 0);

	//delete message from buffer:
	recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

	return true;
}

void Player::send_player_message(Connection *connection_) const {
	assert(connection_);
	auto &connection = *connection_;

	uint32_t size = 14;
	connection.send(Message::C2S_Player);
	connection.send(uint8_t(size));
	connection.send(uint8_t(size >> 8));
	connection.send(uint8_t(size >> 16));

	auto send_button = [&](Button const &b) {
		if (b.downs & 0x80) {
			std::cerr << "Wow, you are really good at pressing buttons!" << std::endl;
		}
		connection.send(uint8_t( (b.pressed ? 0x80 : 0x00) | (b.downs & 0x7f) ) );
	};

	connection.send(player_id);

	send_button(left);
	send_button(right);
	send_button(up);
	send_button(down);
	send_button(mouse);

	connection.send(mouse_x);
	connection.send(mouse_y);
}

bool Player::recv_player_message(Connection *connection_) {
	assert(connection_);
	auto &connection = *connection_;

	auto &recv_buffer = connection.recv_buffer;

	//expecting [type, size_low0, size_mid8, size_high8]:
	if (recv_buffer.size() < 4) return false;
	if (recv_buffer[0] != uint8_t(Message::C2S_Player)) return false;
	uint32_t size = (uint32_t(recv_buffer[3]) << 16)
	              | (uint32_t(recv_buffer[2]) << 8)
	              |  uint32_t(recv_buffer[1]);
	if (size != 14) throw std::runtime_error("Controls message with size " + std::to_string(size) + " != 14!");
	
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

	auto recv_float = [&recv_buffer](int offset, float *f) {
		// TODO: check for endienness and receive accordingly
		char temp[4];
		temp[0] = recv_buffer[offset+0];
		temp[1] = recv_buffer[offset+1];
		temp[2] = recv_buffer[offset+2];
		temp[3] = recv_buffer[offset+3];
		*f = *(reinterpret_cast<float *>(temp));
	};

	player_id = recv_buffer[4+0];
	recv_button(recv_buffer[4+1], &left);
	recv_button(recv_buffer[4+2], &right);
	recv_button(recv_buffer[4+3], &up);
	recv_button(recv_buffer[4+4], &down);
	recv_button(recv_buffer[4+5], &mouse);

	recv_float(recv_buffer[4+6], &mouse_x);
	recv_float(recv_buffer[4+10], &mouse_y);

	//delete message from buffer:
	recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

	return true;
}

Player *Game::spawn_player() {
	assert(next_player_number < 2);

	players.emplace_back();
	Player &player = players.back();

	player.player_id = next_player_number;
	next_player_number++;

	return &player;
}

Character *Game::spawn_character(Player *new_player) {
	Character new_character;
	if (new_player->player_id == 0) {
		new_character = Character(PLAYER0_STARTING_X, PLAYER0_STARTING_Y, PLAYER_SPRITE, 0);
	}
	else {
		assert(new_player->player_id == 1);
		new_character = Character(PLAYER1_STARTING_X, PLAYER1_STARTING_Y, PLAYER_SPRITE, 1);
	}
	common_data->characters.emplace_back(new_character);

	return &common_data->characters.back();
}

void Game::shoot (float world_x, float world_y, int player_id) {
	Character c = common_data->characters[player_id];

	glm::vec2 shoot_velo;
	shoot_velo.x = world_x - c.x;
	shoot_velo.y = world_y - c.y;
	shoot_velo = glm::normalize(shoot_velo) * BULLET_SPEED;

	Bullet bullet = Bullet(c.x, c.y, BULLET_SPRITE, shoot_velo, player_id); 
	common_data->bullets.emplace_back(bullet);	
	float amount_to_move = static_cast<float>(static_cast<uint32_t>(PLAYER_SIZE / BULLET_SPEED) + 1);

	bullet.move_bullet(amount_to_move);
} 

void Game::place_clone(float world_x, float world_y, int player_id) {
	Clone clone = Clone(world_x, world_y, CLONE_SPRITE, player_id); 
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
	std::vector<MapObject> objs;
	std::array<std::pair<float, float>, 3> wall_positions = {
		std::pair(100.f, 340.f),
		std::pair(250.f, 200.f),
		std::pair(-100.f, -100.f),
	};
	for (auto p : wall_positions) {
		objs.emplace_back(MapObject(p.first, p.second, WALL_SPRITE));	
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

// state, vector<bullet>, vector<clone>, vector<character>
void Game::send_state_message(Connection *connection_, Player *connection_player) const {
	assert(connection_);
	auto &connection = *connection_;

	connection.send(Message::S2C_State);
	//will patch message size in later, for now placeholder bytes:
	connection.send(uint8_t(0));
	connection.send(uint8_t(0));
	connection.send(uint8_t(0));
	size_t mark = connection.send_buffer.size(); //keep track of this position in the buffer

	connection.send((uint8_t)state);


{
	auto send_bullet = [&](Bullet const &bullet) {
		connection.send(bullet.x);
		connection.send(bullet.y);
		connection.send(bullet.sprite_index);
		connection.send(bullet.tag);
		connection.send(bullet.velo);
	};
	int bullet_size = common_data->bullets.size();
	connection.send(bullet_size);
	for (int i = 0; i < bullet_size; i++) {
		send_bullet(common_data->bullets[i]);
	}
}
{
	auto send_clone = [&](Clone const &clone) {
		connection.send(clone.x);
		connection.send(clone.y);
		connection.send(clone.sprite_index);
		connection.send(clone.tag);
		connection.send(clone.hp);
	};
	int clone_size = common_data->clones.size();
	connection.send(clone_size);
	for (int i = 0; i < clone_size; i++) {
		send_clone(common_data->clones[i]);
	}
}
{
	auto send_character = [&](Character const &character) {
		connection.send(character.x);
		connection.send(character.y);
		connection.send(character.sprite_index);
		// Character tag is not sent!!!
		connection.send(character.rot);
		connection.send(character.hp);
	};
	int character_size = common_data->characters.size();
	assert(character_size > 0);
	connection.send(character_size);
	for (int i = 0; i < character_size; i++) {
		send_character(common_data->characters[i]);
	}
}
	uint8_t a = 123;
	connection.send(a);

	//compute the message size and patch into the message header:
	uint32_t size = uint32_t(connection.send_buffer.size() - mark);
	connection.send_buffer[mark-3] = uint8_t(size);
	connection.send_buffer[mark-2] = uint8_t(size >> 8);
	connection.send_buffer[mark-1] = uint8_t(size >> 16);
}

bool Game::recv_state_message(Connection *connection_) {
	assert(connection_);
	auto &connection = *connection_;
	auto &recv_buffer = connection.recv_buffer;

	if (recv_buffer.size() < 4) return false;
	if (recv_buffer[0] != uint8_t(Message::S2C_State)) return false;
	uint32_t size = (uint32_t(recv_buffer[3]) << 16)
	              | (uint32_t(recv_buffer[2]) << 8)
	              |  uint32_t(recv_buffer[1]);
	uint32_t at = 0;
	//expecting complete message:
	if (recv_buffer.size() < 4 + size) return false;

	//copy bytes from buffer and advance position:
	auto read = [&](auto *val) {
		if (at + sizeof(*val) > size) {
			throw std::runtime_error("Ran out of bytes reading state message.");
		}
		std::memcpy(val, &recv_buffer[4 + at], sizeof(*val));
		at += sizeof(*val);
	};

	read(&state);
{
	common_data->bullets.clear();
	int bullet_count;
	read(&bullet_count);
	for (int i = 0; i < bullet_count; i++) {
		common_data->bullets.emplace_back();
		Bullet &bullet = common_data->bullets.back();
		read(&bullet.x);
		read(&bullet.y);
		read(&bullet.sprite_index);
		read(&bullet.tag);
		read(&bullet.velo);
	}
}
{
	common_data->clones.clear();
	int clone_count;
	read(&clone_count);
	for (int i = 0; i < clone_count; i++) {
		common_data->clones.emplace_back();
		Clone &clone = common_data->clones.back();
		read(&clone.x);
		read(&clone.y);
		read(&clone.sprite_index);
		read(&clone.tag);
		read(&clone.hp);
	}
}
{
	common_data->characters.clear();
	int character_count;
	read(&character_count);
	assert(character_count > 0);
	assert(character_count < 0);
	std::cout << std::to_string(character_count) << "characters\n";
	for (int i = 0; i < character_count; i++) {
		common_data->characters.emplace_back();
		Character &character = common_data->characters.back();
		read(&character.x);
		read(&character.y);
		read(&character.sprite_index);
		read(&character.rot);
		read(&character.hp);
	}
}

	if (at != size) throw std::runtime_error("Trailing data in state message.");

	//delete message from buffer:
	recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

	return true;
}
