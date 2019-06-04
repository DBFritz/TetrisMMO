#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <string>
#include <algorithm>
#include <sstream>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <utility>
#include <mutex>
#include <cerrno>
#include "tetris_base.hpp"
#include "tetris_server.hpp"

///////////////////////
//      MESSAGE      //
///////////////////////
namespace tetris{
    message_t::message_t(header_t header, content_type_t content, const void * payload, size_t payload_size){
        packet = (packet_t *)(malloc(sizeof(packet_t)+payload_size));
        packet->payload_size = payload_size;
        packet->header = header;
        packet->type_of_content = content;
        if (payload_size) {
            std::memcpy(packet->payload, payload, payload_size);
        }
    }
    message_t::message_t(packet_t packet): message_t(packet.header, packet.type_of_content, packet.payload, packet.payload_size){}

    message_t::message_t(header_t header): message_t(header, message_t::content_type_t::NONE){}

    message_t::message_t(header_t header, game_t &board)
    : message_t(header, message_t::content_type_t::BOARD, board.getBoard(), board.cols()*board.rows()*sizeof(block_type_t)){};

    message_t::message_t(header_t header, block_t block)
    : message_t(header, message_t::content_type_t::BLOCK, &block, sizeof(block_t)){}

    message_t::message_t(header_t header, std::string str)
    : message_t(header, message_t::content_type_t::STRING, const_cast<char *>(str.c_str()), str.length()+1){}

    message_t::message_t(header_t header, int32_t value)
    : message_t(header, message_t::content_type_t::NUMBER, &value, sizeof(int32_t)){}

    message_t::~message_t(){
        free(packet);
    }

    message_t & message_t::update(const void * payload){
        std::memcpy(packet->payload, payload, packet->payload_size);
        return *this;
    }
    message_t & message_t::update(const void * payload, uint16_t size){
        //TODO: Implement
        return *this;
    }

    message_t & message_t::change_endian(){
        //packet->header            //1-byte: unchanged
        //packet->type_of_content   //1-byte: unchanged
        packet->payload_size = packet->payload_size << 8 | packet->payload_size >> 8;
        switch(packet->type_of_content){
            case content_type_t::NUMBER:    //4-bytes
                std::swap(packet->payload[0],packet->payload[3]);
                std::swap(packet->payload[1],packet->payload[2]);
            case content_type_t::BLOCK:
                //packet->payload[0] // block_type_t typ;
                //packet->payload[1] // uint8_t ori;
                std::swap(packet->payload[2],packet->payload[3]);   //short
                std::swap(packet->payload[4],packet->payload[5]);   //short
            case content_type_t::BOARD:
            case content_type_t::STRING:
            case content_type_t::NONE:
                break;
        }
        return *this;
    }

    void message_t::send(int socket, bool change_endian){
        if (change_endian) this->change_endian();
        if (::send(socket, packet, size(), 0) != size()){
            throw "BAD SENDING PACKET";
        }
    }
    void message_t::send(int socket, std::mutex &mutex, bool change_endian){
        if (change_endian) this->change_endian();
        std::lock_guard<std::mutex> lck(mutex);
        send(socket);
    }

    message_t message_t::recv(int socket, std::mutex &mutex, bool change_endian){
        std::lock_guard<std::mutex> lck(mutex);
        return message_t::recv(socket, change_endian);
    }
    message_t message_t::recv(int socket, bool change_endian){
        
        message_t::packet_t * packet = new message_t::packet_t;
        ssize_t read = ::recv(socket, packet, sizeof(packet_t), 0);
        if (read != sizeof(packet_t) && read != 0){
            delete packet;
            throw "BAD RECIEVING PACKET";
        }
        if (read == 0) {
            packet->header = message_t::header_t::DISCONNECT;
            packet->type_of_content = message_t::content_type_t::NONE;
            packet->payload_size = 0;
        }
        uint8_t * payload = nullptr;
        if (packet->payload_size) {
            payload = new uint8_t[packet->payload_size];
            if (::recv(socket, payload, packet->payload_size, 0) != packet->payload_size){
                delete packet;
                delete[] payload;
                throw "BAD RECIEVING PAYLOAD";
            }
        }
        message_t newMessage(packet->header, packet->type_of_content, payload, packet->payload_size);
        if (change_endian) newMessage.change_endian();
        delete packet;
        if (payload != nullptr)
            delete[] payload;
        return newMessage;
    }

}

