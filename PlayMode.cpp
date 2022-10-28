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
	player.init();
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (player.state == GameStarting || player.state == GamePaused || player.state == GameOver) {
		return false;	
	}
	float screen_x = static_cast<float>(evt.button.x);
	float screen_y = static_cast<float>(evt.button.y);
	if (evt.type == SDL_MOUSEBUTTONUP) {
		switch (player.state) {
			case PlaceClones: 
				if (player.clones.size() >= NUM_CLONES) {
					return false;	
				}
				player.place_clone(screen_x, screen_y, window_size);	
				return true;
			case KillClones:
				player.shoot(screen_x, screen_y, window_size);
				return true;
			default:
				return false;	
		}	
	}
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_a) {
			player.move_player(-1.0 * PLAYER_SPEED, 0.0);	
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			player.move_player(1.0 * PLAYER_SPEED, 0.0);	
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			player.move_player(0.0, 1.0 * PLAYER_SPEED);	
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			player.move_player(0.0, -1.0 * PLAYER_SPEED);	
			return true;
		} 
		if (evt.key.keysym.sym == SDLK_SPACE) {
			switch(player.state) {
				case PlaceClones:
					printf("Switched to find clones state\n");
					player.state = FindClones;
					return true;
				case FindClones:
					printf("Switched to kill clones state\n");
					player.state = KillClones;
					return true;
				case KillClones:	
					printf("Switched to game over state\n");
					player.state = GameOver;
					return true;
			}
		}
		return false;
	}
	return false;
}

void PlayMode::update(float elapsed) {
	player.update(elapsed);
	/*
	//send/receive data:
	client.poll([this](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
			std::cout << "[" << c->socket << "] opened" << std::endl;
		} else if (event == Connection::OnClose) {
			std::cout << "[" << c->socket << "] closed (!)" << std::endl;
			throw std::runtime_error("Lost connection to server!");
		} else { assert(event == Connection::OnRecv);
			//std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush(); //DEBUG
			bool handled_message;
			try {
				do {
					handled_message = false;
					if (game.recv_state_message(c)) handled_message = true;
				} while (handled_message);
			} catch (std::exception const &e) {
				std::cerr << "[" << c->socket << "] malformed message from server: " << e.what() << std::endl;
				//quit the game:
				throw e;
			}
		}
	}, 0.0);
	*/
}



void PlayMode::draw(glm::uvec2 const &drawable_size) {

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	
	player.draw(drawable_size);
	GL_ERRORS();
}
