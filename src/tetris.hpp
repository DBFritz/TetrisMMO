#ifndef _TETRIS_
#define _TETRIS_

#include "tetris_curses.hpp"

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
      public:
        client_t(std::string name = "Unnamed");
        ~client_t();

        void play();

        bool connect(std::string server = "127.0.0.1");

        // Overloading
        void move(int direction);
        //void rotate(int direction);
        //void hold();
        //void drop();
    };
}


#endif