namespace tetris{
    server_t::server_t(int N, bool _verbose): verbose(_verbose), max_players(N), current_players(N){
        game            = new game_t[N];
        clients_socket  = new int[N];
        sock_mtx        = new std::mutex[N];
        trash_stacks    = new int[N];
        trash_mtx       = new std::mutex[N];
        names           = new std::string[N];
        attacked        = new int[N];
        attackers       = new std::vector<int>[N];

        for (int i=0; i<N;i++)
            attacked[i] = i+1;
        attacked[N-1] = 0;

        for (int i=N-1; i>0;i--)
            attackers[i].push_back(i-1);
        attackers[0].push_back(N-1);

        bzero(clients_socket,N*sizeof(int));
        
        set_command (message_t::header_t::MOVE,      &game_t::move);
        set_response(message_t::header_t::MOVE,      &server_t::send_falling);
        set_command (message_t::header_t::ROTATE,    &game_t::rotate);
        set_response(message_t::header_t::ROTATE,    &server_t::send_falling);
        set_command (message_t::header_t::DROP,      &game_t::drop);
        set_response(message_t::header_t::DROP,      &server_t::send_falling);
        set_command (message_t::header_t::HOLD,      &game_t::hold);
        set_response(message_t::header_t::HOLD,      &server_t::send_stored_and_falling);
        set_command (message_t::header_t::ACCELERATE,&game_t::accelerate);

        set_response(message_t::header_t::DISCONNECT,&server_t::disconnect);
        set_response(message_t::header_t::CHANGE_ATTACKED, &server_t::change_attacked);
    }

    server_t::~server_t(){
        running = false;
        shutdown(master_socket, 2);

        delete[] game;
        delete[] trash_stacks;
        delete[] trash_mtx;
        delete[] names;
        delete[] clients_socket;
        delete[] sock_mtx;
        delete[] attacked;
        delete[] attackers;
    }

    void server_t::set_command(message_t::header_t command, void (game_t::*fun)(int)){
        commands[command].is_with_argument = true;
        commands[command].fun.with_argument = fun;
    }
    void server_t::set_command(message_t::header_t command, void (game_t::*fun)()){
        commands[command].is_with_argument = false;
        commands[command].fun.wo_argument = fun;
    }

    void server_t::start(int port) {
        master_socket = socket(AF_INET , SOCK_STREAM , 0);
        if( master_socket == 0) throw "ERROR Creating Master_socket";

        int opt = true;   
        if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )    
            throw "ERROR setsockopt";

        address.sin_family = AF_INET;   
        address.sin_addr.s_addr = INADDR_ANY;   
        address.sin_port = htons( port );

        if ( bind(master_socket, (struct sockaddr *)(&address), sizeof(address)) < 0)
            throw "ERROR binding";         // Why vscode gets bugged with this cast?...

        if (verbose)
            std::cerr << "Listening on port" << port << std::endl;   

        if (listen(master_socket, 3) < 0)   
            throw "ERROR listening"; 

