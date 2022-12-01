
//--------- OS-specific socket-related headers ---------
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS 1 //so we can use strerror()
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#undef APIENTRY
#include <winsock2.h>
#include <ws2tcpip.h> //for getaddrinfo
#undef max
#undef min

#pragma comment(lib, "Ws2_32.lib") //link against the winsock2 library

#define MSG_DONTWAIT 0 //on windows, sockets are set to non-blocking with an ioctl
typedef int ssize_t;

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>     /* exit */

#define closesocket close

#endif

#include "Connection.hpp"

//------------------------------------------------------

#include <iostream>
#include <cmath>
#include <algorithm>
#include <cassert>
#include <cstring>

//NOTE: much of the sockets code herein is based on http-tweak's single-header http server
// see: https://github.com/ixchow/http-tweak

//Also, some help and examples for getaddrinfo from: https://beej.us/guide/bgnet/html/multi/syscalls.html


void Connection::close() {
	if (socket != InvalidSocket) {
		::closesocket(socket);
		socket = InvalidSocket;
	}
}

//---------------------------------
//Polling helper used by both server and client:
void poll_connections(
	char const *where,
	std::list< Connection > &connections,
	std::function< void(Connection *, Connection::Event event) > const &on_event,
	double timeout,
	Socket listen_socket = InvalidSocket) {

	fd_set read_fds, write_fds;
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);

	int max = 0;

	//add listen_socket to fd_set if needed:
	if (listen_socket != InvalidSocket) {
		max = std::max(max, int(listen_socket));
		FD_SET(listen_socket, &read_fds);
	}

	//add each connection's socket to read (and possibly write) sets:
	for (auto c : connections) {
		if (c.socket != InvalidSocket) {
			max = std::max(max, int(c.socket));
			FD_SET(c.socket, &read_fds);
			if (!c.send_buffer.empty()) {
				FD_SET(c.socket, &write_fds);
			}
		}
	}

	{ //wait (until timeout) for sockets' data to become available:
		struct timeval tv;
		tv.tv_sec = std::lround(std::floor(timeout));
		tv.tv_usec = std::lround((timeout - std::floor(timeout)) * 1e6);
		//NOTE: on windows nfds is ignored -- https://msdn.microsoft.com/en-us/library/windows/desktop/ms740141(v=vs.85).aspx
		int ret = select(max + 1, &read_fds, &write_fds, NULL, &tv);

		if (ret < 0) {
			std::cerr << "[" << where << "] Select returned an error; will attempt to read/write anyway." << std::endl;
		} else if (ret == 0) {
			//nothing to read or write.
			return;
		}
	}

	//add new connections as needed:
	if (listen_socket != InvalidSocket && FD_ISSET(listen_socket, &read_fds)) {
		Socket got = accept(listen_socket, NULL, NULL);
		if (got == InvalidSocket) {
			//oh well.
		} else {
			#ifdef _WIN32
			unsigned long one = 1;
			if (0 == ioctlsocket(got, FIONBIO, &one)) {
			#else
			{
			#endif
				connections.emplace_back();
				connections.back().socket = got;
				std::cerr << "[" << where << "] client connected on " << connections.back().socket << "." << std::endl; //INFO
				if (on_event) on_event(&connections.back(), Connection::OnOpen);
			}
		}
	}

	const uint32_t BufferSize = 20000;
	static thread_local char *buffer = new char[BufferSize];

	//process requests:
	for (auto &c : connections) {
		//only read from valid sockets marked readable:
		if (c.socket == InvalidSocket || !FD_ISSET(c.socket, &read_fds)) continue;

		while (true) { //read until more data left to read
			ssize_t ret = recv(c.socket, buffer, BufferSize, MSG_DONTWAIT);
			if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
				//~no problem~ but no data
				break;
			} else if (ret <= 0 || ret > (ssize_t)BufferSize) {
				//~problem~ so remove connection
				if (ret == 0) {
					std::cerr << "[" << where << "] port closed, disconnecting." << std::endl;
				} else if (ret < 0) {
					std::cerr << "[" << where << "] recv() returned error " << errno << "(" << strerror(errno) << "), disconnecting." << std::endl;
				} else {
					std::cerr << "[" << where << "] recv() returned strange number of bytes, disconnecting." << std::endl;
				}
				c.close();
				if (on_event) on_event(&c, Connection::OnClose);
				break;
			} else { //ret > 0
				c.recv_buffer.insert(c.recv_buffer.end(), buffer, buffer + ret);
				if (on_event) on_event(&c, Connection::OnRecv);
				if (ret < BufferSize) break; //ran out of data before buffer: no more data left to read
			}
		}
	}

	//process responses:
	for (auto &c : connections) {
		//don't bother with connections unless they are valid, have something to send, and are marked writable:
		if (c.socket == InvalidSocket || c.send_buffer.empty() || !FD_ISSET(c.socket, &write_fds)) continue;
		
		#ifdef _WIN32
		ssize_t ret = send(c.socket, reinterpret_cast< char const * >(c.send_buffer.data()), int(c.send_buffer.size()), MSG_DONTWAIT);
		#else
		ssize_t ret = send(c.socket, reinterpret_cast< char const * >(c.send_buffer.data()), c.send_buffer.size(), MSG_DONTWAIT);
		#endif 
		if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			//~no problem~, but don't keep trying
			break;
		} else if (ret <= 0 || ret > (ssize_t)c.send_buffer.size()) {
			if (ret < 0) {
				std::cerr << "[" << where << "] send() returned error " << errno << ", disconnecting." << std::endl;
			} else { assert(ret == 0 || ret > (ssize_t)c.send_buffer.size());
				std::cerr << "[" << where << "] send() returned strange number of bytes [" << ret << " of " << c.send_buffer.size() << "], disconnecting." << std::endl;
			}
			c.close();
			if (on_event) on_event(&c, Connection::OnClose);
		} else { //ret seems reasonable
			c.send_buffer.erase(c.send_buffer.begin(), c.send_buffer.begin() + ret);
		}
	}

		
}

