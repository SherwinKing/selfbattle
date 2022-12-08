#include "PlayMode.hpp"

#include "DrawLines.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"
#include "Sound.hpp"
#include "Load.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <random>
#include <array>
#include <chrono>

Load< Sound::Sample > background_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("audio/funshine.wav"));
});

PlayMode::PlayMode(Client &client_) : client(client_) {
	common_data = CommonData::get_instance();
	text_renderer = TextRenderer("font/Roboto/Roboto-Regular.ttf");
	background_music = Sound::loop(*background_sample, VOLUME, 0);
	if (single_player) {
		std::cout << "Playing in Single Player Mode.\n";
		player = &game.players[0];
		character = &common_data->characters[0];
		player_id = 0;
		player->ready = true;
		game.ready = true;
	}
	else {
		std::cout << "Playing in Multiplayer Mode.\n";
	}
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	// this function assumes that we are sending a full message
	// which includes all player inputs, player position, and system time
	auto send_message_to_server = [&] (MESSAGE message) {
		// change this assert if needed
		assert(message == MESSAGE::PLAYER_READY || message == MESSAGE::PLAYER_INPUT);
		player->time_updated = std::chrono::system_clock::now();
		game.send_message(&client.connection, player, message);
	};

	if (player == nullptr || !player->ready) {
		if (evt.type == SDL_KEYDOWN || evt.type == SDL_MOUSEBUTTONDOWN) {
			player->ready = true;
			send_message_to_server(MESSAGE::PLAYER_READY);
			return true;
		}
	}
	
	if (!game.ready) {
		return false;
	}

	if (game.state == GameStarting || game.state == GamePaused || game.state == GameOver) {
		return false;
	}

	if (common_data->characters[player->player_id].dead) {
		return false;
	}
	// TODO: store info to message queue
	if (evt.type == SDL_MOUSEBUTTONDOWN) {
		player->mouse.state = Button::BTN_DOWN;
		// save position of mouse on the player
		float screen_x = (float)evt.button.x;
		float screen_y = (float)evt.button.y;
		screen_to_world(screen_x, screen_y, window_size, player->mouse_x, player->mouse_y);
		send_message_to_server(MESSAGE::PLAYER_INPUT);
		return true;
	}
	if (evt.type == SDL_MOUSEBUTTONUP) {
		player->mouse.state = Button::BTN_RELEASE;
		send_message_to_server(MESSAGE::PLAYER_INPUT);
		return true;
	}
	if (evt.type == SDL_MOUSEMOTION) {
		// only process this if after the game starts
		if (game.ready) {
			float center_x = static_cast<float>(window_size.x) / 2.f;
			float center_y = static_cast<float>(window_size.y) / 2.f;
			float rel_x = center_x - evt.button.x;
			float rel_y = center_y - evt.button.y;
			common_data->characters[player_id].rotation = atan2f(rel_x, rel_y);
			return true;
		}
	}
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_a) {
			if (player->left.state != Button::BTN_DOWN 
			 && player->left.state != Button::BTN_IS_PRESSED) {
				player->left.state = Button::BTN_DOWN;
				send_message_to_server(MESSAGE::PLAYER_INPUT);
				return true;
			}
		} else if (evt.key.keysym.sym == SDLK_d) {
			if (player->right.state != Button::BTN_DOWN 
			 && player->right.state != Button::BTN_IS_PRESSED) {
				player->right.state = Button::BTN_DOWN;
				send_message_to_server(MESSAGE::PLAYER_INPUT);
				return true;
			}
		} else if (evt.key.keysym.sym == SDLK_w) {
			if (player->up.state != Button::BTN_DOWN 
			 && player->up.state != Button::BTN_IS_PRESSED) {
				player->up.state = Button::BTN_DOWN;
				send_message_to_server(MESSAGE::PLAYER_INPUT);
				return true;
			}
		} else if (evt.key.keysym.sym == SDLK_s) {
			if (player->down.state != Button::BTN_DOWN 
			 && player->down.state != Button::BTN_IS_PRESSED) {
				player->down.state = Button::BTN_DOWN;
				send_message_to_server(MESSAGE::PLAYER_INPUT);
				return true;
			}
		}
	} 
	else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			player->left.state = Button::BTN_RELEASE;
			send_message_to_server(MESSAGE::PLAYER_INPUT);
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			player->right.state = Button::BTN_RELEASE;
			send_message_to_server(MESSAGE::PLAYER_INPUT);
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			player->up.state = Button::BTN_RELEASE;
			send_message_to_server(MESSAGE::PLAYER_INPUT);
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			player->down.state = Button::BTN_RELEASE;
			send_message_to_server(MESSAGE::PLAYER_INPUT);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	if (single_player) {
		game.update(elapsed);
		return;
	}

	// send/receive data:
	client.poll([this](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
			std::cout << "[" << c->socket << "] opened" << std::endl;
		} else if (event == Connection::OnClose) {
			std::cout << "[" << c->socket << "] closed (!)" << std::endl;
			throw std::runtime_error("Lost connection to server!");
		} else { assert(event == Connection::OnRecv);
			// std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush(); //DEBUG
			bool handled_message;
			try {
				do {
					handled_message = true;
					Player temp_player;
					switch(game.recv_message(c, &temp_player, false)) {
						case MESSAGE::SERVER_INIT:
							player_id = temp_player.player_id;
							player = &game.players[player_id];
							character = &common_data->characters[player_id];
							break;
						case MESSAGE::PLAYER_INPUT:
							assert(temp_player.player_id != player_id);
							assert(game.players[temp_player.player_id].player_id == temp_player.player_id);
							game.players[temp_player.player_id].read_player_data(temp_player);
							break;
						case MESSAGE::SERVER_READY:
							// all computations are already done in game
							break;
						case MESSAGE::PLAYER_UPDATE:
							// computations done in game
							break;
						default:
							handled_message = false;
							break;
					}
				} while (handled_message);
			} catch (std::exception const &e) {
				std::cerr << "[" << c->socket << "] malformed message from server: " << e.what() << std::endl;
				//quit the game:
				throw e;
			}
		}
	}, 0.0);

	if (!game.ready) {
		return;
	}
	
	// send player's rotation
	game.send_message(&client.connection, player, MESSAGE::PLAYER_UPDATE);
	game.update(elapsed);
}

