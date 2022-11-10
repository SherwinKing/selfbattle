#include "Game.hpp"

#include "Connection.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>
#include <cassert>

#include "hex_dump.hpp"

#include <glm/gtx/norm.hpp>


//-----------------------------------------

Game::Game() : mt(0x15466666) {
	std::vector<std::pair<uint32_t, const char *>> sprite_paths = {
		std::pair(SPRITE::PLAYER_SPRITE_RED, "sprites/test.png"),
		std::pair(SPRITE::PLAYER_SPRITE_BLUE, "sprites/test.png"),
		std::pair(SPRITE::CLONE_SPRITE, "sprites/clone.png"),
		std::pair(SPRITE::WALL_SPRITE, "sprites/wall.png"),
		std::pair(SPRITE::BULLET_SPRITE, "sprites/bullet.png"),	
		std::pair(SPRITE::FENCE_SELF_H, "sprites/fence_self_h.png"),
		std::pair(SPRITE::FENCE_SELF_V, "sprites/fence_self_v.png"),
		std::pair(SPRITE::FENCE_HALF_T, "sprites/fence_half_t.png"),
		std::pair(SPRITE::FENCE_HALF_R, "sprites/fence_half_r.png"),
		std::pair(SPRITE::FENCE_HALF_B, "sprites/fence_half_b.png"),
		std::pair(SPRITE::FENCE_HALF_L, "sprites/fence_half_l.png"),
		std::pair(SPRITE::FENCE_FULL_H, "sprites/fence_full_h.png"),
		std::pair(SPRITE::FENCE_FULL_V, "sprites/fence_full_v.png"),
		std::pair(SPRITE::FENCE_CORNER_TR, "sprites/fence_corner_tr.png"),
		std::pair(SPRITE::FENCE_CORNER_RB, "sprites/fence_corner_rb.png"),
		std::pair(SPRITE::FENCE_CORNER_BL, "sprites/fence_corner_bl.png"),
		std::pair(SPRITE::FENCE_CORNER_LT, "sprites/fence_corner_lt.png"),
		std::pair(SPRITE::CLOCK_1, "sprites/clock_1.png"),
		std::pair(SPRITE::CLOCK_2, "sprites/clock_2.png"),
		std::pair(SPRITE::CLOCK_3, "sprites/clock_3.png"),
	};
	
	common_data = CommonData::get_instance();

	for (size_t i = 0; i < sprite_paths.size(); ++i) {
		const auto& p = sprite_paths[i];
		ImageData s;
		load_png(data_path(std::string(p.second)), &s.size, &s.pixels, LowerLeftOrigin);
		s.sprite_index = static_cast<uint8_t>(i);
		common_data->sprites.emplace_back(s);
	}

	common_data->map_objects = create_map();
	common_data->characters.reserve(2);
	common_data->characters.emplace_back( Character(PLAYER0_STARTING_X, PLAYER0_STARTING_Y, SPRITE::PLAYER_SPRITE_BLUE, 0) );
	common_data->characters.emplace_back( Character(PLAYER1_STARTING_X, PLAYER1_STARTING_Y, SPRITE::PLAYER_SPRITE_RED, 1) );

	players.reserve(2);
	players.emplace_back(Player(0));
	players.emplace_back(Player(1));
}

