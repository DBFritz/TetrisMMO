#ifndef _TETRIS_BASE_
#define _TETRIS_BASE_

//#include <string>
#include <random>

namespace tetris {
    const int TETRIS{4};
    const int NUM_TETROMINOS{7};
    const int NUM_ORIENTATIONS{4};
    const int SCORE_MULTIPLIER{10};
    
    static std::random_device rd;
    static std::mt19937 rng(rd()); // ojalá estuviera en la clase como static

    const int MAX_LEVEL{20};
    const int LINES_PER_LEVEL{10};

    enum block_type_t { 
        EMPTY, I, J, L, O, S, T, Z, TRASH
    };

    #define y first
    #define x second
    typedef std::pair<int,int> location_t;

    // from: https://github.com/brenns10/tetris
    const location_t TETROMINOS[NUM_TETROMINOS][NUM_ORIENTATIONS][TETRIS] = {
        // I
        {{{1, 0}, {1, 1}, {1, 2}, {1, 3}},  //    □ □ □ □     □ □ ■ □     □ □ □ □     □ ■ □ □
         {{0, 2}, {1, 2}, {2, 2}, {3, 2}},  //    ■ ■ ■ ■     □ □ ■ □     □ □ □ □     □ ■ □ □
         {{3, 0}, {3, 1}, {3, 2}, {3, 3}},  //    □ □ □ □     □ □ ■ □     ■ ■ ■ ■     □ ■ □ □
         {{0, 1}, {1, 1}, {2, 1}, {3, 1}}}, //    □ □ □ □     □ □ ■ □     □ □ □ □     □ ■ □ □   
        // J
        {{{0, 0}, {1, 0}, {1, 1}, {1, 2}},   //    ■ □ □ □     □ □ □ □     □ □ □ □     □ □ □ □
         {{0, 1}, {0, 2}, {1, 1}, {2, 1}},   //    ■ ■ ■ □     ■ ■ □ □     ■ ■ ■ □     ■ ■ □ □
         {{1, 0}, {1, 1}, {1, 2}, {2, 2}},   //    □ □ □ □     □ ■ □ □     □ □ ■ □     ■ □ □ □
         {{0, 1}, {1, 1}, {2, 0}, {2, 1}}},  //    □ □ □ □     □ ■ □ □     □ □ □ □     ■ □ □ □ 
        // L
        {{{0, 2}, {1, 0}, {1, 1}, {1, 2}},   //    □ □ ■ □     □ ■ □ □     □ □ □ □     ■ ■ □ □
         {{0, 1}, {1, 1}, {2, 1}, {2, 2}},   //    ■ ■ ■ □     □ ■ □ □     ■ ■ ■ □     □ ■ □ □
         {{1, 0}, {1, 1}, {1, 2}, {2, 0}},   //    □ □ □ □     □ ■ ■ □     ■ □ □ □     □ ■ □ □
         {{0, 0}, {0, 1}, {1, 1}, {2, 1}}},  //    □ □ □ □     □ □ □ □     □ □ □ □     □ □ □ □
        // O
        {{{0, 1}, {0, 2}, {1, 1}, {1, 2}},   //    □ ■ ■ □     □ ■ ■ □     □ ■ ■ □     □ ■ ■ □
         {{0, 1}, {0, 2}, {1, 1}, {1, 2}},   //    □ ■ ■ □     □ ■ ■ □     □ ■ ■ □     □ ■ ■ □
         {{0, 1}, {0, 2}, {1, 1}, {1, 2}},   //    □ □ □ □     □ □ □ □     □ □ □ □     □ □ □ □
         {{0, 1}, {0, 2}, {1, 1}, {1, 2}}},  //    □ □ □ □     □ □ □ □     □ □ □ □     □ □ □ □
        // S
        {{{0, 1}, {0, 2}, {1, 0}, {1, 1}},   //    □ ■ ■ □     □ ■ □ □     □ □ □ □     ■ □ □ □
         {{0, 1}, {1, 1}, {1, 2}, {2, 2}},   //    ■ ■ □ □     □ ■ ■ □     □ ■ ■ □     ■ ■ □ □
         {{1, 1}, {1, 2}, {2, 0}, {2, 1}},   //    □ □ □ □     □ □ ■ □     ■ ■ □ □     □ ■ □ □
         {{0, 0}, {1, 0}, {1, 1}, {2, 1}}},  //    □ □ □ □     □ □ □ □     □ □ □ □     □ □ □ □
        // T
        {{{0, 1}, {1, 0}, {1, 1}, {1, 2}},   //    □ ■ □ □     □ ■ □ □     □ □ □ □     □ ■ □ □
         {{0, 1}, {1, 1}, {1, 2}, {2, 1}},   //    ■ ■ ■ □     □ ■ ■ □     ■ ■ ■ □     ■ ■ □ □
         {{1, 0}, {1, 1}, {1, 2}, {2, 1}},   //    □ □ □ □     □ ■ □ □     □ ■ □ □     □ ■ □ □
         {{0, 1}, {1, 0}, {1, 1}, {2, 1}}},  //    □ □ □ □     □ □ □ □     □ □ □ □     □ □ □ □
        // Z
        {{{0, 0}, {0, 1}, {1, 1}, {1, 2}},   //    ■ ■ □ □     □ □ ■ □     □ □ □ □     □ ■ □ □
         {{0, 2}, {1, 1}, {1, 2}, {2, 1}},   //    □ ■ ■ □     □ ■ ■ □     ■ ■ □ □     ■ ■ □ □
         {{1, 0}, {1, 1}, {2, 1}, {2, 2}},   //    □ □ □ □     □ ■ □ □     □ ■ ■ □     ■ □ □ □
         {{0, 1}, {1, 0}, {1, 1}, {2, 0}}},  //    □ □ □ □     □ □ □ □     □ □ □ □     □ □ □ □
    };

