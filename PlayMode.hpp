#pragma once

#include "Mode.hpp"

#include "Connection.hpp"
#include "Game.hpp"
#include "ImageRenderer.hpp"
#include "TextRenderer.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode(Client &client);
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	void world_to_opengl(float world_x, float world_y, glm::uvec2 const &screen_size, float& screen_x, float& screen_y);
	void screen_to_world(float screen_x, float screen_y, glm::uvec2 const &screen_size, float& world_x, float& world_y);

	//----- game state -----
	ImageRenderer img_renderer;
	TextRenderer text_renderer;
	int8_t player_id = -1;
	Player *player = nullptr;
	Character *character = nullptr;
	bool single_player = false;

	CommonData *common_data;

	//latest game state (from server):
	Game game;

	//last message from server:
	std::string server_message;

	//connection to server:
	Client &client;

};
