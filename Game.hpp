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
	C2S_Controls = 1, //Greg!
	S2C_State = 's',
	//...
};

enum GameState  {
	GamePaused, // A count down or something
	GameStarting,
	PlaceClones,
	FindClones,
	KillClones,
	GameOver
};

struct Player {
    Player() = default;

	void place_clone(float world_x, float world_y);
	void shoot (float world_x, float world_y); 
	bool recv_message(Connection *connection);
	void send_message(Connection *connection);
	void move_player(float dx, float dy);

	CommonData common_data;

	// Place clones phase info
	float place_time_elapsed = 0.f;

	// Find clones phase info
	float find_time_elapsed = 0.f;

	// Kill clones phase info
	float kill_time_elapsed = 0.f;

	
	void update(float elapsed);

	void set_position(float new_x, float new_y);
	void draw(glm::uvec2 const &drawable_size);

	// Game information (synchronize with server?)
	GameState state;

	// Character information
    // in radians from positive x (like a unit circle)
    // used to know where the player mouse is pointing right now
	Character c;
    
	bool is_client;

	// Pressed inputs
	uint32_t lefts = 0;
	uint32_t rights	= 0;
	uint32_t ups = 0;
	uint32_t downs = 0;


	private:

		void update_place_clones(float elapsed);
		void update_find_clones(float elapsed);
		void update_kill_clones(float elapsed);
};



//state of one player in the game:

struct Game {
	std::list< Player > players; //(using list so they can have stable addresses)
	Player *spawn_player(); //add player the end of the players list (may also, e.g., play some spawn anim)
	void remove_player(Player *); //remove player from game (may also, e.g., play some despawn anim)

	std::mt19937 mt; //used for spawning players
	uint32_t next_player_number = 1; //used for naming players

	Game();

	//state update function:
	void update(float elapsed);

	//constants:
	//the update rate on the server:
	inline static constexpr float Tick = 1.0f / 30.0f;

	//arena size:
	inline static constexpr glm::vec2 ArenaMin = glm::vec2(-0.75f, -1.0f);
	inline static constexpr glm::vec2 ArenaMax = glm::vec2( 0.75f,  1.0f);

	//player constants:
	inline static constexpr float PlayerRadius = 0.06f;
	inline static constexpr float PlayerSpeed = 2.0f;
	inline static constexpr float PlayerAccelHalflife = 0.25f;
	
	std::vector<MapObject> create_map();
	CommonData common_data;

	//---- communication helpers ----

	//used by client:
	//set game state from data in connection buffer
	// (return true if data was read)
	bool recv_state_message(Connection *connection);

	//used by server:
	//send game state.
	//  Will move "connection_player" to the front of the front of the sent list.
	void send_state_message(Connection *connection, Player *connection_player = nullptr) const;
};
