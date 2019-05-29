#include <stdlib.h>
#include <random>
#include <cstring>
#include "tetris_base.hpp"

namespace tetris {
    game_t::game_t(int rows, int cols): 
        n_rows(rows), n_cols(cols)
    {
        board = new block_type_t[n_rows*n_cols];
        std::memset(board, block_type_t::EMPTY, sizeof(block_type_t) * rows * cols);
        ticks_till_gravity = TICKS_PER_LEVEL[level];
        lines_remaining = LINES_PER_LEVEL;
        new_falling();
        new_falling();
        stored.typ = block_type_t::EMPTY;
    }
    
    game_t::~game_t(){
        delete[] board;
    }
    
    bool game_t::bounded(int row, int col, bool exception)
    {
        if (exception) {
            if (col<0)
                throw exception_t::X_UNDERFLOW;
            else if (col>=n_cols)
                throw exception_t::X_OVERFLOW;
            else if (row>=n_rows)
                throw exception_t::Y_OVERFLOW;
            else if (row<0)
                throw exception_t::Y_UNDERFLOW;
        }
        else {
            return bounded(row, col);
        }
        return true;
    }
    
    block_type_t game_t::operator()(int row, int col){
        if (bounded(row, col))
            return board[n_cols * row + col];
        else
            throw "INVALID INDEX";
        return block_type_t::EMPTY;
    }

    block_type_t& game_t::get(int row, int col){
        return board[n_cols * row + col];
    }

    void game_t::set(int row, int col, block_type_t value){
        if (bounded(row, col))
            board[n_cols * row + col] = value;
    }

    void game_t::put(block_t block){
        for (int i = 0; i < TETRIS; i++) {
            location_t cell = block.tetromino()[i];
            set(block.loc.y + cell.y, block.loc.x + cell.x, block.typ);
        }
    }

    void game_t::remove(block_t block){
        for (int i = 0; i < TETRIS; i++) {
            location_t cell = block.tetromino()[i];
            set(block.loc.y + cell.y, block.loc.x + cell.x, block_type_t::EMPTY);
        }
    }

    bool game_t::fits(block_t block, bool exception){
        for (int i = 0; i < TETRIS; i++) {
            location_t cell = block.tetromino()[i];
            int row = block.loc.y + cell.y;
            int col = block.loc.x + cell.x;
            try{
                if (!bounded(row, col, exception) || !empty(row, col))
                    return false;
            } catch (exception_t exception){
                if (exception != Y_UNDERFLOW)
                    throw;
            }
        }
        return true;
    }

    void game_t::new_falling(){
        static std::uniform_int_distribution<int> uni(1,NUM_TETROMINOS);
        falling = next;
        next.typ = static_cast<block_type_t>(uni(rng));
        next.ori = 0;
        next.loc.y = -1;    //initial offset
        next.loc.x = cols()/2 - 2;
        ticks_till_gravity = TICKS_PER_LEVEL[level];
    }

    bool game_t::gravity_update()
    {
        ticks_till_gravity--;
        if (ticks_till_gravity <= 0) {
            falling.loc.y++;
            ticks_till_gravity = TICKS_PER_LEVEL[level];
            return -1;
        }
        return 0;
    }

    void game_t::move(int direction){
        try {
            falling.loc.x += direction;
            if (!fits(falling, true))
                falling.loc.x -= direction;
        } catch (exception_t err) {
            if (err!=Y_UNDERFLOW)
                falling.loc.x -= direction;
        }
    }

    void game_t::rotate(int direction){
        while(true) {
            falling.ori += direction + NUM_ORIENTATIONS;
            falling.ori %= NUM_ORIENTATIONS;
            try {
                if (fits(falling, true)) break;
            } catch (game_t::exception_t err){
                while (!fits(falling)) {
                    switch (err){
                        case X_OVERFLOW:
                            falling.loc.x--;
                            break;
                        case X_UNDERFLOW:
                            falling.loc.x++;
                            break;
                        case Y_OVERFLOW:
                            falling.loc.y--;
                            break;
                        case Y_UNDERFLOW:
                            return;
                        default:
                            falling.ori += direction + NUM_ORIENTATIONS;
                            falling.ori %= NUM_ORIENTATIONS;
                            break;
                    }
                }
            }
        }
    }

    void game_t::drop(){
        do falling.loc.y++; while (fits(falling));
        falling.loc.y--;
        ticks_till_gravity = -1;
    }

    void game_t::hold()
    {
        if (stored.typ == block_type_t::EMPTY) {
            stored = falling;
            new_falling();
        } else {
            block_type_t typ = falling.typ;
            int ori = falling.ori;
            falling.typ = stored.typ;
            falling.ori = stored.ori;
            stored.typ = typ;
            stored.ori = ori;
            try {   // FIXME: I'm not sure if this works
                while (!fits(falling,true) && falling.loc.y>0) //In case it doesn't fit
                    falling.loc.y--;
            } catch (exception_t err){
                while (!fits(falling)){
                    switch (err){
                        case X_OVERFLOW:
                            falling.loc.x--;
                            break;
                        case X_UNDERFLOW:
                            falling.loc.x++;
                            break;
                        case Y_OVERFLOW:
                            falling.loc.y--;
                            break;
                        case Y_UNDERFLOW:
                            falling.loc.y++;
                            break;
                    }
                }
            }
        }
    }

    bool game_t::line_full(int row){
        for (int j=0; j < n_cols; j++)
            if (get(row,j) == block_type_t::EMPTY)
                return false;
        return true;
    }

    void game_t::shift_lines(int row){
        for (int i=row-1;i>=0; i--)
            for(int j=0;j < n_cols; j++)
                set(i+1, j, get(i, j));
        for(int j=0;j < n_cols; j++)        
            set(0,j, block_type_t::EMPTY);
    }

    int game_t::remove_fulls(){
        int lines = 0;
        for (int i = n_rows-1; i >= 0; i--) {
            while (line_full(i)) {
                shift_lines(i);
                lines++;
            }
        }
        return lines;
    }

    void game_t::update_lines_remaining(int lines){
        if (lines < lines_remaining) {
            lines_remaining -= lines;
        } else {
            level++;
            if (level>MAX_LEVEL) level=MAX_LEVEL;
            lines_remaining += LINES_PER_LEVEL - lines;
        }
    }

    bool game_t::is_over(){
        for (int j = 0; j < n_cols; j++)
            if (get(0, j) != block_type_t::EMPTY)
                return true;
        return false;
    }

    void game_t::add_trash(int lines){
        static std::uniform_int_distribution<int> uni(0,n_cols-1);
        for (int i=lines;i<n_rows; i++)
            for(int j=0;j < n_cols; j++)
                set(i-lines, j, get(i, j));
        for (int i=n_rows-lines; i<n_rows; i++){
            int col = uni(rng);
            for(int j=0;j < n_cols; j++)
                set(i, j, (j!=col ?TRASH:EMPTY));
        }
    }
    
    void game_t::accelerate(int newremaining){
        ticks_till_gravity = newremaining;
    }

    int game_t::update(bool force){
        if (gravity_update() || force){
            if (!fits(falling)) {
                falling.loc.y--;
                put(falling);
                int lines = remove_fulls();
                score += score_calculator(lines);
                update_lines_remaining(lines);
                new_falling();
                return lines;
            }
            return -1;
        };
        return -2;
    }
}

