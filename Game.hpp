#pragma once

#include "ImageRenderer.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"
#include "Entity.hpp"

#include <glm/glm.hpp>

#include <string>
#include <list>
#include <random>

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

#include <string>
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


struct Map {
	std::vector<std::shared_ptr<MapObject>> map_objects;
};

struct Player {
    Player() = default;

	// Both the client and the server have a "Player" to send and receive stuff
	// But we really only need to do all the logic stuff and load textures
	// for the Player objects that clients have
	void init() {
		/// @brief  TODO: move this somewhere else
		// I originally had this at the top of the file but this was giving issues with 
		// the make and I didn't have the time/couldn't be bothered to spend hours trying
		// to figure out why it was doing this, so i just moved it here for now.
		constexpr uint32_t NUM_SPRITES = 4;
		std::array<std::pair<const char *, const char *>, NUM_SPRITES> sprite_paths = {
			std::pair("player0", "sprites/test.png"),
			std::pair("clone", "sprites/clone.png"),
			std::pair("wall", "sprites/wall.png"),
			std::pair("bullet", "sprites/bullet.png")	
		};
		
		for (size_t i = 0; i < NUM_SPRITES; ++i) {
            const auto& p = sprite_paths[i];
            ImageData s;
            load_png(data_path(std::string(p.second)), &s.size, &s.pixels, LowerLeftOrigin); 
            sprites.emplace(p.first, s);
	    }
		state = PlaceClones;
		c.init(PLAYER1_STARTING_X, PLAYER2_STARTING_Y);
		ImageData& player_sprite = sprites["player0"];
		c.set_box(player_sprite.size.x, player_sprite.size.y);

		map = create_map();
	}

	void place_clone(float screen_x, float screen_y, glm::uvec2 const &window_size);
	void shoot (float screen_x, float screen_y, glm::uvec2 const &window_size); 
	bool recv_message(Connection *connection);
	void send_message(Connection *connection);
	void move_player(float dx, float dy);


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

	std::shared_ptr<Map> map;

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

	ImageRenderer renderer;
    // Map map;
    ///TODO:
    // 2nd element is whatever we need for drawing sprites @SHERWIN
    // load_png gave us a std::vector< glm::u8vec4 > so using that for now
    std::unordered_map<std::string, ImageData> sprites;
	///TODO: should be updated with server because both players need to know 
	// where all bullets are
	std::vector<std::shared_ptr<Bullet>> bullets;	
	std::vector<std::shared_ptr<Clone>> clones;	
	std::vector<std::shared_ptr<Clone>> enemy_clones;	
	



	private:
		std::shared_ptr<Map> create_map();
		const ImageData& get_player_sprite();
		void world_to_opengl(float world_x, float world_y, glm::uvec2 const &screen_size, float& screen_x, float& screen_y);
		void screen_to_world(float screen_x, float screen_y, glm::uvec2 const &screen_size, float& world_x, float& world_y);

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
