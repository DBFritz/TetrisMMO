#include <string>
#include <sys/socket.h>
//#include <netinet/in.h>
#include <netdb.h>
#include "tetris.hpp"
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
        keys.set('x',       &tetris::client_t::add_trash, 1);
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
    }
    client_t::~client_t(){
        shutdown(sockfd, 2);
    }

    bool client_t::connect(std::string _server){
        struct hostent *server = gethostbyname(_server.c_str());
        struct sockaddr_in serv_addr;
        if (server == NULL) throw "NO HOST";

        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
        serv_addr.sin_port = htons(PORT);
        if (::connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
            throw "CONNECTION ERROR";
        return true;
    }
    
    void client_t::move(int direction){
        send(sockfd, "MOVE 1", strlen("MOVE 1"), 0);
    }

    void client_t::play(){
        timeout(-1); //wait for input
        while(1){
            keys(*this, getch());
        }
    }
}