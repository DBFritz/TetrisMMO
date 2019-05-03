#ifndef _TETRIS_CURSES_
#define _TETRIS_CURSES_

#include <iostream>
#include <stdlib.h>
//#include <ncursesw/curses.h>
#include <curses.h>
#include <chrono>
#include <cstring>
#include <thread>
#include <locale.h>
#include <algorithm>
#include "tetris_base.hpp"

#define NORMAL_BLOCK {' ', ' ', 0}
namespace tetris{
    const int COLS_PER_CELL{2};
    const int ROWS_PER_CELL{1};
    /*
    const chtype BLOCK_STR[NUM_TETROMINOS+2][(COLS_PER_CELL+1)*ROWS_PER_CELL] = {
        {' ', ' '},                  // EMPTY
        {' ', ' '},                   // I
        {' ', ' '},                   // J
        {' ', ' '},                   // L
        {' ', ' '},                   // O
        {' ', ' '},                   // S
        {' ', ' '},                   // T
        {' ', ' '},                   // Z
        {ACS_CKBOARD, ACS_CKBOARD},  // TRASH
    };          /// FIXME: DOESN'T WORK HERE: MOVED TO .CPP
    */  
    //const std::string BLOCK_STR[NUM_TETROMINOS+2] = {
    //    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  "
    //    //L"  \n  ", L"  \n  ", L"  \n  ", L"  \n  ", L"  \n  ", L"  \n  ", L"  \n  ", L"  \n  ", L"▓▓\n▓▓"
    //};

    //const std::vector<std::vector<int>> colours({
    //    {   0,   0,   0},
    //    {   0, 704, 984},
    //    {   0,   0,1000},
    //    {1000, 500,  10},
    //    {1000,1000,   0},
    //    {   0,1000,   0},
    //    { 588, 448, 876},
    //    {1000,   0,   0},
    //    {1000,1000,1000}
    //});

    class window_t{
        WINDOW * self;
        bool with_box;
        void draw_block(block_type_t type, bool origin = true);
    };

    template <class T>
    class keys_t {
    private:
        struct key_t {
            bool is_with_argument;
            union fun_t {
                void (T::*with_argument)(int);
                void (T::*wo_argument)();
            } fun;
            int argument;
        };
        key_t keys[KEY_MAX];
        bool is_defined[KEY_MAX];

    public:
        keys_t();
        
        typename key_t::fun_t operator()(int key){ return keys[key].fun; }

        void set(int key, void (T::*fun)(int), int arg);
        void set(int key, void (T::*fun)());

        void remove(int key){ is_defined[key] = false; }

        void operator()(T &game, int key);
    };

    class layout_t{

    };
    void paint_block(WINDOW *w, int y, int x, block_type_t type);
    class tetris_t : public game_t{
    protected:
        std::string name;
        WINDOW *board_w, *stored_w, *next_w, *score_w;
        void draw_block(WINDOW *w, block_t block, std::string text = "");
    public:

        void draw_board();
        void draw_falling();
        void draw_hold(std::string text = "Stored") { draw_block(stored_w,getStored(), text); }
        void draw_next(std::string text = "Next") { draw_block(next_w,getNext(), text); }
        void draw_score();

        tetris_t(std::string name = "Unnamed", int rows = 20, int cols = 10);
        ~tetris_t();
    };

    template <class T>
    keys_t<T>::keys_t(){
        std::memset(static_cast<void *>(is_defined), false, KEY_MAX*sizeof(bool));
    }

    template <class T>
    void keys_t<T>::set(int key, void (T::*fun)(int), int arg){
        is_defined[key] = true;
        keys[key].is_with_argument = true;
        keys[key].fun.with_argument = fun;
        keys[key].argument = arg;
    }

    template <class T>
    void keys_t<T>::set(int key, void (T::*fun)(void)){
        is_defined[key] = true;
        keys[key].is_with_argument = false;
        keys[key].fun.wo_argument = fun;
    }

    template <class T>
    void keys_t<T>::operator()(T &game, int key){
        if (is_defined[key]) {
            if (keys[key].is_with_argument){
                (game.*(keys[key].fun.with_argument))(keys[key].argument);
            } else {
                (game.*(keys[key].fun.wo_argument))();
            }
        }
    }
}

#endif
