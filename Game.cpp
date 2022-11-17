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
		std::pair(SPRITE::PLAYER_SPRITE_RED, "sprites/player_sprite_red.png"),
		std::pair(SPRITE::PLAYER_SPRITE_BLUE, "sprites/player_sprite_blue.png"),
		std::pair(SPRITE::CLONE_SPRITE_RED, "sprites/clone_sprite_red.png"),
		std::pair(SPRITE::CLONE_SPRITE_BLUE, "sprites/clone_sprite_blue.png"),
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
		std::pair(SPRITE::FENCE_T_T, "sprites/fence_t_t.png"),
		std::pair(SPRITE::FENCE_T_R, "sprites/fence_t_r.png"),
		std::pair(SPRITE::FENCE_T_B, "sprites/fence_t_b.png"),
		std::pair(SPRITE::FENCE_T_L, "sprites/fence_t_l.png"),
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
	common_data->characters.emplace_back( Character(PLAYER0_STARTING_X, PLAYER0_STARTING_Y, SPRITE::PLAYER_SPRITE_RED, 0) );
	common_data->characters.emplace_back( Character(PLAYER1_STARTING_X, PLAYER1_STARTING_Y, SPRITE::PLAYER_SPRITE_BLUE, 1) );

	players.reserve(2);
	players.emplace_back(Player(0));
	players.emplace_back(Player(1));
}

