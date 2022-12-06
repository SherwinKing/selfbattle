
#include "Connection.hpp"

#include "hex_dump.hpp"

#include "Game.hpp"

#include <chrono>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include <unordered_map>

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

#ifdef _WIN32
extern "C" { uint32_t GetACP(); }
#endif

// execute command in c++
// https://stackoverflow.com/questions/52164723/how-to-execute-a-command-and-get-return-code-stdout-and-stderr-of-command-in-c
std::string exec(const char* cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    auto pipe = popen(cmd, "r");
    
    if (!pipe) throw std::runtime_error("popen() failed!");
    
    while (!feof(pipe))
    {
        if (fgets(buffer.data(), 128, pipe) != nullptr)
            result += buffer.data();
    }
    
    auto rc = pclose(pipe);
    
    if (rc == EXIT_SUCCESS)
    {
        std::cout << "SUCCESS\n";
    }
    else
    {
        std::cout << "FAILED\n";
    }
    
    return result;
}

int main(int argc, char **argv) {
#ifdef _WIN32
	{ //when compiled on windows, check that code page is forced to utf-8 (makes file loading/saving work right):
		//see: https://docs.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page
		uint32_t code_page = GetACP();
		if (code_page == 65001) {
			std::cout << "Code page is properly set to UTF-8." << std::endl;
		} else {
			std::cout << "WARNING: code page is set to " << code_page << " instead of 65001 (UTF-8). Some file handling functions may fail." << std::endl;
		}
	}

	//when compiled on windows, unhandled exceptions don't have their message printed, which can make debugging simple issues difficult.
	try {
#endif

	//------------ argument parsing ------------

	// if (argc < 2) {
	// 	std::cerr << "Usage:\n\t./server [<host>] <port>" << std::endl;
	// 	return 1;
	// }

	bool should_broadcast_beacon = false;
	LANServerHelper lan_helper;
	std::string host;
	std::string port;
	if (argc >= 3) {
		host = argv[1];
		port = argv[2];
	} else if (argc == 2) {
		host = argv[1];
		port = "1234";
	} else {
		std::cout << "\nNo IP detected from argument\n";
		std::cout << "Attempting to collect IP through command line...\n";
		std::string ip_command = R"( ifconfig en0 | grep -E "inet ([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}) " | awk '{print $2}' )";
		host = exec(ip_command.c_str());
		// removing \n and space
		host.erase(remove(host.begin(), host.end(), '\n'), host.end());
   		host.erase(remove(host.begin(), host.end(), ' '), host.end());
		std::cout << "Host IP: " << host << "\n";
		port = "1234";
		// should_broadcast_beacon = true;
		// port = "15666";
		// lan_helper.broadcast_beacon();
		// host = lan_helper.get_server_ip();
	}

	//------------ initialization ------------

	Server server(host, port);

	//------------ main loop ------------

	//keep track of which connection is controlling which player:
	std::unordered_map< Connection *, Player * > connection_to_player;
	std::unordered_map< int8_t, Connection * > player_to_connection;
	//keep track of game state:
	Game game;

	while (true) {
		static auto next_tick = std::chrono::steady_clock::now() + std::chrono::duration< double >(Game::Tick);
		//process incoming data from clients until a tick has elapsed:
		while (true) {
			auto now = std::chrono::steady_clock::now();
			double remain = std::chrono::duration< double >(next_tick - now).count();
			if (remain < 0.0) {
				next_tick += std::chrono::duration< double >(Game::Tick);
				break;
			}

			// Use UDP to broadcast the server's existence for server discovery
			if (should_broadcast_beacon) {
				lan_helper.broadcast_beacon();
			}

			//helper used on client close (due to quit) and server close (due to error):
			auto remove_connection = [&](Connection *c) {
				// TODO: fix this
				auto f = connection_to_player.find(c);
				assert(f != connection_to_player.end());
				Player *player = connection_to_player[c];
				player_to_connection.erase(player->player_id);
				game.remove_player(f->second);
				connection_to_player.erase(f);
			};

			server.poll([&](Connection *c, Connection::Event evt){
				if (evt == Connection::OnOpen) {
					//client connected:

					//create some player info for them:
					Player *new_player = game.spawn_player();
					connection_to_player.emplace(c, new_player);
					player_to_connection.emplace(new_player->player_id, c);

					game.send_message(c, new_player, MESSAGE::SERVER_INIT);

				} else if (evt == Connection::OnClose) {
					//client disconnected:

					remove_connection(c);

				} else { assert(evt == Connection::OnRecv);
					//got data from client:
					//std::cout << "current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush(); //DEBUG
					//look up in players list:
					auto f = connection_to_player.find(c);
					assert(f != connection_to_player.end());
					Player &player = *f->second;

					//handle messages from client:
					try {
						bool handled_message;
						do {
							handled_message = false;
							if (game.recv_message(c, &player, true) != MESSAGE::MSG_NONE) handled_message = true;
							//TODO: extend for more message types as needed
						} while (handled_message);
					} catch (std::exception const &e) {
						std::cout << "Disconnecting client:" << e.what() << std::endl;
						c->close();
						remove_connection(c);
					}
				}
			}, remain);
		}

		// send messages in queue
		while (game.message_queue.size() != 0) {
			MessageInfo message = game.message_queue.front();
			// send it to the other player
			Connection *c = player_to_connection[!message.player_id];
			game.send_message(c, &game.players[message.player_id], message.tag);
			game.message_queue.pop_front();
		}

		// complete updates related to the messages (like input sync)
		while (game.message_queue.size() != 0) {
			MessageInfo message = game.message_queue.front();
			game.process_action(&game.players[message.player_id], message.tag);
			game.message_queue.pop_front();
		}

		//update current game state
		game.update(Game::Tick);

		//send updated game state to all clients
		// for (auto &[c, player] : connection_to_player) {
		// 	game.send_message(c, player);
		// }
	}


	return 0;

#ifdef _WIN32
	} catch (std::exception const &e) {
		std::cerr << "Unhandled exception:\n" << e.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << "Unhandled exception (unknown type)." << std::endl;
		throw;
	}
#endif
}
