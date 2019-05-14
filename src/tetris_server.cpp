#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <string>
#include <sstream>
#include <arpa/inet.h>
#include <thread>
#include <chrono>
#include <unordered_map>
#include "tetris_base.hpp"
#include "tetris_server.hpp"


///////////////////////
//      MESSAGE      //
///////////////////////
namespace tetris{
    message_t::message_t(header_t header, content_type_t content, void * payload, size_t payload_size){
        packet = static_cast<packet_t *>(::operator new(sizeof(packet_t)+payload_size));
        packet->payload_size = payload_size;
        packet->header = header;
        packet->type_of_content = content;
        if (payload_size) {
            std::memcpy(packet->payload, payload, payload_size);
        }
    }
    message_t::message_t(header_t header): message_t(header, message_t::content_type_t::NONE){}

    message_t::message_t(header_t header, game_t &board) :
    message_t(header, message_t::content_type_t::BOARD,
        const_cast<tetris::block_type_t *>(board.getBoard()), 
        //(board.cols()<<sizeof(uint8_t)) | board.rows() ) {}   //Podría almacenar el ancho en un byte y el largo en el otro...
        board.cols()*board.rows()*sizeof(block_type_t)) {};

    message_t::message_t(header_t header, block_t &block)
    : message_t(header, message_t::content_type_t::BLOCK, &block, sizeof(block_t)){}

    message_t::message_t(header_t header, std::string str)
    : message_t(header, message_t::content_type_t::STRING, const_cast<char *>(str.c_str()), str.length()+1){}

    message_t::message_t(header_t header, int32_t value)
    : message_t(header, message_t::content_type_t::NUMBER, &value, sizeof(int32_t)){}

    message_t::~message_t(){
        delete packet;
    }

    void message_t::send(int socket){
        if (::send(socket, packet, size(), 0) != size())
            throw "BAD SENDING PACKET"; //FIXME
        return ;
    }

    message_t message_t::recv(int socket){
        message_t::packet_t * packet = new message_t::packet_t;
        if (::recv(socket, packet, sizeof(packet_t), 0) != sizeof(packet_t)){
            delete packet;
            throw "BAD RECIEVING PACKET";
        }
        uint8_t * payload = new uint8_t[packet->payload_size];
        if (::recv(socket, payload, packet->payload_size, 0) != packet->payload_size){
            delete packet;
            delete payload;
            throw "BAD RECIEVING PAYLOAD";
        }
        message_t newMessage(packet->header, packet->type_of_content, payload, packet->payload_size);
        delete packet;
        delete payload;
        return newMessage;
    }
}

namespace tetris{
    server_t::server_t(int N, bool _verbose): verbose(_verbose), max_players(N){
        players = new game_t[N];
        clients_socket = new int[N];
        attacked = new int[N];
        for (int i=0; i<N;i++)
            attacked[i] = i+1;
        attacked[N-1] = 0;
        bzero(clients_socket,N*sizeof(int));
        /// Setting up the default commands.
        set_command(message_t::header_t::MOVE,      &game_t::move);
        set_command(message_t::header_t::ROTATE,    &game_t::rotate);
        set_command(message_t::header_t::HOLD,      &game_t::hold);
        set_command(message_t::header_t::DROP,      &game_t::drop);
        set_command(message_t::header_t::ACCELERATE,&game_t::accelerate);
    }

    server_t::~server_t(){
        delete[] players;
        delete[] clients_socket;
        delete[] attacked;
    }

    void server_t::set_command(message_t::header_t command, void (game_t::*fun)(int)){
        //server_t::command_t new_command;
        //new_command.is_with_argument = true;
        //new_command.fun.with_argument = fun;
        commands[command].is_with_argument = true;
        commands[command].fun.with_argument = fun;
    }
    void server_t::set_command(message_t::header_t command, void (game_t::*fun)()){
        commands[command].is_with_argument = false;
        commands[command].fun.wo_argument = fun;
    }

    void server_t::run(int port){
        int opt = true;   
        int master_socket, addrlen;
        int max_sd;   
        char buffer[BUFSIZ];

        master_socket = socket(AF_INET , SOCK_STREAM , 0);
        if( master_socket == 0) throw "ERROR Creating Master_socket";

        if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )    
            throw "ERROR setsockopt";

        struct sockaddr_in address;
        address.sin_family = AF_INET;   
        address.sin_addr.s_addr = INADDR_ANY;   
        address.sin_port = htons( port );

