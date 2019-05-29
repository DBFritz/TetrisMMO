#ifndef _TETRIS_SERVER_
#define _TETRIS_SERVER_ 
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <string>
#include <algorithm>
#include <sstream>
#include <arpa/inet.h>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <utility>
#include <mutex>
#include "tetris_base.hpp"
#include "tetris_server.hpp"

namespace tetris{

    class message_t {
    public:
        enum class header_t: uint8_t {
            // Client -> Server
            CLIENTHI = 0x00, MOVE, ROTATE, HOLD, DROP, ACCELERATE, CHANGE_ATTACKED, DISCONNECT, SMS = 0x79,
            // Server -> Client
            SERVERHI = 0x80, PLAY, BOARD, ATTACKED, ATT_NAME, FALLING, NEXT, STORED, POINTS, TRASH_STACK, TRASH_ENEMY_STACK, WIN, LOSES
        };
        enum class content_type_t: uint8_t {
            NONE, BOARD, BLOCK, STRING, NUMBER
        };
        struct packet_t {
            header_t header;
            content_type_t type_of_content;
            uint16_t payload_size;
            uint8_t payload[0];
        };

        message_t(packet_t packet);
        message_t(header_t header, content_type_t content, const void * payload = nullptr, size_t payload_size = 0);

        message_t(header_t header);
        message_t(header_t header, game_t &board);
        message_t(header_t header, block_t block);
        message_t(header_t header, std::string str);
        message_t(header_t header, int32_t value);
        ~message_t();

        message_t &update(const void * payload);
        message_t &update(const void * payload, uint16_t size);

        bool is_server();
        void send(int socket);
        static message_t recv(int socket);

        const header_t getHeader() { return packet->header; }
        const content_type_t getContentType() { return packet->type_of_content; }
        const void * getPayload() { return packet->payload; }
        uint16_t getPayloadSize() { return packet->payload_size; }
        uint16_t size() { return sizeof(packet_t)+packet->payload_size; }

    private:
        packet_t * packet;
    };

    class server_t{
      private:
        bool verbose;

        int max_players;
        game_t *game;
        int current_players;
        std::mutex *trash_mtx;
        int *trash_stacks;

        int *attacked;
        std::vector<int> *attackers;
        std::string *names;

        int master_socket;
        struct sockaddr_in address;
        int *clients_socket;
        bool running = false;

        struct command_t {
            bool is_with_argument;
            union fun_t {
                void (game_t::*with_argument)(int);
                void (game_t::*wo_argument)();
            } fun;
        };
        std::unordered_map<message_t::header_t, command_t> commands;
        std::unordered_map<message_t::header_t, std::pair<const void *, message_t> > responses;

      public:
        static const int trash_stack_size{14};
        
        server_t(int N=2, bool verbose = false);
        ~server_t();

        bool is_playing(int i) {return clients_socket[i]!=0;}

        void set_command(message_t::header_t command, void (game_t::*fun)(int));
        void set_command(message_t::header_t command, void (game_t::*fun)());
        void set_response(message_t::header_t command,
            message_t::header_t response, message_t::content_type_t content, const void * payload, uint16_t payload_size);

        //void remove_command(void (game_t::*fun)(int, int), int arg);
        //void remove_command(void (game_t::*fun)(int));

        void start(int port=8558);
        void run();
        void handle_message(int player, message_t *msg);

        void play(int player);
        void disconnect(int player);
        void disconnect(int player, message_t msg);
    };
}

std::ostream & operator << (std::ostream &out, const tetris::block_t &b);
std::ostream & operator << (std::ostream &out, tetris::game_t &g);

std::ostream & operator << (std::ostream &out, tetris::message_t &g);

#endif