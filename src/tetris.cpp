#include <string>
#include <sys/socket.h>
#include <ncurses.h>
//#include <netinet/in.h>
#include <netdb.h>
#include <sstream>
#include <thread>
#include "tetris.hpp"
#include "tetris_server.hpp"
#define PORT 8558

namespace tetris{
    singleplayer_t::singleplayer_t(std::string name, int rows, int cols): tetris_t(name, rows, cols){
        keys.set(KEY_UP,    &tetris::singleplayer_t::rotate,  -1);
        keys.set(KEY_LEFT,  &tetris::singleplayer_t::move,    -1);
        keys.set(KEY_RIGHT, &tetris::singleplayer_t::move,     1);
        keys.set(KEY_RIGHT, &tetris::singleplayer_t::move,     1);
        keys.set(' ',       &tetris::singleplayer_t::hold);
        keys.set(KEY_DOWN,  &tetris::singleplayer_t::drop);
        keys.set('x',       &tetris::singleplayer_t::add_trash, 1);
        keys.set('c',       &tetris::singleplayer_t::accelerate, 2);
    }

    void singleplayer_t::play(){
        while (!is_over()) {
            update();
            draw_board();
            draw_next("Next");
            draw_hold("Stored");
            draw_score();
            doupdate();
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            keys(*this, getch());
        }
    }

    client_t::client_t(std::string name): tetris_t(name){
        keys.set(KEY_UP,    &tetris::client_t::rotate,  -1);
        keys.set(KEY_LEFT,  &tetris::client_t::move,    -1);
        keys.set(KEY_RIGHT, &tetris::client_t::move,     1);
        keys.set(KEY_RIGHT, &tetris::client_t::move,     1);
        keys.set(' ',       &tetris::client_t::hold);
        keys.set(KEY_DOWN,  &tetris::client_t::drop);
        keys.set('c',       &tetris::client_t::change_attacked);
        //keys.set('s',       &tetris::client_t::add_trash, 1);
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        enemy_w = newwin(ROWS_PER_CELL * n_rows + 2, COLS_PER_CELL * n_cols + 2, 0,  getmaxx(stored_w) + getmaxx(board_w) + getmaxx(next_w));
        enemy_board = new block_type_t[n_rows*n_cols];
    }
    client_t::~client_t(){
        shutdown(sockfd, 2);
        delete[] enemy_board;
    }

    bool client_t::connect(std::string _server){
        struct hostent *server = gethostbyname(_server.c_str());
        struct sockaddr_in serv_addr;
        if (server == NULL) throw "NO HOST";

        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
        serv_addr.sin_port = htons(PORT);
        if (::connect(sockfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
            throw "CONNECTION ERROR";
        //std::cerr << "Executing thread" << std::endl;
        socket_handler = std::thread(&client_t::handle_socket, this);
        socket_handler.detach();
        return true;
    }

    void client_t::send(std::string command, int value){
        try {
            stream.clear();
            stream.str("");
            stream << command << ' ' << value << std::endl;
            ::send(sockfd, stream.str().c_str(), stream.str().length(), 0);
        } catch(char const * err){
            std::cerr << err << std::endl;
            throw;
        }
    }

    void client_t::send(std::string command){
        try {
            stream.clear();
            stream.str("");
            stream << command << std::endl;
            ::send(sockfd, stream.str().c_str(), stream.str().length(), 0);
        } catch (char const * err){
            std::cerr << err << std::endl;
            throw;
        }
    }
    
    void client_t::play(){
        timeout(-1); //wait for input
        while(1){
            keys(*this, getch());
        }
    }
    
    void client_t::draw_enemy_board(std::string name){
        werase(enemy_w);
        box(enemy_w, 0, 0);
        wmove(enemy_w, 0, (getmaxx(enemy_w)-name.length())/2);
        wprintw(enemy_w, name.c_str());
        for (int i = 0; i < rows(); i++)
            for (int j = 0; j < cols(); j++)
                paint_block(enemy_w, 1 + i * ROWS_PER_CELL, 1 + j * COLS_PER_CELL, enemy_board[n_cols * i + j]);
        wnoutrefresh(enemy_w);
    }

    void client_t::move(int direction)  {message_t(message_t::header_t::MOVE, direction)  .send(sockfd); }
    void client_t::rotate(int direction){message_t(message_t::header_t::ROTATE, direction).send(sockfd); }
    void client_t::hold() { message_t(message_t::header_t::HOLD).send(sockfd);  }
    void client_t::drop() { message_t(message_t::header_t::DROP).send(sockfd);  }
    void client_t::accelerate(int r) {message_t(message_t::header_t::ACCELERATE, r)  .send(sockfd);  }
    void client_t::change_attacked() {message_t(message_t::header_t::CHANGE_ATTACKED).send(sockfd); }

    void client_t::handle_socket(){
        while (sockfd){
            message_t msg = message_t::recv(sockfd);
            //std::cerr << "New message!" << std::endl;
            switch (msg.getHeader()){
                case message_t::header_t::BOARD:
                    std::memcpy(board, msg.getPayload(),msg.getPayloadSize());
                    draw_board();
                    doupdate();
                    break;
                case message_t::header_t::FALLING:
                    std::memcpy(static_cast<void *>(&falling), msg.getPayload(), msg.getPayloadSize());
                    draw_board();
                    doupdate();
                    break;
                case message_t::header_t::ATTACKED:
                    std::memcpy(&enemy_board, msg.getPayload(), msg.getPayloadSize());
                    draw_board(false);
                    doupdate();
                    break;
                case message_t::header_t::NEXT:
                    break;
                case message_t::header_t::STORED:
                    break;
                case message_t::header_t::POINTS:
                    break;
                case message_t::header_t::WIN:
                    break;
                default:
                    break;
            }
        }
    }
}

std::istream & operator >> (std::istream &in, tetris::block_t &b){
    in >> b.ori;
    b.typ = static_cast<tetris::block_type_t>(b.ori);
    in >> b.ori >> b.loc.first >> b.loc.second;
    return in;
}