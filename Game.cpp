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
			std::pair(SPRITE::PLAYER_SPRITE, "sprites/test.png"),
			std::pair(SPRITE::CLONE_SPRITE, "sprites/clone.png"),
			std::pair(SPRITE::WALL_SPRITE, "sprites/wall.png"),
			std::pair(SPRITE::BULLET_SPRITE, "sprites/bullet.png")	
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
}

Player *Game::spawn_player() {
	assert(player_cnt < 2);

	players.emplace_back();
	Player &player = players.back();

	player.player_id = player_cnt;
	player_cnt++;

	return &player;
}

Character *Game::spawn_character(Player *new_player) {
	Character new_character;
	if (new_player->player_id == 0) {
		new_character = Character(PLAYER0_STARTING_X, PLAYER0_STARTING_Y, SPRITE::PLAYER_SPRITE, 0);
	}
	else {
		assert(new_player->player_id == 1);
		new_character = Character(PLAYER1_STARTING_X, PLAYER1_STARTING_Y, SPRITE::PLAYER_SPRITE, 1);
	}
	common_data->characters.emplace_back(new_character);

	return &common_data->characters.back();
}

void Player::shoot() {
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

std::vector<MapObject> Game::create_map() {
	std::vector<MapObject> objs;
	std::vector<std::pair<float, float>> wall_positions = {
		std::pair(100.f, 340.f),
		std::pair(250.f, 200.f),
		std::pair(-100.f, -100.f),
	};
	for (auto p : wall_positions) {
		objs.emplace_back(MapObject(p.first, p.second, SPRITE::WALL_SPRITE));	
	}
	return objs;
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

	// TODO: I'm putting a temporary fix here so we won't be spawning too many clones together
	// Ideally we want clone placement to be signalled by key release
	for (Player &player : players) {
		int clone_num = 0;
		for (Clone clone : common_data->clones) {
			if (clone.player_id == player.player_id) {
				clone_num++;
			}
		}
		if (player.mouse.pressed && clone_num < NUM_CLONES) {
			bool overlap = false;
			for (Clone &clone : common_data->clones) {
				if (sqrt((player.mouse_x-clone.x)*(player.mouse_x-clone.x)+(player.mouse_y-clone.y)*(player.mouse_y-clone.y) < 50)) {
					overlap = true;
				}
			}
			if (!overlap) {
				player.place_clone();
			}
		}
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
		if (player.mouse.pressed && player.shoot_interval < 0) {
			player.shoot();
			player.shoot_interval = BULLET_INTERVAL;
		}
	}
}

void Game::update(float elapsed) {
	// wait if players haven't arrived yet
	if (player_cnt < 2 && !single_player) {
		return;
	}

	for (Player &player : players) {
		glm::vec2 dir = glm::vec2(0.0f, 0.0f);
		if (player.left.pressed) dir.x -= 1.0f;
		if (player.right.pressed) dir.x += 1.0f;
		if (player.down.pressed) dir.y -= 1.0f;
		if (player.up.pressed) dir.y += 1.0f;

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

	int at = 6;
	auto read = [&](auto *val) {
		if (at + sizeof(*val) > size) {
			throw std::runtime_error("Ran out of bytes reading state message.");
		}
		std::memcpy(val, &recv_buffer[4 + at], sizeof(*val));
		at += sizeof(*val);
	};

	player_id = recv_buffer[4+0];
	recv_button(recv_buffer[4+1], &left);
	recv_button(recv_buffer[4+2], &right);
	recv_button(recv_buffer[4+3], &up);
	recv_button(recv_buffer[4+4], &down);
	recv_button(recv_buffer[4+5], &mouse);

	read(&mouse_x);
	read(&mouse_y);

	//delete message from buffer:
	recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);
	return true;
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

	connection.send(state);
	connection.send(time_remaining);

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
	read(&time_remaining);

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

bool Game::client_recv_message(Connection *connection_, Player *client_player) {
	assert(connection_);
	auto &connection = *connection_;
	auto &recv_buffer = connection.recv_buffer;
	if (recv_buffer.size() < 4) return false;
	if (recv_buffer[0] == uint8_t(Message::S2C_Setup)) {
		return recv_setup_message(connection_, client_player);
	}
	if (recv_buffer[0] == uint8_t(Message::S2C_State)) {
		return recv_state_message(connection_);
	}
	// not a matching tag
	std::cout << "No matching tag, probably an error here\n";
	return false;
}