        if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)   
            throw "ERROR binding";

        if (verbose)
            std::cerr << "Listening on port" << port << std::endl;   

        // FIXME: why 3?
        if (listen(master_socket, 3) < 0)   
            throw "ERROR listening"; 

        addrlen = sizeof(address);
        if (verbose)
            std::cerr << "Waiting for connections ..." << std::endl;   
    
        // Wait for all clients
        for(int new_socket, i=0; i<max_players;i++){
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
                    throw "ERROR accepting";
            if (verbose)
                std::cerr << "Player " << i << " conected, ip: " << inet_ntoa(address.sin_addr) << ':' << ntohs(address.sin_port) << std::endl; 
            if (verbose)
                std::cerr << "Sending HI\n" << std::endl;
            message_t(message_t::header_t::SERVERHI, 1).send(clients_socket[i]);
            clients_socket[i] = new_socket;
        }

        // Say to all clients that we're ready
        strcpy(buffer,"PLAY\n");
        for(int i=0;i<max_players;i++)
            message_t(message_t::header_t::PLAY, players[i]).send(clients_socket[i]);
        
        for(int i=0;i<max_players;i++){
            std::thread t(&server_t::play, this, i);
            t.detach();
        }

        fd_set fds;
        FD_ZERO(&fds);
        while(true){
            for (int i = 0 ; i < max_players ; i++) {
                if (clients_socket[i]!= 0)
                    FD_SET(clients_socket[i],&fds);
                if(clients_socket[i] > max_sd)
                    max_sd = clients_socket[i];
            }
            
            if ((select( max_sd + 1 , &fds , NULL , NULL , NULL) < 0) && (errno!=EINTR))
                std::cerr << "select error" << std::endl;

            for (int i = 0, sd; i < max_players; i++){
                sd = clients_socket[i];
                if (sd==0) continue;
                if (FD_ISSET( sd , &fds)) {
                    buffer[::recv(sd, buffer, BUFSIZ, 0)] = 0;
                    handle_message(i, std::string(buffer));
                    FD_CLR(sd, &fds);
                }
            }
        }
    }
    void server_t::sendboard(int player){
        static int rows= players[player].rows(), cols=players[player].cols();
        std::stringstream sendstream;
        sendstream << "BOARD ";
        sendstream << players[player] << std::endl;
        sendstream << std::endl;
        sendstream << "FALLING ";
        sendstream << players[player].getFalling() << std::endl;
        sendstream << "NEXT ";
        sendstream << players[player].getNext() << std::endl;
        sendstream << "STORED ";
        sendstream << players[player].getStored() << std::endl;
        sendstream << "ATTACKED ";
        for (int row=0; row< rows; row++)
            for (int col=0; col< cols; col++)
                sendstream << players[attacked[player]](row, col);
        sendstream << std::endl;
        send(clients_socket[player], sendstream.str().c_str(), sendstream.str().length(), 0);
    }

    void server_t::play(int player){
        while (clients_socket[player] != 0) {
            int lines = players[player].update();
            if (lines >= 0) {
                send_command("BOARD",   players[player], clients_socket[player]);
                send_command("FALLING", players[player].getFalling(), clients_socket[player]);
                send_command("NEXT",    players[player].getNext(), clients_socket[player]);
                send_command("STORED",  players[player].getStored(), clients_socket[player]);
                send_command("ATTACKED", players[attacked[player]], clients_socket[player]);
                //sendboard(player);
            }
            if (lines > 0)
                players[attacked[player]].add_trash(lines-1);
            //if (players[player].is_over()) {
            //    shutdown(clients_socket[player],2);
            //    clients_socket[player] = 0;
            //}
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    void server_t::handle_message(int player, std::string buffer){
        if ( buffer.length() == 0 ) {    // Somebody disconnected
            if (verbose)
                std::cout << "Player " << player << " disconnected" << std::endl;
            shutdown(clients_socket[player],2);
            clients_socket[player] = 0;
            return;
        }
        static std::stringstream recvstream;
        //int value; // just in case
        recvstream.clear();
        if (verbose)
            std::cout << "Message from player " << player << ": " << buffer << std::endl;
        recvstream.str(buffer);
        while (recvstream >> buffer){/*
            if (commands.find(buffer) != commands.end()){
                if (commands[buffer].is_with_argument){
                    recvstream >> value;
                    (players[player].*(commands[buffer].fun.with_argument))(value);
                } else {
                    (players[player].*(commands[buffer].fun.wo_argument))();
                }
            }*/
            
            send_command("BOARD",   players[player], clients_socket[player]);
            send_command("FALLING", players[player].getFalling(), clients_socket[player]);
            send_command("NEXT",    players[player].getNext(), clients_socket[player]);
            send_command("STORED",  players[player].getStored(), clients_socket[player]);
            send_command("ATTACKED", players[attacked[player]], clients_socket[player]);
        }
    }
}

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