        if (verbose)
            std::cerr << "Waiting for connections ..." << std::endl;   
    }

    void server_t::run(){
        // Wait for all clients
        running = true;
        int addrlen = sizeof(address);
        for(int new_socket, i=0; i<max_players;i++){
            if ((new_socket = ::accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
                throw "ERROR accepting";
            if (verbose)
                std::cerr << "Player " << i << " conected, ip: " << inet_ntoa(address.sin_addr) << ':' << ntohs(address.sin_port) << std::endl; 
            if (verbose)
                std::cerr << "Sending HI\n" << std::endl;
            message_t(message_t::header_t::SERVERHI, 1).send(new_socket,sock_mtx[i]);
            clients_socket[i] = new_socket;
        }

        if (verbose)
            std::cerr << "All players are ready" << std::endl;

        // Say to all clients that we're ready
        for(int i=0;i<max_players;i++) {
            message_t msg = message_t::recv(clients_socket[i]);
            if (msg.getHeader() != message_t::header_t::CLIENTHI) throw "WTF";
            names[i] = std::string(static_cast<const char *>(msg.getPayload()));
        }
        for(int i=0;i<max_players;i++) {
            message_t(message_t::header_t::ATT_NAME, names[attacked[i]]).send(clients_socket[i], sock_mtx[i]);
            message_t(message_t::header_t::PLAY, game[i]).send(clients_socket[i], sock_mtx[i]);
        }
        
        for(int i=0;i<max_players;i++){
            std::thread t(&server_t::play, this, i);
            t.detach();
        }

        int max_sd = 0;
        fd_set fds;
        //FD_ZERO(&fds);
        while(running){
            for (int i = 0 ; i < max_players ; i++) {
                if (clients_socket[i]!= 0)
                    FD_SET(clients_socket[i],&fds);
                if(clients_socket[i] > max_sd)
                    max_sd = clients_socket[i];
            }
            
            if ((::select( max_sd + 1 , &fds , NULL , NULL , NULL) < 0) && (errno!=EINTR))
                std::cerr << "select error" << std::endl;

            for (int i = 0, sd; i < max_players; i++){
                sd = clients_socket[i];
                if (sd==0) continue;
                if (FD_ISSET( sd , &fds)) {
                    message_t newmsg = message_t::recv(sd);
                    FD_CLR(sd, &fds);
                    if (verbose) std::cerr << newmsg;
                    handle_message(i, &newmsg);
                }
            }
        }
    }

    void server_t::play(int player){
        while (!game[player].is_over())
        {
            int lines = game[player].update();
            if (lines > 0){      // LINES CLEARED
                //adding points
                message_t(message_t::header_t::POINTS, game[player].getScore()).send(clients_socket[player],sock_mtx[player]);
                
                //sending trash to the stack
                std::lock_guard<std::mutex> lck (trash_mtx[attacked[player]]);
                trash_stacks[ attacked[player] ] += lines-1;
                message_t(message_t::header_t::TRASH_STACK, trash_stacks[attacked[player]]).send(clients_socket[attacked[player]],sock_mtx[attacked[player]]);
                for(auto i: attackers[attacked[player]])
                    message_t(message_t::header_t::TRASH_ENEMY_STACK, trash_stacks[attacked[player]]).send(clients_socket[i],sock_mtx[i]);
            }

            if (lines >=-1)     // FALLING MOVED
                message_t(message_t::header_t::FALLING, game[player].getFalling()).send(clients_socket[player],sock_mtx[player]);
            
            if (lines >= 0) {   // PIECE_PUTTED
                // Add trash
                std::lock_guard<std::mutex> lck (trash_mtx[player]);
                if (trash_stacks[player] > 0) {
                    game[player].add_trash(trash_stacks[player]);
                    trash_stacks[player] = 0;
                    message_t(message_t::header_t::TRASH_STACK, 0).send(clients_socket[player],sock_mtx[player]);
                    for(auto i: attackers[player])
                        message_t(message_t::header_t::TRASH_ENEMY_STACK, 0).send(clients_socket[i],sock_mtx[i]);
                }
                lck.~lock_guard();

                // Send status of the board and next piece
                message_t(message_t::header_t::BOARD, game[player]).send(clients_socket[player],sock_mtx[player]);
                message_t(message_t::header_t::NEXT, game[player].getNext()).send(clients_socket[player],sock_mtx[player]);
                for(auto i: attackers[player])
                    message_t(message_t::header_t::ATTACKED, game[player]).send(clients_socket[i],sock_mtx[i]);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(16));

        }
        
        int posible_winner = attackers[player][0];
        disconnect(player, message_t(message_t::header_t::LOSES));

        if (current_players==1) {
            disconnect(posible_winner, message_t(message_t::header_t::WIN));
            std::this_thread::sleep_for(std::chrono::milliseconds(32));
        }

    }

    void server_t::change_attacked(int player){

        attackers[attacked[player]].erase(std::remove(attackers[attacked[player]].begin(), attackers[attacked[player]].end(), player), attackers[attacked[player]].end());
        for(int i=0;i<max_players;i++) {
            attacked[player]++;
            attacked[player] %= max_players;
            if (clients_socket[attacked[player]] != 0 && player != attacked[player])
                break;
        }

        message_t(message_t::header_t::ATT_NAME, names[attacked[player]]).send(clients_socket[player], sock_mtx[player]);
        message_t(message_t::header_t::ATTACKED, game[attacked[player]]).send(clients_socket[player], sock_mtx[player]);
        message_t(message_t::header_t::TRASH_ENEMY_STACK, trash_stacks[attacked[player]]).send(clients_socket[player], sock_mtx[player]);
        attackers[attacked[player]].push_back(player);

    }

    void server_t::disconnect(int player, message_t msg){
        if (clients_socket[player]==0) return;
        msg.send(clients_socket[player], sock_mtx[player]);
        disconnect(player);
    }

    void server_t::disconnect(int player){

        std::lock_guard<std::mutex> lck(disconnecting);
        if (clients_socket[player]==0) return;  // Already disconnected
        current_players--;
        attackers[attacked[player]].erase(std::remove(attackers[attacked[player]].begin(), attackers[attacked[player]].end(), player), attackers[attacked[player]].end());
        message_t msg = message_t(message_t::header_t::CHANGE_ATTACKED);

        int point_to_repart = game[player].getScore()*attackers[player].size()/(attackers[player].size()+1);
        for(auto i: attackers[player]){
            message_t(message_t::header_t::POINTS, game[i].addScore(point_to_repart)).send(clients_socket[i], sock_mtx[i]);
            handle_message(i,&msg);
        }

        shutdown(clients_socket[player], 0);
        clients_socket[player] = 0;
        if (current_players==0)
            running =false;

    }

    void server_t::handle_message(int player, message_t *msg){
        if ( msg == nullptr ) {    // Somebody disconnected // DEPRECATED, NOW IT'S A DISCONNECT MSG.
            if (verbose)
                std::cerr << "Player " << player << " disconnected" << std::endl;
            disconnect(player);
            return;
        }

        message_t::header_t header = msg->getHeader();
        if (commands.find(header) != commands.end()){
            if (commands[header].is_with_argument){
                (game[player].*(commands[header].fun.with_argument))(*static_cast<int32_t *>(const_cast<void *>(msg->getPayload())));
            } else {
                (game[player].*(commands[header].fun.wo_argument))();
            }
        }
        if (responses.find(header) != responses.end())
            (this->*responses[header])(player);
    }
}

// PRINTING STUFF,
// NOT USEFUL ANYMORE
std::ostream & operator << (std::ostream &out, const tetris::block_t &b){
    out << b.typ << ' ' << b.ori << ' ' << b.loc.first << ' ' << b.loc.second;
    return out;
}

std::ostream & operator << (std::ostream &out, tetris::game_t &g){
    for (int row=0; row< g.rows(); row++)
            for (int col=0; col< g.cols(); col++)
                out << g(row, col);
    return out;
}

std::ostream & operator << (std::ostream &out, tetris::message_t &msg){
    out << "Header_type:\t" << (static_cast<uint8_t>(msg.getHeader())&0x80 ? "SERVER_" : "CLIENT_");
    #define _CASE_MSG_TO_STR(x,y) case tetris::message_t::header_t::x: out << #y; break;
    #define CASE_MSG_TO_STR(x) _CASE_MSG_TO_STR(x,x)
    switch (msg.getHeader()) {
        _CASE_MSG_TO_STR(CLIENTHI,HI);      CASE_MSG_TO_STR(DROP);
        CASE_MSG_TO_STR(MOVE);              CASE_MSG_TO_STR(ROTATE);
        CASE_MSG_TO_STR(HOLD);              CASE_MSG_TO_STR(ACCELERATE);
        CASE_MSG_TO_STR(CHANGE_ATTACKED);   CASE_MSG_TO_STR(DISCONNECT);

        _CASE_MSG_TO_STR(SERVERHI,HI);      CASE_MSG_TO_STR(BOARD);
        CASE_MSG_TO_STR(ATTACKED);          CASE_MSG_TO_STR(ATT_NAME);
        CASE_MSG_TO_STR(FALLING);           CASE_MSG_TO_STR(NEXT);
        CASE_MSG_TO_STR(STORED);            CASE_MSG_TO_STR(POINTS);
        CASE_MSG_TO_STR(TRASH_STACK);       CASE_MSG_TO_STR(TRASH_ENEMY_STACK);
        CASE_MSG_TO_STR(WIN);               CASE_MSG_TO_STR(LOSES);

        default:
            out << static_cast<unsigned int>(msg.getHeader());
            break;
    }
    #undef CASE_MSG_TO_STR
    #undef _CASE_MSG_TO_STR
    out << std::endl;

    out << "Content_type:\t";
    switch(msg.getContentType()){
        case tetris::message_t::content_type_t::BLOCK:  out << "BLOCK"; break;
        case tetris::message_t::content_type_t::BOARD:  out << "BOARD"; break;
        case tetris::message_t::content_type_t::NONE:   out << "NONE";  break;
        case tetris::message_t::content_type_t::NUMBER: out << "NUMBER";break;
        case tetris::message_t::content_type_t::STRING: out << "STRING";break;
    }
    out << std::endl;

    out << "Payload_len:\t" << msg.getPayloadSize() << std::endl;

    //switch(msg.getContentType()){
    //    case tetris::message_t::content_type_t::BLOCK:
    //    case tetris::message_t::content_type_t::BOARD:
    //    case tetris::message_t::content_type_t::NONE:
    //    case tetris::message_t::content_type_t::NUMBER:
    //    case tetris::message_t::content_type_t::STRING:
    //}
    return out;
}