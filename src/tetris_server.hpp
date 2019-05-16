#ifndef _TETRIS_SERVER_
#define _TETRIS_SERVER_ 
#include <unordered_map>
#include <sys/socket.h>
#include <vector>
#include "tetris_base.hpp"


// #include <new>          //placement
// char buf[1024];
// message_t *pm = new (&buf[0]) message_t(MOVE);

namespace tetris{
    
    /*
    uint8_t *payload = new uint8_t[max_packet_size];
    message_t *packet = new (payload) message_t()

    uint8_t buf[max_packet_size];
    message_t *packet = new (buf) message_t();*/

    class message_t {
    public:
        enum class header_t: uint8_t {
            // Client -> Server
            CLIENTHI = 0x00, MOVE, ROTATE, HOLD, DROP, ACCELERATE, CHANGE_ATTACKED, DISCONNECT,
            // Server -> Client
            SERVERHI = 0x80, PLAY, BOARD, ATTACKED, FALLING, NEXT, STORED, POINTS, WIN, LOSES
        };
        enum class content_type_t: uint8_t {
            NONE, BOARD, BLOCK, STRING, NUMBER
        };
        struct packet_t {
            header_t header;
            content_type_t type_of_content;
            uint16_t payload_size;
            uint8_t payload[0];      // doesn't contribute in sizeof();
        };

        message_t(packet_t packet);
        message_t(header_t header, content_type_t content, void * payload = nullptr, size_t payload_size = 0);

        message_t(header_t header);
        message_t(header_t header, game_t &board);
        message_t(header_t header, block_t block);
        message_t(header_t header, std::string str);
        message_t(header_t header, int32_t value);
        ~message_t();


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
        game_t *players;
        int *attacked;
        std::vector<int> *attackers;
        std::string *names;
        int *clients_socket;

        void sendboard(int player);
        struct command_t {
            bool is_with_argument;
            union fun_t {
                void (game_t::*with_argument)(int);
                void (game_t::*wo_argument)();
            } fun;
            bool has_response;

        };
        std::unordered_map<message_t::header_t, command_t> commands;
        //message_t::header_t message;

      public:
        server_t(int N=2, bool verbose = false);
        ~server_t();

        bool is_playing(int i) {return clients_socket[i]!=0;}

        void set_command(message_t::header_t command, void (game_t::*fun)(int));
        void set_command(message_t::header_t command, void (game_t::*fun)());

        //void remove_command(void (game_t::*fun)(int, int), int arg);
        //void remove_command(void (game_t::*fun)(int));

        void run(int port=8558);
        void handle_message(int player, message_t *msg);

        void play(int player);
    };
}

std::ostream & operator << (std::ostream &out, const tetris::block_t &b);
std::ostream & operator << (std::ostream &out, tetris::game_t &g);

#endif