std::vector<MapObject> Game::create_map() {
	std::vector<MapObject> objs;
	struct wp {
		wp(float x, float y, SPRITE s) {
			this->x = x;
			this->y = y;
			this->s = s;
		}
		float x;
		float y;
		SPRITE s;
	};
	auto rw1 = SPRITE::CLOCK_1;
	auto rw2 = SPRITE::CLOCK_2;
	auto rw3 = SPRITE::CLOCK_3;
	auto fctr = SPRITE::FENCE_CORNER_TR;
	auto ffh = SPRITE::FENCE_FULL_H;
	auto ffv = SPRITE::FENCE_FULL_V;
	auto fcrb = SPRITE::FENCE_CORNER_RB;
	auto fcbl = SPRITE::FENCE_CORNER_BL;
	auto fclt = SPRITE::FENCE_CORNER_LT;
	auto fsh = SPRITE::FENCE_SELF_H;
	auto fsv = SPRITE::FENCE_SELF_V;
	auto fht = SPRITE::FENCE_HALF_T;
	auto fhr = SPRITE::FENCE_HALF_R;
	auto fhb = SPRITE::FENCE_HALF_B;
	auto fhl = SPRITE::FENCE_HALF_L;
	auto ftt = SPRITE::FENCE_T_T;
	// auto ftr = SPRITE::FENCE_T_R;
	auto ftb = SPRITE::FENCE_T_B;
	auto ftl = SPRITE::FENCE_T_L;

	std::vector<wp> walls = {
		// RED
		// 2
		wp(600.f, -200.f, rw1),
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

		
		// ---------------------------------------------------------------------
		// BLUE
		// 2
		wp(-600.f, -200.f, fctr),
		wp(-400.f, -200.f, ffh),
		wp(-200.f, -200.f, fhl),

		// 10
		wp(-600.f, 0.f, ffv),

		// 4
		wp(-600.f, 200.f, fcrb),
		wp(-400.f, 200.f, ffh),
		wp(-200.f, 200.f, fhl),

		// 1
		wp(-600.f, -800.f, fcrb),
		wp(-400.f, -800.f, ffh),
		wp(-200.f, -800.f, fhl),

		// 9
		wp(-600.f, -1000.f, ffv),
		wp(-600.f, -1200.f, ffv),
		wp(-600.f, -1400.f, fclt),

		// 5
		wp(-600.f, 800.f, ffh),
		wp(-400.f, 800.f, ffh),
		wp(-200.f, 800.f, fhl),

		// 11
		wp(-800.f, 800.f, fctr),
		wp(-800.f, 1000.f, ffv),
		wp(-800.f, 1200.f, ffv),
		wp(-800.f, 1400.f, fcbl),

		// 6
		wp(-1000.f, 1400.f, ffh),
		wp(-1200.f, 1400.f, ffh),
		wp(-1400.f, 1400.f, ftb),
		wp(-1600.f, 1400.f, ffh),
		wp(-1800.f, 1400.f, ffh),
		wp(-2000.f, 1400.f, ffh),
		wp(-2200.f, 1400.f, ffh),
		wp(-2400.f, 1400.f, ffh),
		wp(-2600.f, 1400.f, ffh),
		wp(-2800.f, 1400.f, ffh),
		wp(-3000.f, 1400.f, ftb),
		wp(-3200.f, 1400.f, ffh),
		wp(-3400.f, 1400.f, ffh),
		wp(-3600.f, 1400.f, ffh),
		wp(-3800.f, 1400.f, ffh),
		wp(-4000.f, 1400.f, ffh),
		wp(-4200.f, 1400.f, ffh),
		wp(-4400.f, 1400.f, ffh),
		wp(-4600.f, 1400.f, ffh),
		wp(-4800.f, 1400.f, ffh),

		// 8
		wp(-800.f, -1400.f, ffh),
		wp(-1000.f, -1400.f, ffh),
		wp(-1200.f, -1400.f, ffh),
		wp(-1400.f, -1400.f, ffh),
		wp(-1600.f, -1400.f, ffh),
		wp(-1800.f, -1400.f, ffh),
		wp(-2000.f, -1400.f, ffh),
		wp(-2200.f, -1400.f, ffh),
		wp(-2400.f, -1400.f, ffh),
		wp(-2600.f, -1400.f, ffh),
		wp(-2800.f, -1400.f, ftt),
		wp(-3000.f, -1400.f, ffh),
		wp(-3200.f, -1400.f, ffh),
		wp(-3400.f, -1400.f, ffh),
		wp(-3600.f, -1400.f, ffh),
		wp(-3800.f, -1400.f, ffh),
		wp(-4000.f, -1400.f, ffh),
		wp(-4200.f, -1400.f, ffh),
		wp(-4400.f, -1400.f, ffh),
		wp(-4600.f, -1400.f, ffh),
		wp(-4800.f, -1400.f, ffh),
		
		// 12
		wp(-1200.f, 600.f, fhb),
		wp(-1200.f, 400.f, ffv),
		wp(-1200.f, 200.f, fclt),

		// 13
		wp(-1400.f, 200.f, ffh),
		wp(-1600.f, 200.f, fhr),

		// 14
		wp(-1200.f, -800.f, fclt),
		wp(-1200.f, -600.f, ffv),
		wp(-1200.f, -400.f, ffv),
		wp(-1200.f, -200.f, fhb),


		// 15
		wp(-1400.f, -800.f, ffh),
		wp(-1600.f, -800.f, ffh),
		wp(-1800.f, -800.f, ffh),
		wp(-2000.f, -800.f, ffh),
		wp(-2200.f, -800.f, ffh),
		wp(-2400.f, -800.f, fhr),

		// 16
		wp(-2000.f, -400.f, fhl),
		wp(-2200.f, -400.f, ffh),
		wp(-2400.f, -400.f, fctr),

		// 17
		wp(-2400.f, -200.f, ffv),
		wp(-2400.f, 0.f, ftl),
		wp(-2400.f, 200.f, ffv),
		wp(-2400.f, 400.f, ffv),
		wp(-2400.f, 600.f, ffv),
		wp(-2400.f, 800.f, ffv),
		wp(-2400.f, 1000.f, fcrb),
		// 18
		wp(-2200.f, 1000.f, ffh),
		wp(-2000.f, 1000.f, ffh),
		wp(-1800.f, 1000.f, fhl),

		// 19
		wp(-1600.f, -300.f, fsv),

		// 3
		wp(-1400.f, 1200.f, fht),

		// 19.2
		wp(-2600.f, 0.f, ffh),
		wp(-2800.f, 0.f, ffh),
		wp(-3000.f, 0.f, ffh),
		wp(-3200.f, 0.f, fcrb),

		// 20
		wp(-3200.f, -200.f, ffv),
		wp(-3200.f, -400.f, ffv),
		wp(-3200.f, -600.f, ffv),
		wp(-3200.f, -800.f, fht),

		// 21
		wp(-2800.f, -1200.f, ffv),
		wp(-2800.f, -1000.f, fhb),

		// 22.1
		wp(-2800.f, -400.f, fsh),

		// 22
		wp(-3200.f, 400.f, fsh),

		// 23
		wp(-3000.f, 800.f, fht),
		wp(-3000.f, 1000.f, ffv),
		wp(-3000.f, 1200.f, ffv),

		// 24
		wp(-3600.f, 1000.f, fhl),
		wp(-3800.f, 1000.f, ffh),
		wp(-4000.f, 1000.f, ffh),

		// 25
		wp(-4200.f, 800.f, fht),
		wp(-4200.f, 1000.f, fcrb),

		// 26
		wp(-4200.f, 400.f, fsh),
		wp(-4200.f, -200.f, fsh),
		wp(-4200.f, -800.f, fsh),
		wp(-3700.f, 400.f, fsv),
		wp(-3700.f, -200.f, fsv),
		wp(-3700.f, -800.f, fsv),

		// 7
		wp(-5000.f, 1400.f, fcrb),
		wp(-5000.f, 1200.f, ffv),
		wp(-5000.f, 1000.f, ffv),
		wp(-5000.f, 800.f, ffv),
		wp(-5000.f, 600.f, ffv),
		wp(-5000.f, 400.f, ffv),
		wp(-5000.f, 200.f, ffv),
		wp(-5000.f, 0.f, ffv),
		wp(-5000.f, -200.f, ffv),
		wp(-5000.f, -400.f, ffv),
		wp(-5000.f, -600.f, ffv),
		wp(-5000.f, -800.f, ffv),
		wp(-5000.f, -1000.f, ffv),
		wp(-5000.f, -1200.f, ffv),
		wp(-5000.f, -1400.f, fctr),
		

	};
	for (auto p : walls) {
		objs.emplace_back(MapObject(p.x, p.y, p.s));	
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
	SPRITE clone_sprite;
	// determine clone color by player id: 0 for red, 1 for blue
	if (player_id == 0) {
		clone_sprite = SPRITE::CLONE_SPRITE_RED;
	} else {
		clone_sprite = SPRITE::CLONE_SPRITE_BLUE;
	}
	Clone clone = Clone(mouse_x, mouse_y, clone_sprite, player_id); 
	CommonData::get_instance()->clones.emplace_back(clone);	
}

void Player::read_player_data(const Player &other_player) {
	this->player_id = other_player.player_id;
	this->left = other_player.left;
	this->right = other_player.right;
	this->up = other_player.up;
	this->down = other_player.down;
	this->mouse = other_player.mouse;
	this->mouse_x = other_player.mouse_x;
	this->mouse_y = other_player.mouse_y;
}

glm::vec2 Player::get_direction() {
	glm::vec2 dir = glm::vec2(0.0f, 0.0f);
	if (left.state == Button::BTN_IS_PRESSED) dir.x -= 1.0f;
	if (right.state == Button::BTN_IS_PRESSED) dir.x += 1.0f;
	if (down.state == Button::BTN_IS_PRESSED) dir.y -= 1.0f;
	if (up.state == Button::BTN_IS_PRESSED) dir.y += 1.0f;

	if (dir.x != 0 || dir.y != 0) {
		dir = glm::normalize(dir);
	}

	return dir;
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
	time_elapsed += elapsed;
	if (time_remaining < 0) {
		setup_find_clones();
		return;
	}

	// Record character snapshot
	for (Character &c : common_data->characters) {
		c.phase1_replay_buffer.emplace_back(c.x, c.y, time_elapsed);
		c.phase1_replay_buffer_2.emplace_back(c.x, c.y, time_elapsed);
	}
}

void Game::setup_find_clones() {
	time_elapsed = 0;
	state = FindClones;
	time_remaining = FIND_CLONE_PHASE_DURATION;
	common_data->characters[0].x = PLAYER1_STARTING_X;
	common_data->characters[0].y = PLAYER1_STARTING_Y;
	common_data->characters[1].x = PLAYER0_STARTING_X;
	common_data->characters[1].y = PLAYER0_STARTING_Y;

	// Setup the shadows
	common_data->shadows.resize(common_data->characters.size());
	for (Character &c : common_data->characters) {
		SPRITE shadow_sprite;
		// determine clone color by player id: 0 for red, 1 for blue
		if (c.player_id == 0) {
			shadow_sprite = SPRITE::CLONE_SPRITE_RED;
		} else {
			shadow_sprite = SPRITE::CLONE_SPRITE_BLUE;
		}
		const auto & first_shadow_snapshot = c.phase1_replay_buffer[0];
		common_data->shadows[c.player_id] = Shadow(first_shadow_snapshot.x, first_shadow_snapshot.y,  shadow_sprite, c.player_id);
	}
}

void Game::update_find_clones(float elapsed) {
	time_remaining -= elapsed;
	time_elapsed += elapsed;
	if (time_remaining < 0) {
		setup_kill_clones();
		return;
	}

	// Replay character in phase 1 as shadow
	for (Character &c : common_data->characters) {
		while (c.phase1_replay_buffer.size() > 0) {
			const auto & first_snapshot = c.phase1_replay_buffer[0];

			// Stop replaying if we've reached the current time
			if (first_snapshot.timestamp > time_elapsed) {
				break;
			}

			// Replay the snapshot
			common_data->shadows[c.player_id].x = first_snapshot.x;
			common_data->shadows[c.player_id].y = first_snapshot.y;

			// Remove the snapshot
			c.phase1_replay_buffer.pop_front();
		}
	}
}

void Game::setup_kill_clones() {
	state = KillClones;
	time_remaining = KILL_CLONE_PHASE_DURATION;
	common_data->characters[0].x = PLAYER1_STARTING_X;
	common_data->characters[0].y = PLAYER1_STARTING_Y;
	common_data->characters[1].x = PLAYER0_STARTING_X;
	common_data->characters[1].y = PLAYER0_STARTING_Y;

	time_elapsed = 0;

	// // Setup the shadows
	// common_data->shadows.resize(common_data->characters.size());
	// for (Character &c : common_data->characters) {
	// 	SPRITE shadow_sprite;
	// 	// determine clone color by player id: 0 for red, 1 for blue
	// 	if (c.player_id == 0) {
	// 		shadow_sprite = SPRITE::CLONE_SPRITE_RED;
	// 	} else {
	// 		shadow_sprite = SPRITE::CLONE_SPRITE_BLUE;
	// 	}
	// 	const auto & first_shadow_snapshot = c.phase1_replay_buffer[0];
	// 	common_data->shadows[c.player_id] = Shadow(first_shadow_snapshot.x, first_shadow_snapshot.y,  shadow_sprite, c.player_id);
	// }
}

void Game::update_kill_clones(float elapsed) {
	time_remaining -= elapsed;	
	time_elapsed += elapsed;
	if (time_remaining < 0) {
		printf("Time is up!\n");
		state = GameOver;
		return;
	}

	// Replay character in phase 1 as shadow
	for (Character &c : common_data->characters) {
		while (c.phase1_replay_buffer_2.size() > 0) {
			const auto & first_snapshot = c.phase1_replay_buffer_2[0];

			// Stop replaying if we've reached the current time
			if (first_snapshot.timestamp > time_elapsed) {
				break;
			}

			// Replay the snapshot
			common_data->shadows[c.player_id].x = first_snapshot.x;
			common_data->shadows[c.player_id].y = first_snapshot.y;

			// Remove the snapshot
			c.phase1_replay_buffer_2.pop_front();
		}
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

				if (clone.hp <= 0) {
					common_data->characters[clone.player_id].score++;
				}
			}
		}
		for (Shadow &shadow : common_data->shadows) {
			if (bullet.collide(shadow)) {
				shadow.take_damage(BULLET_DAMAGE);
				bullet.active = false;

				if (shadow.hp <= 0) {
					common_data->characters[shadow.player_id].score++;
				}
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

	common_data->shadows.erase(
		std::remove_if(common_data->shadows.begin(),
					   common_data->shadows.end(),
					   [](Clone clone){return clone.hp <= 0;}),
		common_data->shadows.end()
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
				if (clone_num < NUM_CLONES) {
					player.place_clone();
				}
			}
			if (state == KillClones) {
				player.try_shooting();
			}
		}

		glm::vec2 dir = player.get_direction();
		common_data->characters[player.player_id].move_character(dir.x * PLAYER_SPEED * elapsed, dir.y * PLAYER_SPEED * elapsed);
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
		connection.send(character.hp);
		connection.send(character.rotation);
	};

	auto send_player = [&](Player const &player) {
		connection.send(player.player_id);
		connection.send(player.left.state);
		connection.send(player.right.state);
		connection.send(player.up.state);
		connection.send(player.down.state);
		connection.send(player.mouse.state);
		connection.send(player.mouse_x);
		connection.send(player.mouse_y);
		connection.send(player.time_updated);
	};

	switch(message_type) {
		case MESSAGE::SERVER_INIT:
			std::cout << "send server_init\n";
			connection.send(connection_player->player_id);
			break;
		case MESSAGE::PLAYER_INPUT:
			// std::cout << "send player input\n";
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
		case MESSAGE::PLAYER_UPDATE:
			// connection.send(common_data->characters[connection_player->player_id].rotation);
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
		read(&character->hp);
		read(&character->rotation);
	};

	auto read_player = [&](Player *player) {
		read(&player->player_id);
		read(&player->left.state);
		read(&player->right.state);
		read(&player->up.state);
		read(&player->down.state);
		read(&player->mouse.state);
		read(&player->mouse_x);
		read(&player->mouse_y);
		read(&player->time_updated);
	};

	MESSAGE message_type = (MESSAGE) recv_buffer[0];
	switch(message_type) {
		case MESSAGE::SERVER_INIT:
			std::cout << "read server_init\n";
			assert(!is_server);
			read(&client_player->player_id);
			break;
		case MESSAGE::PLAYER_INPUT: {
			// std::cout << "read player_input\n";
			read_player(client_player);
			read_character(&common_data->characters[client_player->player_id]);
			if (is_server) {
				message_queue.push_back(MessageInfo(MESSAGE::PLAYER_INPUT, client_player->player_id));
			}
			// adjust character position for network latency
			// std::chrono::time_point<std::chrono::system_clock> client_tp = client_player->time_updated;
			// std::chrono::duration latency = std::chrono::system_clock::now() - client_tp;
			// std::chrono::duration<float> f_latency = latency;
			// std::cout << "latency is: " << f_latency.count() << "\n";
			// glm::vec2 displacement = client_player->get_direction() * f_latency.count() * PLAYER_SPEED;
			// common_data->characters[client_player->player_id].move_character(displacement.x, displacement.y);
			break;
		}
		case MESSAGE::PLAYER_READY:
			std::cout << "read player_ready\n";
			assert(is_server);
			client_player->ready = true;
			if (players[0].ready && players[1].ready) {
				ready = true;
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
		case MESSAGE::PLAYER_UPDATE:
			// read(&common_data->characters[client_player->player_id].rotation);
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
