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
#include "tetris_server.hpp"

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
        set_command("MOVE", &game_t::move);
        set_command("ROTATE", &game_t::rotate);
        set_command("HOLD",&game_t::hold);
        set_command("DROP",&game_t::drop);
        set_command("ACCELERATE",&game_t::accelerate);
    }

    server_t::~server_t(){
        delete players;
        delete clients_socket;
    }

    void server_t::set_command(std::string command, void (game_t::*fun)(int)){
        //server_t::command_t new_command;
        //new_command.is_with_argument = true;
        //new_command.fun.with_argument = fun;
        commands[command].is_with_argument = true;
        commands[command].fun.with_argument = fun;
    }
    void server_t::set_command(std::string command, void (game_t::*fun)()){
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
            strcpy(buffer,"HI\n");
            if( send(new_socket, buffer, strlen(buffer), 0) != static_cast<ssize_t>(strlen(buffer)) )
                throw "ERROR sending HI";
            clients_socket[i] = new_socket;
        }

        // Say to all clients that we're ready
        printf("Sending PLAY\n");
        strcpy(buffer,"PLAY\n");
        for(int i=0;i<max_players;i++)
            if (send(clients_socket[i], buffer, strlen(buffer), 0) != static_cast<ssize_t>(strlen(buffer)))
                throw "ERROR sending PLAY";
        
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
                    buffer[recv(sd, buffer, BUFSIZ, 0)] = 0;
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
        for (int row=0; row< rows; row++)
            for (int col=0; col< cols; col++)
                sendstream << players[player](row, col);
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
        std::stringstream sendstream;
        while (clients_socket[player] != 0) {
            sendstream.clear();
            sendstream.str("");
            int lines = players[player].update();
            if (lines >= 0)
                sendboard(player);
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
        int value; // just in case
        recvstream.clear();
        if (verbose)
            std::cout << "Message from player " << player << ": " << buffer << std::endl;
        recvstream.str(buffer);
        while (recvstream >> buffer){
            if (commands.find(buffer) != commands.end()){
                if (commands[buffer].is_with_argument){
                    recvstream >> value;
                    (players[player].*(commands[buffer].fun.with_argument))(value);
                } else {
                    (players[player].*(commands[buffer].fun.wo_argument))();
                }
            }
            //if (buffer == "MOVE"){
            //    int direction;
            //    recvstream >> direction;
            //    players[player].move(direction);
            //} else if (buffer == "DROP"){
            //    players[player].drop();
            //} else if (buffer == "ROTATE"){
            //    int direction;
            //    recvstream >> direction;
            //    players[player].rotate(direction);
            //}
            sendboard(player);
        }
    }
}

std::ostream & operator << (std::ostream &out, const tetris::block_t &b){
    out << b.typ << ' ' << b.ori << ' ' << b.loc.first << ' ' << b.loc.second;
    return out;
}