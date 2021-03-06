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
        std::vector<bool> is_defined;

    public:
        keys_t();
        
        typename key_t::fun_t operator()(int key){ return keys[key].fun; }

        void set(int key, void (T::*fun)(int), int arg);
        void set(int key, void (T::*fun)());

        void remove(int key){ is_defined[key] = false; }

        void operator()(T &game, int key);
    };

    void paint_block(WINDOW *w, int y, int x, block_type_t type);

    class tetris_t : public game_t{
    protected:
        std::string name;
        WINDOW *board_w, *stored_w, *next_w, *score_w;
        void draw_block(WINDOW *w, block_t block, std::string text = "");
        
        void display_centered_message(WINDOW *w, std::string text, bool need_char = false);
    public:

        void draw_board(bool drawfalling = true);
        void draw_falling();
        void draw_hold(std::string text = "Stored") { draw_block(stored_w,getStored(), text); }
        void draw_next(std::string text = "Next") { draw_block(next_w,getNext(), text); }
        void draw_score();
        
        tetris_t(std::string name = "Unnamed", int rows = 20, int cols = 10);
        ~tetris_t();
    };

    template <class T>
    keys_t<T>::keys_t(){
        is_defined.resize(KEY_MAX, false);
        //std::memset(static_cast<void *>(is_defined), false, KEY_MAX*sizeof(bool));
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
        if (0<=key && static_cast<unsigned>(key) < is_defined.size()){
            if (is_defined[key]) {
                if (keys[key].is_with_argument){
                    (game.*(keys[key].fun.with_argument))(keys[key].argument);
                } else {
                    (game.*(keys[key].fun.wo_argument))();
                }
            }
        }
    }
}

#endif