void PlayMode::world_to_opengl(float world_x, float world_y, glm::uvec2 const &screen_size, float& screen_x, float& screen_y) {
	float w = static_cast<float>(screen_size.x);
	float h = static_cast<float>(screen_size.y);

	// Between -1 and 1	
	screen_x = ((world_x - character->x) / w) * 2.f;	
	screen_y = ((world_y - character->y) / h) * 2.f;
}

void PlayMode::screen_to_world(float screen_x, float screen_y, glm::uvec2 const &screen_size, float& world_x, float& world_y) {
	float w = static_cast<float>(screen_size.x);
	float h = static_cast<float>(screen_size.y);

	float center_x = w / 2.f;
	float center_y = h / 2.f;
	world_x = character->x + (screen_x - center_x) * (1920.0f / w); 
	world_y = character->y - (screen_y - center_y) * (1080.0f / h);
	// std::cout << "input: " << std::to_string(screen_x) << ", " << std::to_string(screen_y) << "\n";
	// std::cout << "output: " << std::to_string(world_x) << ", " << std::to_string(world_y) << "\n";
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {

	// formatting time manually because I wasn't able to find a library function
	auto float_to_string = [](float f) {
		std::string text = std::to_string(f);
		int period_index = 0;
		for (int i = 0; i < text.size(); i++) {
			if (text[i] == '.') {
				period_index = i;
				break;
			}
		}
		text = text.substr(0, period_index + 3);

		return text;
	};

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	img_renderer.resize(drawable_size.x, drawable_size.y);
	text_renderer.resize(drawable_size.x, drawable_size.y);
	
	// player data not in the system yet
	if (player_id == -1) {
		// TODO: maybe consider rendering something here
		return;
	}

	// game over screen
	if (game.state == GameOver) {
		Character& p1 = common_data->characters[0];
		Character& p2 = common_data->characters[1];
		img_renderer.render_image(common_data->sprites[SPRITE::END], 0.f, 0.f, 0.f);	
		text_renderer.render_text("Statistics", -0.2f, 0.4f, END_TEXT_COLOR, 100);
		text_renderer.render_text("Player 1", -0.52f, .2f, END_TEXT_COLOR, 70);
		text_renderer.render_text("Player 2", .28f, 0.2f, END_TEXT_COLOR, 70);
		
		std::string kills1 = "Kills: " + std::to_string(p1.kills);
		std::string kills2 = "Kills: " + std::to_string(p2.kills);
		text_renderer.render_text(kills1, -0.52f, 0.0f, END_TEXT_COLOR, 50);
		text_renderer.render_text(kills2, .28f, 0.0f, END_TEXT_COLOR, 50);

		std::string deaths1 = "Deaths: " + std::to_string(p1.deaths);
		std::string deaths2 = "Deaths: " + std::to_string(p2.deaths);
		text_renderer.render_text(deaths1, -0.52f, -0.2f, END_TEXT_COLOR, 50);
		text_renderer.render_text(deaths2, .28f, -0.2f, END_TEXT_COLOR, 50);

		std::string score1 = "Score: " + std::to_string(p1.score);
		std::string score2 = "Score: " + std::to_string(p2.score);
		text_renderer.render_text(score1, -0.52f, -0.4f, END_TEXT_COLOR, 50);
		text_renderer.render_text(score2, .28f, -0.4f, END_TEXT_COLOR, 50);
	}
	// Game start screen
	else if (!game.ready) {
		img_renderer.render_image(common_data->sprites[SPRITE::START], 0.f, 0.f, 0.f);	
		// text_renderer.render_text("Self Battle", -0.2f, 0.4f, START_TEXT_COLOR, 100);
		// text_renderer.render_text("A two player shooter!", -0.45f, -0.1f, START_TEXT_COLOR, 100);
		if (!player->ready) {
			text_renderer.render_text("Press _ to start", -0.27f, -0.5f, START_TEXT_COLOR, 80);
		}
		else {
			text_renderer.render_text("Waiting for the other player...", -0.5f, -0.5f, START_TEXT_COLOR, 80);
		}
	}
	// Dead
	else if (character != nullptr && common_data->characters[player->player_id].dead) {
		float time_remaining = common_data->characters[player->player_id].dead_timer;
		std::string respawn = "Respawning in " + std::to_string(time_remaining);
		text_renderer.render_text(respawn, -0.52f, 0.0f, RESPAWN_TEXT_COLOR, 50);
	} 
	// normal gameplay
	else {
		auto draw_entity = [&] (Entity &entity) {
			float screen_x;
			float screen_y;
			world_to_opengl(entity.x, entity.y, drawable_size, screen_x, screen_y);
			if (entity.anim.playing) {
				assert(entity.anim.initialized);
				img_renderer.render_image(common_data->sprites[entity.anim.animation[entity.anim.sprite_index]], screen_x, screen_y, entity.rotation);
			}
			else {
				img_renderer.render_image(common_data->sprites[entity.sprite_index], screen_x, screen_y, entity.rotation);
			}
		};

		// Draw the background before anything else (if not drawn yet)
		// Game lags with background drawing continuously so commented out for now
		/*
		if (!game.bg_drawn) {
			draw_entity(common_data->map.bg);
			game.bg_drawn = true;
		} 
		*/

		for (Character& c : common_data->characters) {
			if (!c.dead) {
				draw_entity(c);
			}
		}

		for (Bullet& bullet : common_data->bullets) {
			draw_entity(bullet);
		}

		for (Clone& clone : common_data->clones) {
			draw_entity(clone);
		}

		for (Shadow& shadow : common_data->shadows) {
			draw_entity(shadow);
		}

		for (MapObject& map_obj : common_data->map.map_objects) {
			draw_entity(map_obj);
		}

		std::string game_state_text;
		switch(game.state) {
			case PlaceClones:
				game_state_text = "Phase 1: Place clones";
				break;
			case FindClones:
				game_state_text = "Phase 2: Search";
				break;
			case KillClones:	
				game_state_text = "Phase 3: Kill clones";
				break;
			default:
				game_state_text = "Unimplemented";
				break;
		}

		// TODO: add latency

		text_renderer.render_text(game_state_text, -0.7f, 0.7f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 80);
		
		std::string time_text = float_to_string(game.time_remaining);
		text_renderer.render_text(time_text, 0.5f, 0.7f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 80);

		if (game.state == KillClones) {
			std::string hp_text = "HP: " + float_to_string(character->hp);
			text_renderer.render_text(hp_text, 0.5f, -0.5f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 80);
			
			std::string score_text = "Score: " + std::to_string(character->score);
			text_renderer.render_text(score_text, 0.5f, -0.7f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 80);

			int your_clone_cnt = 0;
			int enemy_clone_cnt = 0;
			for (Clone& clone : common_data->clones) {
				if (clone.player_id == player_id) {
					your_clone_cnt++;
				}
				else {
					enemy_clone_cnt++;
				}
			}
			for (Shadow& shadow : common_data->shadows) {
				if (shadow.player_id == player_id) {
					your_clone_cnt++;
				}
				else {
					enemy_clone_cnt++;
				}
			}
			std::string your_clone_text = "You have " + std::to_string(your_clone_cnt) + " clones left to kill!";
			text_renderer.render_text(your_clone_text, -0.7f, -0.7f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 80);

			std::string enemy_clone_text = "Protect the " + std::to_string(enemy_clone_cnt) + " enemy clones.";
			text_renderer.render_text(enemy_clone_text, -0.7f, -0.8f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 80);
		}
	}
	

	

	GL_ERRORS();
}
