#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <ncursesw/curses.h>
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
        
        start_color();
        init_pair(block_type_t::I, COLOR_BLACK, COLOR_CYAN);
        init_pair(block_type_t::J, COLOR_BLACK, COLOR_BLUE);
        init_pair(block_type_t::L, COLOR_BLACK, COLOR_WHITE);
        init_pair(block_type_t::O, COLOR_BLACK, COLOR_YELLOW);
        init_pair(block_type_t::S, COLOR_BLACK, COLOR_GREEN);
        init_pair(block_type_t::T, COLOR_BLACK, COLOR_MAGENTA);
        init_pair(block_type_t::Z, COLOR_BLACK, COLOR_RED);
        init_pair(block_type_t::TRASH, COLOR_BLACK, COLOR_WHITE);

        stored_w= newwin(ROWS_PER_CELL * TETRIS + 2, COLS_PER_CELL * TETRIS + 2, 0, 0);
        board_w = newwin(ROWS_PER_CELL * n_rows + 2, COLS_PER_CELL * n_cols + 2, 0, getmaxx(stored_w));
        next_w  = newwin(ROWS_PER_CELL * TETRIS + 2, COLS_PER_CELL * TETRIS + 2, 0,  getmaxx(stored_w) + getmaxx(board_w));
        score_w = newwin(ROWS_PER_CELL * TETRIS + 2, COLS_PER_CELL * TETRIS + 2, 14, getmaxx(stored_w) + getmaxx(board_w));
    }

    tetris_t::~tetris_t(){
        erase();
        endwin();

        std::cout << "Game over!" << std::endl;
        std::cout << "You have reach level " << getLevel() << ", your score was " << getScore() << std::endl;
    }

    void paint_block(WINDOW *w, int y, int x, block_type_t type){
        wattron(w, COLOR_PAIR(type));
        mvwaddwstr(w, y, x, BLOCK_STR[type].c_str());
        wattroff(w, COLOR_PAIR(type));
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
        wnoutrefresh(w);
    }
    void tetris_t::draw_board(){
        int i, j;
        box(board_w, 0, 0);
        wmove(board_w, 0, (getmaxx(board_w)-name.length())/2);
        wprintw(board_w, name.c_str());
        for (i = 0; i < rows(); i++)
            for (j = 0; j < cols(); j++)
                paint_block(board_w, 1 + i * ROWS_PER_CELL, 1 + j * COLS_PER_CELL, operator()(i,j));
        block_t block = getFalling();
        for (int b = 0; b < TETRIS; b++) {
            location_t c = block.tetromino()[b];
            if (bounded(block.loc.y + c.y, block.loc.x + c.x))
                paint_block(board_w,  block.loc.y + c.y + 1, (block.loc.x + c.x) * COLS_PER_CELL + 1, block.typ);
        }
        wnoutrefresh(board_w);
    }

    void tetris_t::draw_score(){
        werase(score_w);
        wprintw(score_w, "Score:%4d", score);
        wprintw(score_w, "Level:%4d", level);
        wprintw(score_w, "Lines:%4d", lines_remaining);
        wnoutrefresh(score_w);
    }
}