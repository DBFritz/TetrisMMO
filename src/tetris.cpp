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
        //keys.set('x',       &tetris::singleplayer_t::add_trash, 1);
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
        keys.set('a',       &tetris::client_t::accelerate, 1);
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        trash_w = newwin(ROWS_PER_CELL * server_t::trash_stack_size + 2, COLS_PER_CELL+2,
            getmaxy(board_w)-ROWS_PER_CELL*server_t::trash_stack_size-2, getmaxx(stored_w)-COLS_PER_CELL-2);
        trash_enemy_w = newwin(ROWS_PER_CELL * server_t::trash_stack_size + 2, COLS_PER_CELL+2, 
            getmaxy(board_w)-ROWS_PER_CELL*server_t::trash_stack_size-2, getmaxx(stored_w)+getmaxx(board_w)+getmaxx(next_w));
        enemy_w = newwin(ROWS_PER_CELL * n_rows + 2, COLS_PER_CELL * n_cols + 2,
            0, getmaxx(stored_w)+getmaxx(board_w)+getmaxx(next_w)+getmaxx(trash_enemy_w));
        enemy_board = new block_type_t[n_rows*n_cols];
        //keys.set('y',       &tetris::client_t::write_sms);            //NOT AVAILABLE NOW
        //chat_w = newwin(2, 64, getmaxy(board_w), getmaxx(trash_w));
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
        
        // recieve hi to determing same or different endian
        dif_endian = (*static_cast<const int *>(message_t::recv(sockfd).getPayload()) != 1);

        //say my name
        message_t(message_t::header_t::CLIENTHI, name).send(sockfd, dif_endian);
        //std::cerr << "Executing thread" << std::endl;
        socket_handler = std::thread(&client_t::handle_socket, this);
        socket_handler.detach();
        return true;
    }
    
    void client_t::play(){
        timeout(-1); //wait for input
        while(sockfd != 0){
            keys(*this, getch());
        }
    }
    
    void client_t::write_sms(){
        static char buffer[128];
        mvwprintw(chat_w,1,0,name.c_str());
        strcpy(buffer,name.c_str());
        strcat(buffer,": ");
        wprintw(chat_w,": ");
        echo();
        wnoutrefresh(chat_w);
        doupdate();
        wgetnstr(chat_w, buffer+strlen(buffer), 127-strlen(buffer));
        noecho();
        wmove(chat_w,1,0);
        wrefresh(chat_w);
        wclrtoeol(chat_w);
        wnoutrefresh(chat_w);
        message_t(message_t::header_t::SMS, std::string(buffer)).send(sockfd, dif_endian);
        fflush(stdin);
        doupdate();
    }

    void client_t::draw_sms(std::string sms){
        wmove(chat_w,0,0);
        wrefresh(chat_w);
        wclrtoeol(chat_w);
        mvwprintw(chat_w,0,0,sms.c_str());
        wnoutrefresh(chat_w);
    }

    void client_t::draw_enemy_board(std::string name){
        werase(enemy_w);
        box(enemy_w, 0, 0);
        wmove(enemy_w, 0, (getmaxx(enemy_w)-name.length())/2);
        wprintw(enemy_w, name.c_str());
        for (int i = 0; i < rows(); i++)
            for (int j = 0; j < cols(); j++)
                paint_block(enemy_w, 1 + i * ROWS_PER_CELL, 1 + j * COLS_PER_CELL, enemy_board[n_cols * i + j]);
        touchwin(enemy_w);
        wnoutrefresh(enemy_w);
    }

    void client_t::draw_trash_stack(WINDOW * w, int trash){
        werase(w);
        box(w, 0, 0);
        trash = (trash>server_t::trash_stack_size ? server_t::trash_stack_size : trash);
        for(int i=server_t::trash_stack_size-trash-1;i>=0;i--)
            paint_block(w, 1+i*ROWS_PER_CELL, 1, block_type_t::EMPTY);
        for(int i=server_t::trash_stack_size-trash;i<server_t::trash_stack_size; i++)
            paint_block(w, 1+i*ROWS_PER_CELL, 1, block_type_t::TRASH);
        touchwin(w);
        wnoutrefresh(w);
    }

    void client_t::move(int direction)  {message_t(message_t::header_t::MOVE, direction)  .send(sockfd, dif_endian); }
    void client_t::rotate(int direction){message_t(message_t::header_t::ROTATE, direction).send(sockfd, dif_endian); }
    void client_t::hold() { message_t(message_t::header_t::HOLD).send(sockfd, dif_endian);  }
    void client_t::drop() { message_t(message_t::header_t::DROP).send(sockfd, dif_endian);  }
    void client_t::accelerate(int r) {message_t(message_t::header_t::ACCELERATE, r)  .send(sockfd, dif_endian);  }
    void client_t::change_attacked() {message_t(message_t::header_t::CHANGE_ATTACKED).send(sockfd, dif_endian); }

    void client_t::handle_socket(){
        while (sockfd){
            message_t msg = message_t::recv(sockfd, dif_endian);
            //std::cerr << "New message!" << std::endl;
            switch (msg.getHeader()){
                case message_t::header_t::PLAY:
                    clear();
                    doupdate();
                    refresh();
                    draw_board(false);
                    draw_enemy_board(attacked);
                    draw_hold();
                    draw_next();
                    draw_score();
                    draw_trash_stack(trash_w,0);
                    draw_trash_stack(trash_enemy_w,0);
                    refresh();
                    doupdate();
                    break;
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
                    std::memcpy(enemy_board, msg.getPayload(), msg.getPayloadSize());
                    draw_enemy_board(attacked);
                    doupdate();
                    break;
                case message_t::header_t::NEXT:
                    std::memcpy(static_cast<void *>(&next), msg.getPayload(), msg.getPayloadSize());
                    draw_next();
                    doupdate();
                    break;
                case message_t::header_t::STORED:
                    std::memcpy(static_cast<void *>(&stored), msg.getPayload(), msg.getPayloadSize());
                    draw_hold();
                    doupdate();
                    break;
                case message_t::header_t::POINTS:
                    std::memcpy(static_cast<void *>(&score), msg.getPayload(), msg.getPayloadSize());
                    draw_score();
                    doupdate();
                    break;
                case message_t::header_t::TRASH_STACK:
                    int trash;
                    std::memcpy(static_cast<void *>(&trash), msg.getPayload(), msg.getPayloadSize());
                    draw_trash_stack(trash_w,trash);
                    doupdate();
                    break;
                case message_t::header_t::TRASH_ENEMY_STACK:
                    int enemy_trash;
                    std::memcpy(static_cast<void *>(&enemy_trash), msg.getPayload(), msg.getPayloadSize());
                    draw_trash_stack(trash_enemy_w,enemy_trash);
                    doupdate();
                    break;
                case message_t::header_t::ATT_NAME:
                    attacked = std::string(static_cast<const char *>(msg.getPayload()));
                    break;
                case message_t::header_t::SMS:
                    draw_sms(std::string(static_cast<const char *>(msg.getPayload())));
                    doupdate();
                    break;
                case message_t::header_t::WIN:
                    shutdown(sockfd, 2);
                    sockfd = 0;
                    display_centered_message(board_w, "You're a winner!");
                    break;
                case message_t::header_t::LOSES:
                    shutdown(sockfd, 2);
                    sockfd = 0;
                    display_centered_message(board_w, "You lose");
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