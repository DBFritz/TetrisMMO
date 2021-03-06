#ifndef _TETRIS_
#define _TETRIS_
//#include "tetris_server.hpp"    //MESSAGES
#include "tetris_curses.hpp"
#include <ncurses.h>
#include <sstream>
#include <string>

namespace tetris{
    class singleplayer_t: tetris_t{
        tetris::keys_t<singleplayer_t> keys;
      public:
        singleplayer_t(std::string name = "Unnamed", int rows = 20, int cols = 10);
        void play();
    };

    class client_t: public tetris_t{
      protected:
        tetris::keys_t<client_t> keys;
        int sockfd;
        bool dif_endian;
        std::stringstream stream;
        std::stringstream streamrcv;
        std::string attacked;

        std::thread socket_handler;

        block_type_t *enemy_board;
        WINDOW * trash_w;

        WINDOW * trash_enemy_w;
        WINDOW * enemy_w;

        void send(std::string command, int value);
        void send(std::string command);
        void handle_socket();

      public:
        client_t(std::string name = "Unnamed");
        ~client_t();

        void play();

        bool connect(std::string server = "127.0.0.1");
        void draw_enemy_board(std::string name = "");
        void draw_trash_stack(WINDOW * w, int trash);

        // Overloading
        void move(int direction);
        void rotate(int direction);
        void hold();
        void drop();
        void accelerate(int newremaining = 1);
        void change_attacked();
    };
}

std::istream & operator >> (std::istream &in, tetris::block_t &b);

#endif