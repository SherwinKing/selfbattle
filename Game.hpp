#pragma once

#include "ImageRenderer.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"

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

typedef unsigned int	uint;
#define SPRITE_DATA std::vector< glm::u8vec4 >

struct Connection;

//Game state, separate from rendering.

//Currently set up for a "client sends controls" / "server sends whole state" situation.
constexpr float CLONE_STARTING_HEALTH = 50.f;
constexpr float PLAYER_STARTING_HEALTH = 100.f;
constexpr float PLAYER1_STARTING_X = 0.f;
constexpr float PLAYER1_STARTING_Y = 0.f;
constexpr float PLAYER2_STARTING_X = 0.f;
constexpr float PLAYER2_STARTING_Y = 0.f;
constexpr float PLAYER_SPEED = 10.f;
constexpr float BULLET_SPEED = 80.f;
constexpr float BULLET_DAMAGE = 10.f;
constexpr float BULLET_LIFETIME = 10.f;
// Radius/width. Currently images are 100x100 so enough far away so it won't hit
// player when you click
constexpr float PLAYER_SIZE = 71.f;


constexpr float PLACE_CLONE_PHASE_DURATION = 30.f;
constexpr float FIND_CLONE_PHASE_DURATION = 15.f;
constexpr float KILL_CLONE_PHASE_DURATION = 30.f;


constexpr uint32_t NUM_CLONES = 2;

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

struct BoundingBox {
    BoundingBox() = default;
    BoundingBox(float low_x, float low_y, float high_x, float high_y) {
        lo_x = low_x;
        lo_y = low_y;
        hi_x = high_x;
        hi_y = high_y;
    } 
    float lo_x;
    float lo_y;
    float hi_x;
    float hi_y;
	void update_box(float dx, float dy);
};


struct Entity {
	float hp;
	BoundingBox box;
	float x;
	float y;
	ImageData *sprite;
	void set_box(uint w, uint h);
	void set_box(float w, float h);
	void move(float dx, float dy);
	void get_lower_left(float& lower_left_x, float& lower_left_y);
	bool collide(Entity& other);
};

struct MapObject : Entity {
	MapObject() = default;	
	MapObject(float start_x, float start_y, ImageData *obj_sprite) {
		x = start_x;
		y = start_y;
		sprite = obj_sprite;
		set_box(sprite->size.x, sprite->size.y);
	}
};

struct Map {
	std::vector<std::shared_ptr<MapObject>> map_objects;
};

struct Character : Entity {
	void init(float start_x, float start_y) {
		x = start_x; 
		y = start_y;
	}

	bool take_damage(float damage);

	float rot; 
    float hp = PLAYER_STARTING_HEALTH; 
	
};

struct Clone : Entity {
	Clone (float start_x, float start_y, ImageData *clone_sprite) {
		x = start_x;
		y = start_y;
		sprite = clone_sprite;
		set_box(sprite->size.x, sprite->size.y);
	}

	bool take_damage(float damage);

	float hp = CLONE_STARTING_HEALTH;	
};

struct Bullet : Entity {
	Bullet (float start_x, float start_y, ImageData *bullet_sprite, glm::vec2& bullet_velo) {
		velo = bullet_velo;
		x = start_x;
		y = start_y;
		sprite = bullet_sprite;
		set_box(sprite->size.x, sprite->size.y);
	}	

	void move_bullet(float elapsed);
	float lifetime = 0.f;
	glm::vec2 velo;
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
