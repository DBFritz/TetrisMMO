#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <ncursesw/curses.h>
#include <string.h>
#include <chrono>
#include <thread>
#include <locale.h>
#include <algorithm>
#include "tetris.hpp"

//FIXME: Bug when moving all to the right side when the piece is starting to falling

int main(int argc, char **argv)
{
    std::string name = "Fritz";
    char buffer[BUFSIZ];

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
                tetris::client_t cl(name);
                //join_game();
                mvprintw(12,3, "Insert IP of the host:"); // (also port if required) (?)
                echo();
                mvprintw(15,5, ">> ");
                echo();
                getnstr(buffer, BUFSIZ);
                noecho();
                cl.connect(buffer);
                cl.play();
                //join_game();
                break;
            }
            case '3':{
                //host_game();
                //fork;
                // host
                // connect
                break;
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
    } while (choice != 'q');
    
    endwin();

    return 0;
}