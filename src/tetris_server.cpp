#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <arpa/inet.h>
#include "tetris_server.hpp"

namespace tetris{
    server_t::server_t(int N): max_players(N){
        players = new game_t[N];
        clients_socket = new int[N];
        bzero(clients_socket,N*sizeof(int));
    }

    server_t::~server_t(){
        delete players;
        delete clients_socket;
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

        printf("Listener on port %d \n", port);   

        // FIXME: why 3?
        if (listen(master_socket, 3) < 0)   
            throw "ERROR listening"; 

        addrlen = sizeof(address);   
        puts("Waiting for connections ...");   
    
        // Wait for all clients
        for(int new_socket, i=0; i<max_players;i++){
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
                    throw "ERROR accepting";

            printf("New connection , socket fd is %d , ip is : %s , port : %d\n", 
                new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));   
            printf("Sending HI\n");
            strcpy(buffer,"HI");
            if( send(new_socket, buffer, strlen(buffer), 0) != strlen(buffer) )
                throw "ERROR sending HI";
            clients_socket[i] = new_socket;
        }

        // Say to all clients that we're ready
        printf("Sending PLAY\n");
        strcpy(buffer,"PLAY");
        for(int i=0;i<max_players;i++)
            if (send(clients_socket[i], buffer, strlen(buffer), 0) != strlen(buffer))
                throw "ERROR sending PLAY";

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
                printf("select error");

            for (int i = 0, sd; i < max_players; i++){
                sd = clients_socket[i];
                if (sd==0) continue;
                if (FD_ISSET( sd , &fds)) {
                    int valread = recv(sd, buffer, BUFSIZ, 0);
                    if ( valread == 0 ) {    // Somebody disconnected
                        getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);   
                        printf("Host disconnected , ip %s , port %d \n" ,  
                              inet_ntoa(address.sin_addr) , ntohs(address.sin_port));   

                        //Close the socket and mark as 0 in list for reuse
                        FD_CLR(sd, &fds);
                        shutdown(sd,2);
                        clients_socket[i] = 0;
                    } else {                // echo back
                        buffer[valread] = '\0';
                        printf("Message from player %i: %s\n" , i, buffer);   

                        // Send board or whatever
                        FD_CLR(sd, &fds);
                    }
                }
            }
        }
    }
}   