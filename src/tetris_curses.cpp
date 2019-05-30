#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
//#include <ncursesw/curses.h>
#include <curses.h>
#include <chrono>
#include <thread>
#include <locale.h>
#include <cstring>
#include <algorithm>
#include "tetris_base.hpp"
#include "tetris_curses.hpp"

namespace tetris{
    tetris_t::tetris_t(std::string name, int rows, int cols): game_t(rows,cols), name(name){
        setlocale(LC_ALL, "");
        initscr(); // initialize curses
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        timeout(0);  // don't wait for getch()
        curs_set(0); // invisible cursor
        clear();
        
        start_color();
        //init_color(8, 1000, 700, 30);
        //init_color(3, 1000, 700, 30);
        init_pair(block_type_t::I, COLOR_BLACK, COLOR_CYAN);
        init_pair(block_type_t::J, COLOR_BLACK, COLOR_BLUE);
        init_pair(block_type_t::L, COLOR_BLACK, COLOR_WHITE);
        init_pair(block_type_t::O, COLOR_BLACK, COLOR_YELLOW);
        init_pair(block_type_t::S, COLOR_BLACK, COLOR_GREEN);
        init_pair(block_type_t::T, COLOR_BLACK, COLOR_MAGENTA);
        init_pair(block_type_t::Z, COLOR_BLACK, COLOR_RED);
        //init_pair(8, COLOR_BLACK, 8);
        init_pair(block_type_t::TRASH, COLOR_BLACK, COLOR_WHITE);

        stored_w= newwin(ROWS_PER_CELL * TETRIS + 2, COLS_PER_CELL * TETRIS + 2, 0, 0);
        board_w = newwin(ROWS_PER_CELL * n_rows + 2, COLS_PER_CELL * n_cols + 2, 0, getmaxx(stored_w));
        next_w  = newwin(ROWS_PER_CELL * TETRIS + 2, COLS_PER_CELL * TETRIS + 2, 0,  getmaxx(stored_w) + getmaxx(board_w));
        score_w = newwin(ROWS_PER_CELL * TETRIS + 2, COLS_PER_CELL * TETRIS + 2, 14, getmaxx(stored_w) + getmaxx(board_w));
    }

    tetris_t::~tetris_t(){
        erase();
        echo();
        timeout(-1);
        nocbreak();
        curs_set(1);
        endwin();

        //FIXME: Delete this,
        std::cout << "Game over!" << std::endl;
        std::cout << "You have reach level " << getLevel() << ", your score was " << getScore() << std::endl;
    }

    void paint_block(WINDOW *w, int y, int x, block_type_t type){
        static const chtype BLOCK_STR[NUM_TETROMINOS+2][(COLS_PER_CELL+1)*ROWS_PER_CELL] = {
            {' ',               ' ',               0},  // EMPTY
            {' '|COLOR_PAIR(I), ' '|COLOR_PAIR(I), 0},  // I
            {' '|COLOR_PAIR(J), ' '|COLOR_PAIR(J), 0},  // J
            {' '|COLOR_PAIR(L), ' '|COLOR_PAIR(L), 0},  // L
            {' '|COLOR_PAIR(O), ' '|COLOR_PAIR(O), 0},  // O
            {' '|COLOR_PAIR(S), ' '|COLOR_PAIR(S), 0},  // S
            {' '|COLOR_PAIR(T), ' '|COLOR_PAIR(T), 0},  // T
            {' '|COLOR_PAIR(Z), ' '|COLOR_PAIR(Z), 0},  // Z
            {ACS_CKBOARD,       ACS_CKBOARD,       0},  // TRASH
        };  
        mvwaddchstr(w, y, x, BLOCK_STR[(int)type]);
    }

    void tetris_t::display_centered_message(WINDOW *w, std::string text){
        wclear(w);
        box(w, 0, 0);
        int lines = 1, len=0, max_len = 0;
        for(auto c: text) {
            len+=1;
            if (len>max_len)
                max_len=len;
            if (c=='\n') {
                len=0;
                lines++;
            }
        }
        wmove(w, (getmaxx(w)-lines)/2, (getmaxy(w)-max_len)/2);
        wprintw(w, text.c_str());
        wrefresh(w);
        timeout(-1);
        getch();
        timeout(0);
    }

    void tetris_t::draw_block(WINDOW * w, block_t block, std::string text){
        werase(w);
        box(w, 0, 0);
        wmove(w, 0, (getmaxx(w)-text.length())/2);
        wprintw(w, text.c_str());
        if (block.typ != block_type_t::EMPTY)
            for (int b = 0; b < TETRIS; b++) {
                location_t c = block.tetromino()[b];
                paint_block(w, c.y * ROWS_PER_CELL + 1, c.x * COLS_PER_CELL + 1, block.typ);
            }
        touchwin(w);
        wnoutrefresh(w);
    }

    void tetris_t::draw_board(bool drawfalling){
        werase(board_w);
        box(board_w, 0, 0);
        wmove(board_w, 0, (getmaxx(board_w)-name.length())/2);
        wprintw(board_w, name.c_str());
        for (int i = 0; i < rows(); i++)
            for (int j = 0; j < cols(); j++)
                paint_block(board_w, 1 + i * ROWS_PER_CELL, 1 + j * COLS_PER_CELL, operator()(i,j));
        if (drawfalling){
                    block_t block = getFalling();
            for (int b = 0; b < TETRIS; b++) {
                location_t c = block.tetromino()[b];
                if (bounded(block.loc.y + c.y, block.loc.x + c.x))
                    paint_block(board_w,  block.loc.y + c.y + 1, (block.loc.x + c.x) * COLS_PER_CELL + 1, block.typ);
            }
        }
        //wnoutrefresh(board_w);
        touchwin(board_w);
        wnoutrefresh(board_w);
    }

    void tetris_t::draw_score(){
        werase(score_w);
        wprintw(score_w, "Score:%4d", score);
        wprintw(score_w, "Level:%4d", level);
        wprintw(score_w, "Lines:%4d", lines_remaining);
        touchwin(score_w);
        wnoutrefresh(score_w);
    }
}