    class block_t{
        public:
        block_type_t typ;
        int ori;
        location_t loc;

        int itype(){ return static_cast<int>(typ)-1; }
        const location_t *tetromino(){ return TETROMINOS[itype()][ori]; }
    };

    // from: https://tetris.fandom.com/wiki/Tetris_(Game_Boy)
    const int TICKS_PER_LEVEL[MAX_LEVEL+1] = {
        53, 49, 45, 41, 37, 33, 28, 22, 17, 11, 10, 9, 8, 7, 6, 6, 5, 5, 4, 4, 3
    };
    
    class game_t{
    protected:
        static int score_calculator(int l){
            return (l ? SCORE_MULTIPLIER*(1<<(l-1)) : 0);
        }

        int n_rows;
        int n_cols;
        block_type_t *board;

        int score = 0;
        int level = 0;

        block_t falling;
        block_t next;
        block_t stored;

        int ticks_till_gravity;
        int lines_remaining;

        void set(int row, int column, block_type_t value = block_type_t::EMPTY);
        block_type_t &get(int row, int column);

        bool line_full(int row);
        void shift_lines(int row);
        int remove_fulls();

        void put(block_t block);
        void remove(block_t block);

        bool fits(block_t block, bool exception = false);
        bool gravity_update();
        void update_lines_remaining(int lines);
        void new_falling();

    public:
        enum exception_t {
            X_UNDERFLOW, X_OVERFLOW,
            Y_UNDERFLOW, Y_OVERFLOW
        };

        game_t(int rows = 20, int cols = 10);
        ~game_t();

        int update();
        bool is_over();

        block_type_t operator()(int row, int col);
        int getScore(){ return score; }
        int getLinesRemaining(){ return lines_remaining; }
        int getLevel(){ return level; }
        block_t getNext(){ return next; }
        block_t getFalling(){ return falling; }
        block_t getStored(){ return stored; }

        bool bounded(int row, int col){ return 0<=row && row<n_rows && 0<=col && col <n_cols; }
        bool bounded(int row, int col, bool exception);

        bool empty(int row, int col){ return operator()(row,col) == EMPTY; }
        bool filled(int row, int col){ return !empty(row, col); }

        int rows() { return n_rows; };
        int cols() { return n_cols; };

        void move(int direction);
        void rotate(int direction);
        void drop();
        void hold();
        void add_trash(int lines = 1);
        void accelerate(int speed = 1);
    };
}
#endif