std::vector<MapObject> Game::create_map() {
	std::vector<MapObject> objs;
	std::vector<std::pair<float, float>> wall_positions = {
		std::pair(100.f, 340.f),
		std::pair(100.f, 440.f),
		std::pair(100.f, 540.f),
		std::pair(100.f, 640.f),
		std::pair(50.f, 640.f),
		std::pair(150.f, 640.f),
		std::pair(200.f, 640.f),

		std::pair(-1000.f, 340.f),
		std::pair(-1000.f, 440.f),
		std::pair(-1000.f, 540.f),
		std::pair(-1000.f, 640.f),
		std::pair(-500.f, 640.f),
		std::pair(-1500.f, 640.f),
		std::pair(-2000.f, 640.f),

		std::pair(1000.f,- 340.f),
		std::pair(1000.f,- 440.f),
		std::pair(1000.f,- 540.f),
		std::pair(1000.f,- 640.f),
		std::pair(500.f, -640.f),
		std::pair(1500.f,- 640.f),
		std::pair(2000.f,- 640.f),

		std::pair(300.f,- 340.f),
		std::pair(300.f,- 440.f),
		std::pair(300.f,- 540.f),
		std::pair(300.f,- 640.f),
		std::pair(350.f, -640.f),
		std::pair(450.f,- 640.f),
		std::pair(500.f,- 640.f),

	};
	for (auto p : wall_positions) {
		objs.emplace_back(MapObject(p.first, p.second, SPRITE::WALL_SPRITE));	
	}
	return objs;
}

Player *Game::spawn_player() {
	assert(player_cnt < 2);

	Player &player = players[player_cnt];

	player_cnt++;

	return &player;
}

void Player::try_shooting() {
	if (shoot_interval > 0) {
		return;
	}

	shoot_interval = BULLET_INTERVAL;

	CommonData *common_data = CommonData::get_instance();
	Character c = common_data->characters[player_id];

	glm::vec2 shoot_velo;
	shoot_velo.x = mouse_x - c.x;
	shoot_velo.y = mouse_y - c.y;
	shoot_velo = glm::normalize(shoot_velo) * BULLET_SPEED;

	common_data->bullets.emplace_back(Bullet(c.x, c.y, SPRITE::BULLET_SPRITE, shoot_velo, player_id));	
	float amount_to_move = static_cast<float>(static_cast<uint32_t>(PLAYER_SIZE / BULLET_SPEED) + 1);
	common_data->bullets.back().move_bullet(amount_to_move);
} 

