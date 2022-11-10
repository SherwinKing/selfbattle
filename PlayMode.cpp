#include "PlayMode.hpp"

#include "DrawLines.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <random>
#include <array>

PlayMode::PlayMode(Client &client_) : client(client_) {
	common_data = CommonData::get_instance();
	text_renderer = TextRenderer("font/Roboto/Roboto-Regular.ttf");
	if (single_player) {
		std::cout << "Playing in Single Player Mode.\n";
	}
	else {
		std::cout << "Playing in Miltiplayer Mode.\n";
	}
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (player == nullptr || !player->ready) {
		if (evt.type == SDL_KEYDOWN || evt.type == SDL_MOUSEBUTTONDOWN) {
			player->ready = true;
			game.send_message(&client.connection, player, MESSAGE::PLAYER_READY);
			return true;
		}
	}

	if (!game.ready) {
		return false;
	}

	if (game.state == GameStarting || game.state == GamePaused || game.state == GameOver) {
		return false;
	}

	// TODO: store info to message queue
	if (evt.type == SDL_MOUSEBUTTONDOWN) {
		player->mouse.state = Button::BTN_DOWN;
		// save position of mouse on the player
		float screen_x = (float)evt.button.x;
		float screen_y = (float)evt.button.y;
		screen_to_world(screen_x, screen_y, window_size, player->mouse_x, player->mouse_y);
		game.send_message(&client.connection, player, MESSAGE::PLAYER_INPUT);
		return true;
	}
	if (evt.type == SDL_MOUSEBUTTONUP) {
		player->mouse.state = Button::BTN_RELEASE;
		game.send_message(&client.connection, player, MESSAGE::PLAYER_INPUT);
		return true;
	}
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_a) {
			player->left.state = Button::BTN_DOWN;
			game.send_message(&client.connection, player, MESSAGE::PLAYER_INPUT);
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			player->right.state = Button::BTN_DOWN;
			game.send_message(&client.connection, player, MESSAGE::PLAYER_INPUT);
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			player->up.state = Button::BTN_DOWN;
			game.send_message(&client.connection, player, MESSAGE::PLAYER_INPUT);
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			player->down.state = Button::BTN_DOWN;
			game.send_message(&client.connection, player, MESSAGE::PLAYER_INPUT);
			return true;
		}
	} 
	else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			player->left.state = Button::BTN_RELEASE;
			game.send_message(&client.connection, player, MESSAGE::PLAYER_INPUT);
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			player->right.state = Button::BTN_RELEASE;
			game.send_message(&client.connection, player, MESSAGE::PLAYER_INPUT);
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			player->up.state = Button::BTN_RELEASE;
			game.send_message(&client.connection, player, MESSAGE::PLAYER_INPUT);
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			player->down.state = Button::BTN_RELEASE;
			game.send_message(&client.connection, player, MESSAGE::PLAYER_INPUT);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	if (single_player) {
		player = &game.players[0];
		character = &common_data->characters[0];
		player_id = 0;
		player->ready = true;
		game.ready = true;
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
	world_x = character->x + (screen_x - center_x); 
	world_y = character->y - (screen_y - center_y);
	// std::cout << "input: " << std::to_string(screen_x) << ", " << std::to_string(screen_y) << "\n";
	// std::cout << "output: " << std::to_string(world_x) << ", " << std::to_string(world_y) << "\n";
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {

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

	if (!game.ready) {
		text_renderer.render_text("Press any key to start", -0.5f, 0.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 80);
		return;
	}


	auto draw_entity = [&] (Entity &entity) {
		float screen_x;
		float screen_y;
		world_to_opengl(entity.x, entity.y, drawable_size, screen_x, screen_y);
		img_renderer.render_image(common_data->sprites[entity.sprite_index], screen_x, screen_y);
	};

	for (Character c : common_data->characters) {
		draw_entity(c);
	}

	for (Bullet bullet : common_data->bullets) {
		draw_entity(bullet);
	}

	for (Clone clone : common_data->clones) {
		draw_entity(clone);
	}

	for (MapObject map_obj : common_data->map_objects) {
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

	text_renderer.render_text(game_state_text, -0.7f, 0.7f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 80);

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
	
	std::string time_text = float_to_string(game.time_remaining);
	text_renderer.render_text(time_text, 0.5f, 0.7f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 80);

	std::string hp_text = "HP: " + float_to_string(character->hp);
	text_renderer.render_text(hp_text, -0.7f, -0.7f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 80);

	GL_ERRORS();
}
