#ifndef _TETRIS_SERVER_
#define _TETRIS_SERVER_

#include "tetris_base.hpp"

namespace tetris{
    class server_t{
      private:
        int max_players;
        game_t *players;
        int *clients_socket;

      public:
        server_t(int N=2);
        ~server_t();

        bool is_playing(int i) {return clients_socket[i]!=0;}

        void run(int port=8558);
    };
}

#endif