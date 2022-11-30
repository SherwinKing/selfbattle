#pragma once

#include "ImageRenderer.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"
#include "Entity.hpp"
#include "CommonData.hpp"
#include "Map.hpp"

#include <glm/glm.hpp>

#include <stdint.h>
#include <list>
#include <array>
#include <vector>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <cassert>
#include <random>
#include <deque>

#include <iostream>
#include <fstream>
#include <chrono>

#define SPRITE_DATA std::vector< glm::u8vec4 >

struct Connection;

//Game state, separate from rendering.

enum class MESSAGE : uint8_t {
	MSG_NONE = 0,
	SERVER_INIT = '1',
	PLAYER_INPUT = '2',
	PLAYER_READY = '3',
	SERVER_READY = '4',
	PLAYER_UPDATE = '5',
};

enum GameState : uint8_t {
	GamePaused, // A count down or something
	GameStarting,
	PlaceClones,
	FindClones,
	KillClones,
	GameOver
};

//used to represent a control input:
struct Button {
	enum State {
		NONE,
		BTN_DOWN,
		BTN_IS_PRESSED,
		BTN_RELEASE,
	};

	State state = State::NONE;
};

struct Player {
    Player() = default;
	Player(int8_t player_id) {
		this->player_id = player_id;
	}

	int8_t player_id = -1;
	Button left, right, up, down, mouse;
	float mouse_x;
	float mouse_y;
	float shoot_interval = 0;
	bool ready = false;
	// time where player input was last updated
	std::chrono::time_point<std::chrono::system_clock> time_updated;

	void update(float elapsed);
	void set_position(float new_x, float new_y);
	void place_clone();
	void try_shooting (); 
	void read_player_data(const Player &other_player);
	glm::vec2 get_direction();
};

struct MessageInfo {

	MESSAGE tag;
	// id of the message's owner
	// server will broadcast a message to everyone except its owner
	int8_t player_id;

	MessageInfo();
	MessageInfo(MESSAGE tag, int8_t id) {
		this->tag = tag;
		player_id = id;
	}
};

//state of one player in the game:

struct Game {
	std::vector<Player> players;
	std::deque<MessageInfo> message_queue;
	std::deque<MessageInfo> action_queue;

	Player *spawn_player(); //add player the end of the players list (may also, e.g., play some spawn anim)
	void remove_player(Player *); //remove player from game (may also, e.g., play some despawn anim)

	std::mt19937 mt; //used for spawning players
	uint32_t player_cnt = 0; //used for naming players

	CommonData *common_data;

	GameState state = PlaceClones;
	bool ready = false;
	bool bg_drawn = false;

	Game();

	//state update function:
	void update(float elapsed);

	//constants:
	//the update rate on the server:
	inline static constexpr float Tick = 1.0f / 30.0f;

	float time_remaining = PLACE_CLONE_PHASE_DURATION;
	// TODO: optimize
	float time_elapsed = 0;	// Should be PLACE_CLONE_PHASE_DURATION - time_remaining. Fix later
	
	SPRITE create_start();
	SPRITE create_end();
	std::vector<MapObject> create_map();

	void update_place_clones(float elapsed);
	void update_find_clones(float elapsed);
	void update_kill_clones(float elapsed);

	void setup_place_clones();
	void setup_find_clones();
	void setup_kill_clones();

	int get_section_index(float x, float y);
	bool is_in_section(int section_index, float x, float y);


	//---- communication helpers ----
	void send_message(Connection *connection_, Player *client_player, MESSAGE message_type) const;
	MESSAGE recv_message(Connection *connection_, Player *client_player, bool is_server);
	void process_action(Player *player, MESSAGE message_type);

	private:
		void update_animations(float elapsed);
};