void Player::place_clone() {
	Clone clone = Clone(mouse_x, mouse_y, SPRITE::CLONE_SPRITE, player_id); 
	CommonData::get_instance()->clones.emplace_back(clone);	
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

void Game::setup_place_clones() {
	state = PlaceClones;
	time_remaining = PLACE_CLONE_PHASE_DURATION;
}

void Game::update_place_clones(float elapsed) {
	time_remaining -= elapsed;
	if (time_remaining < 0) {
		setup_find_clones();
		return;
	}
}

void Game::setup_find_clones() {
	state = FindClones;
	time_remaining = FIND_CLONE_PHASE_DURATION;
	common_data->characters[0].x = PLAYER1_STARTING_X;
	common_data->characters[0].y = PLAYER1_STARTING_Y;
	common_data->characters[1].x = PLAYER0_STARTING_X;
	common_data->characters[1].y = PLAYER0_STARTING_Y;
}

void Game::update_find_clones(float elapsed) {
	time_remaining -= elapsed;
	if (time_remaining < 0) {
		setup_kill_clones();
		return;
	}
}

void Game::setup_kill_clones() {
	state = KillClones;
	time_remaining = KILL_CLONE_PHASE_DURATION;
	common_data->characters[0].x = PLAYER1_STARTING_X;
	common_data->characters[0].y = PLAYER1_STARTING_Y;
	common_data->characters[1].x = PLAYER0_STARTING_X;
	common_data->characters[1].y = PLAYER0_STARTING_Y;
}

void Game::update_kill_clones(float elapsed) {
	time_remaining -= elapsed;	
	if (time_remaining < 0) {
		printf("Time is up!\n");
		state = GameOver;
		return;
	}
	// TODO: win condition
	// if (clones.size() == 0) {
	// 	printf("You Win!\n");
	// 	state = GameOver;
	// 	return;
	// }

	// Move everything
	for (Bullet &bullet : common_data->bullets) {
		bullet.move_bullet(elapsed);
		for (Clone &clone : common_data->clones) {
			if (bullet.collide(clone)) {
				clone.take_damage(BULLET_DAMAGE);
				bullet.active = false;
			}
		}
		for (Character &character : common_data->characters) {
			if (bullet.collide(character)) {
				character.take_damage(BULLET_DAMAGE);
				bullet.active = false;
			}
		}
		for (MapObject &map_obj : common_data->map_objects) {
			if (bullet.collide(map_obj)) {
				bullet.active = false;
			}
		}
		if (bullet.lifetime > BULLET_LIFETIME) {
			bullet.active = false;;
		}
	}

	// remove items that should be destroyed
	common_data->bullets.erase(
		std::remove_if(common_data->bullets.begin(),
					   common_data->bullets.end(),
					   [](Bullet bullet){return !bullet.active;}),
		common_data->bullets.end()
	);

	common_data->clones.erase(
		std::remove_if(common_data->clones.begin(),
					   common_data->clones.end(),
					   [](Clone clone){return clone.hp <= 0;}),
		common_data->clones.end()
	);

	for (Player &player : players) {
		player.shoot_interval -= elapsed;
	}
}

void Game::update(float elapsed) {
	// wait if players haven't arrived yet
	if (!ready) {
		return;
	}

	for (Player &player : players) {
		// TODO: register the buttons and store them with timestamps
		if (player.left.state == Button::BTN_DOWN) {
			player.left.state = Button::BTN_IS_PRESSED;
		}
		if (player.right.state == Button::BTN_DOWN) {
			player.right.state = Button::BTN_IS_PRESSED;
		}
		if (player.up.state == Button::BTN_DOWN) {
			player.up.state = Button::BTN_IS_PRESSED;
		}
		if (player.down.state == Button::BTN_DOWN) {
			player.down.state = Button::BTN_IS_PRESSED;
		}
		if (player.mouse.state == Button::BTN_DOWN) {
			// TODO: refactor
			player.mouse.state = Button::BTN_IS_PRESSED;
			if (state == PlaceClones) {
				int clone_num = 0;
				for (Clone clone : common_data->clones) {
					if (clone.player_id == player.player_id) {
						clone_num++;
					}
				}
				player.place_clone();
			}
			if (state == KillClones) {
				player.try_shooting();
			}
		}
		glm::vec2 dir = glm::vec2(0.0f, 0.0f);
		if (player.left.state == Button::BTN_IS_PRESSED) dir.x -= 1.0f;
		if (player.right.state == Button::BTN_IS_PRESSED) dir.x += 1.0f;
		if (player.down.state == Button::BTN_IS_PRESSED) dir.y -= 1.0f;
		if (player.up.state == Button::BTN_IS_PRESSED) dir.y += 1.0f;

		if (dir.x != 0 || dir.y != 0) {
			dir = glm::normalize(dir);
			common_data->characters[player.player_id].move_character(dir.x * PLAYER_SPEED, dir.y * PLAYER_SPEED);
		}
	}

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

void Game::send_message(Connection *connection_, Player *connection_player, MESSAGE message_type) const {
	assert(connection_);
	auto &connection = *connection_;

	connection.send(message_type);
	//will patch message size in later, for now placeholder bytes:
	connection.send(uint8_t(0));
	connection.send(uint8_t(0));
	connection.send(uint8_t(0));
	size_t mark = connection.send_buffer.size(); //keep track of this position in the buffer


	auto send_character = [&](Character const &character) {
		connection.send(character.x);
		connection.send(character.y);
		connection.send(character.sprite_index);
		connection.send(character.rot);
		connection.send(character.hp);
	};

	auto send_player = [&](Player const &player) {
		connection.send(player.left.state);
		connection.send(player.right.state);
		connection.send(player.up.state);
		connection.send(player.down.state);
		connection.send(player.mouse.state);
		connection.send(player.mouse_x);
		connection.send(player.mouse_y);
	};


	switch(message_type) {
		case MESSAGE::SERVER_INIT:
			std::cout << "send server_init\n";
			connection.send(connection_player->player_id);
			break;
		case MESSAGE::PLAYER_INPUT:
			std::cout << "send player input\n";
			send_player(*connection_player);
			send_character(common_data->characters[connection_player->player_id]);
			break;
		case MESSAGE::PLAYER_READY:
			std::cout << "send player ready\n";
			break;
		case MESSAGE::SERVER_READY:
			std::cout << "send server ready\n";
			// sending out the message type as signal
			break;
		default:
			std::cout << "this should not happen\n";
			break;
	}


	//compute the message size and patch into the message header:
	uint32_t size = uint32_t(connection.send_buffer.size() - mark);
	connection.send_buffer[mark-3] = uint8_t(size);
	connection.send_buffer[mark-2] = uint8_t(size >> 8);
	connection.send_buffer[mark-1] = uint8_t(size >> 16);
	// std::cout << "[" << connection.socket << "] recv'd data. Current buffer:\n" << hex_dump(connection.send_buffer); std::cout.flush(); //DEBUG
}

MESSAGE Game::recv_message(Connection *connection_, Player *client_player, bool is_server) {
	assert(connection_);
	auto &connection = *connection_;
	auto &recv_buffer = connection.recv_buffer;

	//expecting [type, size_low0, size_mid8, size_high8]:
	if (recv_buffer.size() < 4) return MESSAGE::MSG_NONE;
	uint32_t size = (uint32_t(recv_buffer[3]) << 16)
	              | (uint32_t(recv_buffer[2]) << 8)
	              |  uint32_t(recv_buffer[1]);
	
	//expecting complete message:
	if (recv_buffer.size() < 4 + size) return MESSAGE::MSG_NONE;
	

	uint32_t at = 0;
	auto read = [&](auto *val) {
		if (at + sizeof(*val) > size) {
			throw std::runtime_error("Ran out of bytes reading state message.");
		}
		std::memcpy(val, &recv_buffer[4 + at], sizeof(*val));
		at += sizeof(*val);
	};

	auto read_character = [&](Character *character) {
		read(&character->x);
		read(&character->y);
		read(&character->sprite_index);
		read(&character->rot);
		read(&character->hp);
	};

	auto read_player = [&](Player *player) {
		read(&player->left.state);
		read(&player->right.state);
		read(&player->up.state);
		read(&player->down.state);
		read(&player->mouse.state);
		read(&player->mouse_x);
		read(&player->mouse_y);
	};

	MESSAGE message_type = (MESSAGE) recv_buffer[0];
	switch(message_type) {
		case MESSAGE::SERVER_INIT:
			std::cout << "read server_init\n";
			assert(!is_server);
			read(&client_player->player_id);
			break;
		case MESSAGE::PLAYER_INPUT:
			std::cout << "read player_input\n";
			// TODO: adjust for network latency
			read_player(client_player);
			read_character(&common_data->characters[client_player->player_id]);
			if (is_server) {
				message_queue.push_back(MessageInfo(MESSAGE::PLAYER_INPUT, client_player->player_id));
			}
			break;
		case MESSAGE::PLAYER_READY:
			std::cout << "read player_ready\n";
			assert(is_server);
			client_player->ready = true;
			if (players[0].ready && players[1].ready) {
				message_queue.push_back(MessageInfo(MESSAGE::SERVER_READY, 1));
				message_queue.push_back(MessageInfo(MESSAGE::SERVER_READY, 0));
			}
			break;
		case MESSAGE::SERVER_READY:
			std::cout << "read server_ready\n";
			assert(!is_server);
			players[0].ready = true;
			players[1].ready = true;
			ready = true;
			break;
		default:
			// TODO: raise an error here if we know this shouldn't happen
			std::cout << "No matching tag " << std::to_string((uint8_t)message_type) << ", probably an error here\n";
			return MESSAGE::MSG_NONE;
	}

	// std::cout << "[" << connection.socket << "] recv'd data. Current buffer:\n" << hex_dump(connection.recv_buffer); std::cout.flush(); //DEBUG
	//delete message from buffer:
	recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

	return message_type;
}