//-------------


LANServerHelper::LANServerHelper() {
	//------------ setting up the UDP broadcast server ------------
	broadcast_udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (broadcast_udp_sock == InvalidSocket) {
		std::cerr << "Failed to create UDP broadcast socket." << std::endl;
		std::exit(1);
	}
	// Set socket options
	int broadcast = 1;
	if (setsockopt(broadcast_udp_sock, SOL_SOCKET, SO_BROADCAST, reinterpret_cast< const char * >(&broadcast), sizeof(broadcast)) < 0) {
		std::cerr << "Failed to set UDP broadcast socket options." << std::endl;
		std::exit(1);
	}
}

void LANServerHelper::broadcast_beacon() {
	// UDP sendto broadcast

	// Set destination address
	struct sockaddr_in addr = {0};
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(lan_helper_port);
	addr.sin_addr.s_addr = INADDR_BROADCAST;

	// Send data
	std::string message = "Hello, this is a server!";
	if (sendto(broadcast_udp_sock, message.c_str(), static_cast<int>(message.size()), 0, reinterpret_cast< struct sockaddr * >(&addr), sizeof(addr)) < 0) {
		std::cerr << "Failed to send UDP broadcast." << std::endl;
		std::exit(1);
	}
}

LANServerHelper::~LANServerHelper() {
	// Close socket
	closesocket(broadcast_udp_sock);
}

std::string LANServerHelper::get_server_ip() {
	// Get the address of self
	struct sockaddr_in self_addr = {0};
	socklen_t self_addr_len = sizeof(self_addr);
	if (getsockname(broadcast_udp_sock, reinterpret_cast< struct sockaddr * >(&self_addr), &self_addr_len) < 0) {
		std::cerr << "Failed to get self address." << std::endl;
		std::exit(1);
	}
	// return InetNtop(self_addr.sin_addr);
	char str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(self_addr.sin_addr), str, INET_ADDRSTRLEN);
	return std::string(str);
}

LANClientHelper::LANClientHelper() {

}

