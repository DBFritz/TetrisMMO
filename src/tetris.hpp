#ifndef _TETRIS_
#define _TETRIS_

#include "tetris_curses.hpp"
#include <sstream>
#include <string>

namespace tetris{
    class singleplayer_t: tetris_t{
        tetris::keys_t<tetris::singleplayer_t> keys;
      public:
        singleplayer_t(std::string name = "Unnamed", int rows = 20, int cols = 10);
        void play();
    };
    //TODO: Complete!
    class client_t: tetris_t{
      private:
        tetris::keys_t<tetris::client_t> keys;
        int sockfd;
        std::stringstream stream;
        std::stringstream streamrcv;
        std::string attacked;

        std::thread socket_handler;

        void send(std::string command, int value);
        void send(std::string command);
        void handle_socket();

      public:
        client_t(std::string name = "Unnamed");
        ~client_t();

        void play();

        bool connect(std::string server = "127.0.0.1");

        // Overloading
        void move(int direction){ send("MOVE", direction); }
        void rotate(int direction){ send("ROTATE", direction); }
        void hold() { send("HOLD"); }
        void drop() { send("DROP"); }
        void change_attacked(){ send("ATTACK"); }
    };
}

std::istream & operator >> (std::istream &in, tetris::block_t &b);

#endif