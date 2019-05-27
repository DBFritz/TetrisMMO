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
//FIXME: Bug when moving all to the right side when the piece is starting to falling

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
    
    int choice;
    do {
        mvprintw(5, 1, "Please, choose an option");
        mvprintw(6, 3, "1. Play singleplayer mode");
        mvprintw(7, 3, "2. Join a game");
        mvprintw(8, 3, "3. Host a game");
        mvprintw(9, 3, "4. Change keys");
        mvprintw(10,3, "q. Quit");
        choice = getch();
        erase();
        switch (choice) {
            case '1':{
                tetris::singleplayer_t sp(name);
                sp.play();
                choice = 'q';
                break;
            }
            case '2':{
                ////join_game();
                mvprintw(12,3, "Insert IP of the host:"); // (also port if required) (?)
                echo();
                mvprintw(15,5, ">> ");
                char server[64]; //FIXME: Change value
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
                break;
            }
            case '3': try
                {
                    mvprintw(12,3, "Insert number of players"); // (also port if required) (?)
                    echo();
                    mvprintw(15,5, ">> ");
                    int players; //FIXME: Change value
                    scanw("%d", &players);
                    noecho();
                    
                    pid_t pid = fork();
                    if (pid > 0) {
                        tetris::client_t cl(name);
                        endwin();
                        cl.connect("127.0.0.1");
                        cl.play();
                    } else if (pid == 0) { // */
                        endwin();
                        std::cout << players << std::endl;
                        tetris::server_t srv(players);
                        srv.run();
                    
                    } else {
                        std::cerr << "ERROR Forking" << std::endl;
                    } // */
                    exit(0);
                } catch (char const * err) {
                    std::cerr << "Exception: " << err << std::endl;
                }
                break;
            case '4':{
                //change_keys();
                break;
            }
            case 'q':{
                endwin();
                exit(0);
            }      
        }
    } while (choice != 'q');
    
    endwin();

    return 0;
}
