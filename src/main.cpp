#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
//#include <ncursesw/curses.h>
#include <curses.h>
#include <string.h>
#include <chrono>
#include <thread>
#include <locale.h>
#include <algorithm>
#include "tetris.hpp"
#include "tetris_server.hpp"
#include <unistd.h>

int main(int argc, char **argv)
{
    std::string name = "Unnamed";
    if (argc>1) {
        name = std::string(argv[1]);
    } else {
        std::cout << "Ingrese su nombre: " ;
        std::cin >> name;
    }

    initscr();
    curs_set(0);
    noecho();
    keypad(stdscr, TRUE);

    mvprintw(1, 1, "Welcome to TetrisMMO!");
    while (true){
        mvprintw(5, 1, "Please, choose an option");
        mvprintw(6, 3, "1. Play singleplayer mode");
        mvprintw(7, 3, "2. Join a game");
        mvprintw(8, 3, "3. Host a game");
        mvprintw(9, 3, "4. Change keys");
        mvprintw(10,3, "q. Quit");
        switch (getch()) {
            case '1': {
                tetris::singleplayer_t sp(name);
                sp.play();
                return 0;
            }
            case '2': {
                mvprintw(12,3, "Insert IP of the host:");
                echo();
                mvprintw(15,5, ">> ");
                char server[64]; //FIXME: Change value
                fflush(stdin);
                getstr(server);
                noecho();
                doupdate();
                tetris::client_t cl(name);
                try {
                    cl.connect(server);
                    cl.play();
                } catch (char const* err){
                    std::cerr << "Exception: " << err << std::endl;
                }
                return 0;
            }
            case '3': {
                mvprintw(12,3, "Insert number of players");
                echo();
                mvprintw(15,5, ">> ");
                int players;
                scanw("%d", &players);
                noecho();
                    
                pid_t pid = fork();
                int ready[2];
                pipe(ready);
                if (pid > 0) {
                    tetris::client_t cl(name);
                    endwin();
                    close(ready[1]);
                    cl.connect("127.0.0.1");
                    cl.play();
                } else if (pid == 0) {
                    endwin();
                    std::cout << players << std::endl;
                    tetris::server_t srv(players, (argc>2 ? true : false));
                    srv.start();
                    close(ready[0]);
                    srv.run();
                } else {
                    std::cerr << "ERROR Forking" << std::endl;
                }
                return 0;
            }
            case '4':{
                //change_keys();
                break;
            }
            case 'q':{
                endwin();
                exit(0);
            }
        }
        erase();
    }
    return 0;
}
