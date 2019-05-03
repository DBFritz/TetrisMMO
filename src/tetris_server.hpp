#ifndef _TETRIS_SERVER_
#define _TETRIS_SERVER_
#include <unordered_map>
#include "tetris_base.hpp"

namespace tetris{
    class server_t{
      private:
        int max_players;
        game_t *players;
        int *attacked;
        int *clients_socket;

        void sendboard(int player);
        struct command_t {
            bool is_with_argument;
            union fun_t {
                void (game_t::*with_argument)(int);
                void (game_t::*wo_argument)();
            } fun;
        };
        std::unordered_map<std::string, command_t> commands;

      public:
        server_t(int N=2);
        ~server_t();

        bool is_playing(int i) {return clients_socket[i]!=0;}

        void set_command(std::string command, void (game_t::*fun)(int));
        void set_command(std::string command, void (game_t::*fun)());

        //void remove_command(void (game_t::*fun)(int, int), int arg);
        //void remove_command(void (game_t::*fun)(int));

        void run(int port=8558);
        void handle_message(int player, std::string buffer);

        void play(int player);
    };
}

std::ostream & operator << (std::ostream &out, const tetris::block_t &b);

#endif