std::string LANClientHelper::discover_server() {
	std::cout << "Discovering server..." << std::endl;
	// The following codes are written with help of Copilot
	// Receive beacon
	// Create a socket
	Socket sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == InvalidSocket) {
		std::cerr << "Failed to create UDP socket." << std::endl;
		closesocket(sock);
		std::exit(1);
	}
	
	// Set socket options
	int broadcast = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, reinterpret_cast< const char * >(&broadcast), sizeof(broadcast)) < 0) {
		std::cerr << "Failed to set UDP socket options." << std::endl;
		closesocket(sock);
		std::exit(1);
	}
	// Set socket to reuse port
	int reuse = 1;
	// Set socket to reuse address
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast< const char * >(&reuse), sizeof(reuse)) < 0) {
		std::cerr << "Failed to set UDP socket options." << std::endl;
		closesocket(sock);
		std::exit(1);
	}

	// Set destination address
	struct sockaddr_in addr = {0};
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(lan_helper_port);
	addr.sin_addr.s_addr = INADDR_ANY;

	// Bind socket
	auto bind_result = bind(sock, reinterpret_cast< struct sockaddr * >(&addr), sizeof(addr));
	if (bind_result < 0) {
		std::cerr << "Failed to bind UDP socket, port: " << addr.sin_port <<". Bind return: " << bind_result << ", errno " << errno << std::endl;
		closesocket(sock);
		std::exit(1);
	}

	// Receive data and get sender address
	char buffer[1024];
	struct sockaddr_in sender = {0};
	socklen_t sender_len = sizeof(sender);
	int bytes = recvfrom(sock, buffer, 1024, 0,  reinterpret_cast< struct sockaddr * >(&sender), &sender_len);
	if (bytes < 0) {
		std::cerr << "Failed to receive UDP data." << std::endl;
		closesocket(sock);
		std::exit(1);
	}
	std::cout << "Received: " << buffer << std::endl;

	// Close socket and unbind port
	closesocket(sock);
	
	char str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(sender.sin_addr), str, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &(sender.sin_addr), str, INET_ADDRSTRLEN);
	return std::string(str);
}



//---------------------------------


Server::Server(std::string const &host, std::string const &port) {

	#ifdef _WIN32
	{ //init winsock:
		WSADATA info;
		if (WSAStartup((2 << 8) | 2, &info) != 0) {
			throw std::runtime_error("WSAStartup failed.");
		}
	}
	#endif

	{ //use getaddrinfo to look up how to bind to port:
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		struct addrinfo *res = nullptr;

		int addrinfo_ret;
		
		if (host.size() > 0) {
			addrinfo_ret = getaddrinfo(host.c_str(), port.c_str(), &hints, &res);
		} else {
			addrinfo_ret = getaddrinfo(NULL, port.c_str(), &hints, &res);
		}
		if (addrinfo_ret != 0) {
			throw std::runtime_error("getaddrinfo error: " + std::string(gai_strerror(addrinfo_ret)));
		}

		std::cout << "[Server::Server] binding to " << port << ":" << std::endl;
		//based on example code in the 'man getaddrinfo' man page on OSX:
		for (struct addrinfo *info = res; info != nullptr; info = info->ai_next) {
			{ //DEBUG: dump info about this address:
				std::cout << "\ttrying ";
				char ip[INET6_ADDRSTRLEN];
				if (info->ai_family == AF_INET) {
					struct sockaddr_in *s = reinterpret_cast< struct sockaddr_in * >(info->ai_addr);
					inet_ntop(res->ai_family, &s->sin_addr, ip, sizeof(ip));
					std::cout << ip << ":" << ntohs(s->sin_port);
				} else if (info->ai_family == AF_INET6) {
					struct sockaddr_in6 *s = reinterpret_cast< struct sockaddr_in6 * >(info->ai_addr);
					inet_ntop(res->ai_family, &s->sin6_addr, ip, sizeof(ip));
					std::cout << ip << ":" << ntohs(s->sin6_port);
				} else {
					std::cout << "[unknown ai_family]";
				}
				std::cout << "... "; std::cout.flush();
			}

			Socket s = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
			if (s == InvalidSocket) {
				std::cout << "(failed to create socket: " << strerror(errno) << ")" << std::endl;
				continue;
			}

			{ //make it okay to reuse port:
				#ifdef _WIN32
				BOOL one = TRUE;
				int ret = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast< const char * >(&one), sizeof(one));
				#else
				int one = 1;
				int ret = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
				#endif
				if (ret != 0) {
					std::cout << "[note: couldn't set SO_REUSEADDR] " << std::endl;
				}
			}

			int ret = bind(s, info->ai_addr, int(info->ai_addrlen));
			if (ret < 0) {
				std::cout << "(failed to bind: " << strerror(errno) << ")" << std::endl;
				continue;
			}
			std::cout << "success!" << std::endl;

			listen_socket = s;
			break;
		}

		freeaddrinfo(res);
	}

	if (listen_socket == InvalidSocket) {
		throw std::runtime_error("Failed to bind to port " + port);
	}

	{ //listen on socket
		int ret = ::listen(listen_socket, 5);
		if (ret < 0) {
			closesocket(listen_socket);
			throw std::system_error(errno, std::system_category(), "failed to listen on socket");
		}
	}
}

