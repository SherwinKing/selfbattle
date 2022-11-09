#pragma once

#include "ImageRenderer.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"
#include "Entity.hpp"
#include "CommonData.hpp"

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

#include <iostream>
#include <fstream>

#define SPRITE_DATA std::vector< glm::u8vec4 >

struct Connection;

//Game state, separate from rendering.

//Currently set up for a "client sends controls" / "server sends whole state" situation.

enum class Message : uint8_t {
	C2S_Player = '1',
	S2C_State = '2',
	S2C_Setup = '3',
};

enum GameState {
	GamePaused, // A count down or something
	GameStarting,
	PlaceClones,
	FindClones,
	KillClones,
	GameOver
};

// enum ButtonState {
// 	DOWN,
// 	PRESSED,
// 	UP,
// 	NONE
// }

//used to represent a control input:
struct Button {
	uint8_t downs = 0; //times the button has been pressed
	bool pressed = false; //is the button pressed now
};

struct Player {
    Player() = default;

	int8_t player_id = -1;
	Button left, right, up, down, mouse;
	float mouse_x;
	float mouse_y;
	float shoot_interval = 0;

	void update(float elapsed);
	void set_position(float new_x, float new_y);
	void place_clone();
	void shoot (); 

	void send_player_message(Connection *connection) const;
	//returns 'false' if no message or not a player message,
	//returns 'true' if read a player message,
	//throws on malformed player message
	bool recv_player_message(Connection *connection);
};



//state of one player in the game:

struct Game {
	std::list< Player > players; //(using list so they can have stable addresses)

	Player *spawn_player(); //add player the end of the players list (may also, e.g., play some spawn anim)
	void remove_player(Player *); //remove player from game (may also, e.g., play some despawn anim)

	Character *spawn_character(Player *new_player);

	std::mt19937 mt; //used for spawning players
	uint32_t player_cnt = 0; //used for naming players

	CommonData *common_data;

	GameState state = PlaceClones;
	bool single_player = false;

	Game();

	//state update function:
	void update(float elapsed);

	//constants:
	//the update rate on the server:
	inline static constexpr float Tick = 1.0f / 30.0f;

	float time_remaining = PLACE_CLONE_PHASE_DURATION;
	// TODO: optimize
	float time_elapsed = 0;	// Should be PLACE_CLONE_PHASE_DURATION - time_remaining. Fix later
	
	std::vector<MapObject> create_map();

	void update_place_clones(float elapsed);
	void update_find_clones(float elapsed);
	void update_kill_clones(float elapsed);

	void setup_place_clones();
	void setup_find_clones();
	void setup_kill_clones();

	//---- communication helpers ----

	//used by client:
	//set game state from data in connection buffer
	// (return true if data was read)
	bool recv_state_message(Connection *connection);

	//used by server:
	//send game state.
	//  Will move "connection_player" to the front of the front of the sent list.
	void send_state_message(Connection *connection, Player *connection_player = nullptr) const;

	// used by server
	void send_setup_message(Connection *connection_, Player *connection_player) const;
	// used by client
	bool recv_setup_message(Connection *connection_, Player *client_player);

	// calls different helper functions based on their connection enums
	bool client_recv_message(Connection *connection_, Player *client_player);
};
