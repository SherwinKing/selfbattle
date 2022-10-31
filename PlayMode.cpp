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
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (game.state == GameStarting || game.state == GamePaused || game.state == GameOver) {
		return false;	
	}
	float screen_x = static_cast<float>(evt.button.x);
	float screen_y = static_cast<float>(evt.button.y);
	if (evt.type == SDL_MOUSEBUTTONUP) {
		// TODO: record number of mouse_downs
		// player.mouse.downs += 1;
		player.mouse.pressed = true;
		// save position of mouse on the player
		screen_to_world(screen_x, screen_y, window_size, player.mouse_x, player.mouse_y);
		return true;
	}
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_a) {
			// player.left.downs += 1;
			player.left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			// player.right.downs += 1;
			player.right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			// player.up.downs += 1;
			player.up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			// player.down.downs += 1;
			player.down.pressed = true;
			return true;
		}
	} 
	else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			player.left.downs = 0;
			player.left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			player.right.downs = 0;
			player.right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			player.up.downs = 0;
			player.up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			player.down.downs = 0;
			player.down.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	// queue data for sending to server:
	player.send_player_message(&client.connection);

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
					handled_message = false;
					if (game.client_recv_message(c, &player)) handled_message = true;
				} while (handled_message);
			} catch (std::exception const &e) {
				std::cerr << "[" << c->socket << "] malformed message from server: " << e.what() << std::endl;
				//quit the game:
				throw e;
			}
		}
	}, 0.0);

	assert(player.player_id <= common_data->characters.size() - 1);
	if (player.player_id >= 0) {
		character = common_data->characters[player.player_id];
	}
}

void PlayMode::world_to_opengl(float world_x, float world_y, glm::uvec2 const &screen_size, float& screen_x, float& screen_y) {
	float w = static_cast<float>(screen_size.x);
	float h = static_cast<float>(screen_size.y);

	// Between -1 and 1	
	screen_x = ((world_x - character.x) / w) * 2.f;	
	screen_y = ((world_y - character.y) / h) * 2.f;
}

void PlayMode::screen_to_world(float screen_x, float screen_y, glm::uvec2 const &screen_size, float& world_x, float& world_y) {
	float w = static_cast<float>(screen_size.x);
	float h = static_cast<float>(screen_size.y);

	float center_x = w / 2.f;
	float center_y = h / 2.f;
	world_x = character.x + (screen_x - center_x); 
	world_y = character.y - (screen_y - center_y);
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	
	// player data not in the system yet
	if (player.player_id == -1) {
		// TODO: maybe consider rendering something here
		return;
	}


	auto draw_entity = [&] (Entity &entity) {
		float lower_x;
		float lower_y;
		entity.get_lower_left(lower_x, lower_y);
		float screen_x;
		float screen_y;
		world_to_opengl(lower_x, lower_y, drawable_size, screen_x, screen_y);
		renderer.render_image(common_data->sprites[entity.sprite_index], screen_x, screen_y);
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

	GL_ERRORS();
}