void Server::poll(std::function< void(Connection *, Connection::Event event) > const &on_event, double timeout) {
	poll_connections("Server::poll", connections, on_event, timeout, listen_socket);

	//reap closed clients:
	for (auto connection = connections.begin(); connection != connections.end(); /*later*/) {
		auto old = connection;
		++connection;
		if (old->socket == InvalidSocket) {
			connections.erase(old);
		}
	}
}

// void broadcast_beacon() {
// 	// UDP sendto broadcast

// 	// Send data
// 	std::string message = "Hello, World!";
// 	std::cout << "Sending " << message << std::endl;
// 	if (sendto(broadcast_udp_sock, message.c_str(), message.size(), 0, reinterpret_cast< struct sockaddr * >(&addr), sizeof(addr)) < 0) {
// 		throw std::runtime_error("Failed to send data");
// 	}

// 	// Get the address of self
// 	struct sockaddr_in self_addr;
// 	socklen_t self_addr_len = sizeof(self_addr);
// 	if (getsockname(sock, reinterpret_cast< struct sockaddr * >(&self_addr), &self_addr_len) < 0) {
// 		throw std::runtime_error("Failed to get self address");
// 	}
	

// 	// Reset 

// 	// Close socket
// 	closesocket(broadcast_udp_sock);
// }


Client::Client(std::string const &host, std::string const &port) : connections(1), connection(connections.front()) {
	#ifdef _WIN32
	{ //init winsock:
		WSADATA info;
		if (WSAStartup((2 << 8) | 2, &info) != 0) {
			throw std::runtime_error("WSAStartup failed.");
		}
	}
	#endif


	{ //use getaddrinfo to look up how to bind to host/port:
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		struct addrinfo *res = nullptr;
		int addrinfo_ret = getaddrinfo(host.c_str(), port.c_str(), &hints, &res);
		if (addrinfo_ret != 0) {
			throw std::runtime_error("getaddrinfo error: " + std::string(gai_strerror(addrinfo_ret)));
		}

		std::cout << "[Client::Client] connecting to " << host << ":" << port << ":" << std::endl;
		//based on example code in the 'man getaddrinfo' man page on OSX:
		for (struct addrinfo *info = res; info != nullptr; info = info->ai_next) {
			{ //DEBUG: dump info about this address:
				std::cout << "\ttrying ";
				char ip[INET6_ADDRSTRLEN];
				if (info->ai_family == AF_INET) {
					struct sockaddr_in *s = reinterpret_cast< struct sockaddr_in * >(info->ai_addr);
					inet_ntop(res->ai_family, &s->sin_addr, ip, sizeof(ip));
					std::cout << ip << ":" << ntohs(s->sin_port);
				} else if (info->ai_family == AF_INET6) {
					struct sockaddr_in6 *s = reinterpret_cast< struct sockaddr_in6 * >(info->ai_addr);
					inet_ntop(res->ai_family, &s->sin6_addr, ip, sizeof(ip));
					std::cout << ip << ":" << ntohs(s->sin6_port);
				} else {
					std::cout << "[unknown ai_family]";
				}
				std::cout << "... "; std::cout.flush();
			}

			Socket s = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
			if (s == InvalidSocket) {
				std::cout << "(failed to create socket: " << strerror(errno) << ")" << std::endl;
				continue;
			}
			int ret = connect(s, info->ai_addr, int(info->ai_addrlen));
			if (ret < 0) {
				std::cout << "(failed to connect: " << strerror(errno) << ")" << std::endl;
				continue;
			}
			std::cout << "success!" << std::endl;

			connection.socket = s;
			break;
		}

		freeaddrinfo(res);

		if (!connection) {
			throw std::runtime_error("Failed to connect to any of the addresses tried for server.");
		}
	}
}


void Client::poll(std::function< void(Connection *, Connection::Event event) > const &on_event, double timeout) {
	poll_connections("Client::poll", connections, on_event, timeout, InvalidSocket);
}

